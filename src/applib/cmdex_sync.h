/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_CMDEX_SYNC_H
#define APP_CMDEX_SYNC_H

#include <sigc++/sigc++.h>
#include <string>
#include <chrono>
#include <utility>

#include "hz/error.h"
#include "hz/process_signal.h"  // hz::SIGNAL_*

#include "cmdex.h"



/// Information about a finished command.
struct CmdexSyncCommandInfo {
	CmdexSyncCommandInfo(std::string c, std::string p,
			std::string so, std::string se, std::string em)
			: command(std::move(c)), parameters(std::move(p)), std_output(std::move(so)),
			std_error(std::move(se)), error_msg(std::move(em))
	{ }

	const std::string command;  ///< Executed command
	const std::string parameters;  ///< Command parameters
	const std::string std_output;  ///< Stdout data
	const std::string std_error;  ///< Stderr data
	const std::string error_msg;  ///< Execution error message
};



/// cmdex_sync_signal_execute_finish() return signal.
using cmdex_signal_execute_finish_type = sigc::signal<void, const CmdexSyncCommandInfo&>;


/// This signal is emitted every time execute() finishes.
cmdex_signal_execute_finish_type& cmdex_sync_signal_execute_finish();





/// Synchronous Cmdex (command executor) with ticking support.
class CmdexSync : public sigc::trackable {
	public:

		/// Constructor
		CmdexSync();

		/// Constructor
		CmdexSync(std::string command_name, std::string command_args);

		/// Virtual destructor
		virtual ~CmdexSync() = default;


		/// Set command to execute and its parameters
		void set_command(std::string command_name, std::string command_args);


		/// Get command to execute
		std::string get_command_name() const;


		/// Get command arguments
		std::string get_command_args() const;


		/// Execute the command. The function will return only after the command exits.
		/// Calls signal_execute_tick signal repeatedly while doing stuff.
		/// Note: If the command _was_ executed, but there was an error,
		/// this will return true. Check get_error_msg() for emptiness.
		/// \c return false if failed to execute, true otherwise.
		virtual bool execute();


		/// Set timeout (in ms) to send SIGKILL after sending SIGTERM.
		/// Used if manual stop was requested through ticker.
		void set_forced_kill_timeout(std::chrono::milliseconds timeout_msec);


		/// Try to stop the process. Call this from ticker slot while executing.
		bool try_stop(hz::Signal sig = hz::Signal::Terminate)
		{
			return cmdex_.try_stop(sig);
		}


		/// Same as try_stop(hz::SIGNAL_SIGKILL).
		bool try_kill()
		{
			return cmdex_.try_kill();
		}


		/// Set a timeout (since call to this function) to terminate, kill or both (use 0 to ignore the parameter).
		/// the timeouts will be unset automatically when the command exits.
		/// Call from ticker slot while executing.
		void set_stop_timeouts(std::chrono::milliseconds term_timeout_msec, std::chrono::milliseconds kill_timeout_msec)
		{
			cmdex_.set_stop_timeouts(term_timeout_msec, kill_timeout_msec);
		}


		/// Unset terminate / kill timeouts. This will stop the timeout counters.
		/// Call from ticker slot while executing.
		void unset_stop_timeouts()
		{
			cmdex_.unset_stop_timeouts();
		}


		/// Check if the child process is running. See Cmdex::is_running().
		/// Call from ticker slot while executing.
		bool is_running() const
		{
			return cmdex_.is_running();
		}


		/// See Cmdex::set_buffer_sizes() for details. Call this before execute().
		void set_buffer_sizes(gsize stdout_buffer_size = 0, gsize stderr_buffer_size = 0)
		{
			cmdex_.set_buffer_sizes(stdout_buffer_size, stderr_buffer_size);
		}

		/// See Cmdex::get_stdout_str() for details.
		std::string get_stdout_str(bool clear_existing = false)
		{
			return cmdex_.get_stdout_str(clear_existing);
		}

		/// See Cmdex::get_stderr_str() for details.
		std::string get_stderr_str(bool clear_existing = false)
		{
			return cmdex_.get_stderr_str(clear_existing);
		}

		/// See Cmdex::set_exit_status_translator() for details.
		void set_exit_status_translator(Cmdex::exit_status_translator_func_t func)
		{
			cmdex_.set_exit_status_translator(func);
		}


		/// Get command execution error message. If \c with_header
		/// is true, a header set using set_error_header() will be displayed first.
		std::string get_error_msg(bool with_header = false) const;


		/// Set a message to display when running. "{command}" in \c msg will be replaced by the command.
		void set_running_msg(const std::string& msg);


		/// Set error header string. See get_error_msg()
		void set_error_header(const std::string& msg);


		/// Get error header string. See get_error_msg()
		std::string get_error_header();


		// ----------------- Signals


		/// Status flags for signal_execute_tick slots, along with possible return values.
		enum class TickStatus {
			starting,  ///< Return status will indicate whether to proceed with the execution
			failed,  ///< The execution failed
			running,  ///< Return status will indicate whether to abort the execution
			stopping,  ///< The child has been sent a signal
			stopped  ///< The child exited
		};


		/// This signal is emitted whenever something happens with the execution
		/// (the status is changed), and periodically while the process is running.
		sigc::signal<bool, TickStatus> signal_execute_tick;



	protected:

		/// Import the last error from cmdex_ and clear all errors there.
		virtual void import_error();


		/// The warnings are already printed via debug_* in cmdex.
		/// Override if needed.
		virtual void on_error_warn(hz::ErrorBase* e);


		/// Set error message
		void set_error_msg(const std::string& error_msg);


		/// Get "running" message
		std::string get_running_msg() const;


		/// Get command executor object
		Cmdex& get_command_executor();


	private:

		Cmdex cmdex_;  ///< Command executor

		std::string command_name_;  ///< Command name
		std::string command_args_;  ///< Command arguments

		std::string running_msg_;  ///< "Running" message (to show in the dialogs, etc...)

		std::chrono::milliseconds forced_kill_timeout_msec_ = std::chrono::seconds(3);  // 3 sec by default. Kill timeout in ms.

		std::string error_msg_;  ///< Execution error message
		std::string error_header_;  ///< The error message may have this prepended to it.

};






#endif

/// @}
