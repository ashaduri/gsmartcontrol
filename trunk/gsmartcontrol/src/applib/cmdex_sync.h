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

#ifndef APP_CMDEX_SYNC_H
#define APP_CMDEX_SYNC_H

#include <sigc++/sigc++.h>
#include <string>

#include "hz/error.h"
#include "hz/intrusive_ptr.h"
#include "hz/process_signal.h"  // hz::SIGNAL_*

#include "cmdex.h"



/// Information about finished command (structure for storing it).
/// Supports intrusive_ptr for memory-efficient storage.
struct CmdexSyncCommandInfoCopy : public hz::intrusive_ptr_referenced {
	CmdexSyncCommandInfoCopy(const std::string& c, const std::string& p,
			const std::string& so, const std::string& se, const std::string& em)
			: command(c), parameters(p), std_output(so), std_error(se), error_msg(em)
	{ }

	std::string command;  ///< Executed command
	std::string parameters;  ///< Command parameters
	std::string std_output;  ///< Stdout data
	std::string std_error;  ///< Stderr data
	std::string error_msg;  ///< Execution error message
};

/// A reference-counting pointer to CmdexSyncCommandInfoCopy
using CmdexSyncCommandInfoRefPtr = hz::intrusive_ptr<CmdexSyncCommandInfoCopy>;



/// Information about a finished command.
struct CmdexSyncCommandInfo {
	CmdexSyncCommandInfo(const std::string& c, const std::string& p,
			const std::string& so, const std::string& se, const std::string& em)
			: command(c), parameters(p), std_output(so), std_error(se), error_msg(em)
	{ }

	/// Make a copy of this structure for storage.
	CmdexSyncCommandInfoRefPtr copy() const
	{
		return new CmdexSyncCommandInfoCopy(command, parameters, std_output, std_error, error_msg);
	}

	const std::string& command;  ///< Executed command
	const std::string& parameters;  ///< Command parameters
	const std::string& std_output;  ///< Stdout data
	const std::string& std_error;  ///< Stderr data
	const std::string& error_msg;  ///< Execution error message
};



/// cmdex_sync_signal_execute_finish() return signal.
using cmdex_signal_execute_finish_type = sigc::signal<void, const CmdexSyncCommandInfo&>;


/// This signal is emitted every time execute() finishes.
cmdex_signal_execute_finish_type& cmdex_sync_signal_execute_finish();





/// Synchronous Cmdex (command executor) with ticking support.
class CmdexSync : public hz::intrusive_ptr_referenced, public sigc::trackable {
	public:

		/// Constructor
		CmdexSync(std::string command_name, std::string command_args)
			: CmdexSync()
		{
			this->set_command(std::move(command_name), std::move(command_args));
		}


		/// Constructor
		CmdexSync()
		{
			running_msg_ = "Running %s...";  // %s will be replaced by command basename
			set_error_header("An error occurred while executing the command:\n\n");
		}


		/// Virtual destructor
		virtual ~CmdexSync() = default;


		/// Set command to execute and its parameters
		void set_command(std::string command_name, std::string command_args)
		{
			cmdex_.set_command(command_name, command_args);
			// keep a copy locally to avoid locking on get() every time
			command_name_ = std::move(command_name);
			command_args_ = std::move(command_args);
		}


		/// Get command to execute
		std::string get_command_name() const
		{
			return command_name_;
		}


		/// Get command arguments
		std::string get_command_args() const
		{
			return command_args_;
		}


		/// Execute the command. The function will return only after the command exits.
		/// Calls signal_execute_tick signal repeatedly while doing stuff.
		/// Note: If the command _was_ executed, but there was an error,
		/// this will return true. Check get_error_msg() for emptiness.
		/// \c return false if failed to execute, true otherwise.
		virtual bool execute();


		/// Set timeout (in ms) to send SIGKILL after sending SIGTERM.
		/// Used if manual stop was requested through ticker.
		void set_forced_kill_timeout(int timeout_msec)
		{
			forced_kill_timeout_msec_ = timeout_msec;
		}


		/// Try to stop the process. Call this from ticker slot while executing.
		bool try_stop(hz::signal_t sig = hz::SIGNAL_SIGTERM)
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
		void set_stop_timeouts(int term_timeout_msec = 0, int kill_timeout_msec = 0)
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
		void set_buffer_sizes(int stdout_buffer_size = 0, int stderr_buffer_size = 0)
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
		void set_exit_status_translator(Cmdex::exit_status_translator_func_t func, void* user_data)
		{
			cmdex_.set_exit_status_translator(func, user_data);
		}


		/// Get command execution error message. If \c with_header
		/// is true, a header set using set_error_header() will be displayed first.
		std::string get_error_msg(bool with_header = false) const
		{
			if (with_header)
				return error_header_ + error_msg_;
			return error_msg_;
		}


		/// Set a message to display when running. %s in \c msg will be replaced by the command.
		void set_running_msg(const std::string& msg)
		{
			running_msg_ = msg;
		}


		/// Set error header string. See get_error_msg()
		void set_error_header(const std::string& msg)
		{
			error_header_ = msg;
		}


		/// Get error header string. See get_error_msg()
		std::string get_error_header()
		{
			return error_header_;
		}


		// ----------------- Signals


		/// Status flags for signal_execute_tick slots, along with possible return values.
		enum tick_status_t {
			status_starting,  ///< Return status will indicate whether to proceed with the execution
			status_failed,  ///< The execution failed
			status_running,  ///< Return status will indicate whether to abort the execution
			status_stopping,  ///< The child has been sent a signal
			status_stopped  ///< The child exited
		};


		/// This signal is emitted whenever something happens with the execution
		/// (the status is changed), and periodically while the process is running.
		sigc::signal<bool, tick_status_t> signal_execute_tick;



	protected:

		/// Import the last error from cmdex_ and clear all errors there.
		virtual void import_error();


		/// The warnings are already printed via debug_* in cmdex.
		/// Override if needed.
		virtual void on_error_warn(hz::ErrorBase* e)
		{
			if (e) {
				set_error_msg(e->get_message());  // this message will be displayed
			}
		}


		/// Set error message
		void set_error_msg(const std::string& error_msg)
		{
			error_msg_ = error_msg;
		}


		/// Get "running" message
		std::string get_running_msg() const
		{
			return running_msg_;
		}


		/// Get command executor object
		Cmdex& get_command_executor()
		{
			return cmdex_;
		}


	private:

		Cmdex cmdex_;  ///< Command executor

		std::string command_name_;  ///< Command name
		std::string command_args_;  ///< Command arguments

		std::string running_msg_;  ///< "Running" message (to show in the dialogs, etc...)

		int forced_kill_timeout_msec_ = 3 * 1000;  // 3 sec by default. Kill timeout in ms.

		std::string error_msg_;  ///< Execution error message
		std::string error_header_;  ///< The error message may have this prepended to it.

};






#endif

/// @}
