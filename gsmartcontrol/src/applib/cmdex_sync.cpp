/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <glib.h>  // g_usleep()

#include "cmdex_sync.h"



namespace {

	/// "Execution finished" signal
	static sigc::signal<void, const CmdexSyncCommandInfo&> s_cmdex_sync_signal_execute_finish;

	/// Mutex for "Execution finished" signal
	static hz::SyncPolicyMtDefault::Mutex s_cmdex_sync_signal_execute_finish_mutex;

}



cmdex_signal_execute_finish_type cmdex_sync_signal_execute_finish()
{
	return cmdex_signal_execute_finish_type(s_cmdex_sync_signal_execute_finish,
			new hz::SyncPolicyMtDefault::ScopedLock(s_cmdex_sync_signal_execute_finish_mutex));
}






bool CmdexSync::execute()
{
	set_error_msg("");  // clear old error if present

	bool slot_connected = !(signal_execute_tick.slots().begin() == signal_execute_tick.slots().end());

	if (slot_connected && !signal_execute_tick.emit(status_starting))
		return false;

	if (!cmdex_.execute()) {  // try to execute
		debug_out_error("app", DBG_FUNC_MSG << "cmdex_.execute() failed.\n");
		import_error();  // get error from cmdex and display warnings if needed

		// emit this for execution loggers
		cmdex_sync_signal_execute_finish()->emit(CmdexSyncCommandInfo(get_command_name(),
				get_command_args(), get_stdout_str(), get_stderr_str(), get_error_msg()));

		if (slot_connected)
			signal_execute_tick.emit(status_failed);
		return false;
	}

	bool stop_requested = false;  // stop requested from tick function
	bool signals_sent = false;  // stop signals sent

	while(!cmdex_.stopped_cleanup_needed()) {

		if (!stop_requested) {  // running and no stop requested yet
			// call the tick function with "running" periodically.
			// if it returns false, try to stop.
			if (slot_connected && !signal_execute_tick.emit(status_running)) {
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
			cmdex_.set_stop_timeouts(0, forced_kill_timeout_msec);
			// import_error();  // don't need errors here - they will be available later anyway.

			signals_sent = true;
		}


		// alert the tick function
		if (stop_requested && slot_connected) {
			signal_execute_tick.emit(status_stopping);  // ignore returned value here
		}


		// Without this, no event sources will be processed and the program will
		// hang waiting for the child to exit (the watch handler won't be called).
		// Note: If you have an idle callback, g_main_context_pending() will
		// always return true (until the idle callback returns false and unregisters itself).
		while(g_main_context_pending(NULL)) {
			g_main_context_iteration(NULL, false);
		}

		g_usleep(50*1000);  // 50 msec. avoids 100% CPU usage.
	}

	// command exited, do a cleanup.
	cmdex_.stopped_cleanup();
	import_error();  // get error from cmdex and display warnings if needed

	// emit this for execution loggers
	cmdex_sync_signal_execute_finish()->emit(CmdexSyncCommandInfo(get_command_name(),
			get_command_args(), get_stdout_str(), get_stderr_str(), get_error_msg()));

	if (slot_connected)
		signal_execute_tick.emit(status_stopped);  // last call

	return true;
}



void CmdexSync::import_error()
{
	cmdex_.errors_lock();

	Cmdex::error_list_t errors = cmdex_.get_errors(false);  // these are not clones
	hz::ErrorBase* e = 0;
	if (!errors.empty())
		e = errors.back()->clone();
	cmdex_.clear_errors(false);  // and clear them

	cmdex_.errors_unlock();

	if (e) {  // if error is present, alert the user
		on_error_warn(e);
	}
}








/// @}
