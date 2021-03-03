/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "local_glibmm.h"
#include <glib.h>  // g_usleep()

#include "cmdex_sync.h"



cmdex_signal_execute_finish_type& cmdex_sync_signal_execute_finish()
{
	/// "Execution finished" signal
	static sigc::signal<void, const CmdexSyncCommandInfo&> s_cmdex_sync_signal_execute_finish;
	return s_cmdex_sync_signal_execute_finish;
}



CmdexSync::CmdexSync(std::string command_name, std::string command_args)
		: CmdexSync()
{
	this->set_command(std::move(command_name), std::move(command_args));
}



CmdexSync::CmdexSync()
{
	/// Translators: {command} will be replaced by command name.
	running_msg_ = _("Running {command}...");
	set_error_header(std::string(_("An error occurred while executing command:")) + "\n\n");
}



void CmdexSync::set_command(std::string command_name, std::string command_args)
{
	cmdex_.set_command(command_name, command_args);
	// keep a copy locally to avoid locking on get() every time
	command_name_ = std::move(command_name);
	command_args_ = std::move(command_args);
}



std::string CmdexSync::get_command_name() const
{
	return command_name_;
}



std::string CmdexSync::get_command_args() const
{
	return command_args_;
}



bool CmdexSync::execute()
{
	set_error_msg("");  // clear old error if present

	bool slot_connected = !(signal_execute_tick.slots().begin() == signal_execute_tick.slots().end());

	if (slot_connected && !signal_execute_tick.emit(TickStatus::starting))
		return false;

	if (!cmdex_.execute()) {  // try to execute
		debug_out_error("app", DBG_FUNC_MSG << "cmdex_.execute() failed.\n");
		import_error();  // get error from cmdex and display warnings if needed

		// emit this for execution loggers
		cmdex_sync_signal_execute_finish().emit(CmdexSyncCommandInfo(get_command_name(),
				get_command_args(), get_stdout_str(), get_stderr_str(), get_error_msg()));

		if (slot_connected)
			signal_execute_tick.emit(TickStatus::failed);
		return false;
	}

	bool stop_requested = false;  // stop requested from tick function
	bool signals_sent = false;  // stop signals sent

	while(!cmdex_.stopped_cleanup_needed()) {

		if (!stop_requested) {  // running and no stop requested yet
			// call the tick function with "running" periodically.
			// if it returns false, try to stop.
			if (slot_connected && !signal_execute_tick.emit(TickStatus::running)) {
				debug_out_info("app", DBG_FUNC_MSG << "execute_tick slot returned false, trying to stop the program.\n");
				stop_requested = true;
			}
		}


		if (stop_requested && !signals_sent) {  // stop request received
			// send the stop request to the command
			if (!cmdex_.try_stop()) {  // try sigterm. this returns false if it can't be done (no permissions, zombie)
				debug_out_warn("app", DBG_FUNC_MSG << "cmdex_.try_stop() returned false.\n");
			}

			// set sigkill timeout to 3 sec (in case sigterm fails); won't do anything if already exited.
			cmdex_.set_stop_timeouts(std::chrono::milliseconds(0), forced_kill_timeout_msec_);
			// import_error();  // don't need errors here - they will be available later anyway.

			signals_sent = true;
		}


		// alert the tick function
		if (stop_requested && slot_connected) {
			signal_execute_tick.emit(TickStatus::stopping);  // ignore returned value here
		}


		// Without this, no event sources will be processed and the program will
		// hang waiting for the child to exit (the watch handler won't be called).
		// Note: If you have an idle callback, g_main_context_pending() will
		// always return true (until the idle callback returns false and unregisters itself).
		while(g_main_context_pending(nullptr) != FALSE) {
			g_main_context_iteration(nullptr, FALSE);
		}

		const gulong sleep_us = 50*1000;  // 50 msec. avoids 100% CPU usage.
		g_usleep(sleep_us);
	}

	// command exited, do a cleanup.
	cmdex_.stopped_cleanup();
	import_error();  // get error from cmdex and display warnings if needed

	// emit this for execution loggers
	cmdex_sync_signal_execute_finish().emit(CmdexSyncCommandInfo(get_command_name(),
			get_command_args(), get_stdout_str(), get_stderr_str(), get_error_msg()));

	if (slot_connected)
		signal_execute_tick.emit(TickStatus::stopped);  // last call

	return true;
}



void CmdexSync::set_forced_kill_timeout(std::chrono::milliseconds timeout_msec)
{
	forced_kill_timeout_msec_ = timeout_msec;
}



std::string CmdexSync::get_error_msg(bool with_header) const
{
	if (with_header)
		return error_header_ + error_msg_;
	return error_msg_;
}



void CmdexSync::set_running_msg(const std::string& msg)
{
	running_msg_ = msg;
}



void CmdexSync::set_error_header(const std::string& msg)
{
	error_header_ = msg;
}



std::string CmdexSync::get_error_header()
{
	return error_header_;
}



void CmdexSync::import_error()
{
	Cmdex::error_list_t errors = cmdex_.get_errors();  // these are not clones
	hz::ErrorBase* e = nullptr;
	if (!errors.empty())
		e = errors.back()->clone();
	cmdex_.clear_errors();  // and clear them

	if (e) {  // if error is present, alert the user
		on_error_warn(e);
	}
}



void CmdexSync::on_error_warn(hz::ErrorBase* e)
{
	if (e) {
		set_error_msg(e->get_message());  // this message will be displayed
	}
}



void CmdexSync::set_error_msg(const std::string& error_msg)
{
	error_msg_ = error_msg;
}



std::string CmdexSync::get_running_msg() const
{
	return running_msg_;
}



Cmdex& CmdexSync::get_command_executor()
{
	return cmdex_;
}







/// @}
