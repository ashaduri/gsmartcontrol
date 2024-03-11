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

#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include <sigc++/sigc++.h>
#include <string>
#include <chrono>
#include <utility>

#include "hz/error_holder.h"
#include "hz/process_signal.h"  // hz::SIGNAL_*

#include "async_command_executor.h"



/// Information about a finished command.
struct CommandExecutorResult {
	CommandExecutorResult(std::string arg_command, std::string arg_parameters,
			std::string arg_std_output, std::string arg_std_error, std::string arg_error_message)
			: command(std::move(arg_command)),
			parameters(std::move(arg_parameters)),
			std_output(std::move(arg_std_output)),
			std_error(std::move(arg_std_error)),
			error_message(std::move(arg_error_message))
	{ }

	const std::string command;  ///< Executed command
	const std::string parameters;  ///< Command parameters
	const std::string std_output;  ///< Stdout data
	const std::string std_error;  ///< Stderr data
	const std::string error_message;  ///< Execution error message
};



/// cmdex_sync_signal_execute_finish() return signal.
using cmdex_signal_execute_finish_t = sigc::signal<void, const CommandExecutorResult&>;


/// This signal is emitted every time execute() finishes.
cmdex_signal_execute_finish_t& cmdex_sync_signal_execute_finish();





/// Synchronous AsyncCommandExecutor (command executor) with ticking support.
class CommandExecutor : public sigc::trackable {
	public:

		/// Constructor
		CommandExecutor();

		/// Constructor
		CommandExecutor(std::string command_name, std::string command_args);


		/// Deleted
		CommandExecutor(const CommandExecutor& other) = delete;

		/// Deleted
		CommandExecutor(CommandExecutor&& other) = delete;

		/// Deleted
		CommandExecutor& operator=(CommandExecutor& other) = delete;

		/// Deleted
		CommandExecutor& operator=(CommandExecutor&& other) = delete;


		/// Virtual destructor
		virtual ~CommandExecutor() = default;


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
		bool try_stop(hz::Signal sig = hz::Signal::Terminate);


		/// Same as try_stop(hz::SIGNAL_SIGKILL).
		bool try_kill();


		/// Set a timeout (since call to this function) to terminate, kill or both (use 0 to ignore the parameter).
		/// the timeouts will be unset automatically when the command exits.
		/// Call from ticker slot while executing.
		void set_stop_timeouts(std::chrono::milliseconds term_timeout_msec, std::chrono::milliseconds kill_timeout_msec);


		/// Unset terminate / kill timeouts. This will stop the timeout counters.
		/// Call from ticker slot while executing.
		void unset_stop_timeouts();


		/// Check if the child process is running. See AsyncCommandExecutor::is_running().
		/// Call from ticker slot while executing.
		bool is_running() const;


		/// See AsyncCommandExecutor::set_buffer_sizes() for details. Call this before execute().
		void set_buffer_sizes(gsize stdout_buffer_size = 0, gsize stderr_buffer_size = 0);

		/// See AsyncCommandExecutor::get_stdout_str() for details.
		std::string get_stdout_str(bool clear_existing = false);

		/// See AsyncCommandExecutor::get_stderr_str() for details.
		std::string get_stderr_str(bool clear_existing = false);

		/// See AsyncCommandExecutor::set_exit_status_translator() for details.
		void set_exit_status_translator(AsyncCommandExecutor::exit_status_translator_func_t func);


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


		sigc::signal<bool, TickStatus>& signal_execute_tick();



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
		AsyncCommandExecutor& get_async_executor();


	private:

		AsyncCommandExecutor cmdex_;  ///< Command executor

		std::string command_name_;  ///< Command name
		std::string command_args_;  ///< Command arguments

		std::string running_msg_;  ///< "Running" message (to show in the dialogs, etc...)

		std::chrono::milliseconds forced_kill_timeout_msec_ = std::chrono::seconds(3);  // 3 sec by default. Kill timeout in ms.

		std::string error_msg_;  ///< Execution error message
		std::string error_header_;  ///< The error message may have this prepended to it.


		/// This signal is emitted whenever something happens with the execution
		/// (the status is changed), and periodically while the process is running.
		sigc::signal<bool, TickStatus> signal_execute_tick_;


};






#endif

/// @}
