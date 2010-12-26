/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_CMDEX_SYNC_H
#define APP_CMDEX_SYNC_H

#include <sigc++/sigc++.h>
#include <string>

#include "hz/error.h"
#include "hz/intrusive_ptr.h"
#include "hz/process_signal.h"  // hz::SIGNAL_*
#include "hz/sync.h"
#include "hz/sync_lock_ptr.h"

#include "cmdex.h"



// Information about finished command (structure for storing).
// Supports intrusive_ptr for memory-efficient storage.
struct CmdexSyncCommandInfoCopy : public hz::intrusive_ptr_referenced {
	CmdexSyncCommandInfoCopy(const std::string& c, const std::string& p,
			const std::string& so, const std::string& se, const std::string& em)
			: command(c), parameters(p), std_output(so), std_error(se), error_msg(em)
	{ }

	std::string command;
	std::string parameters;
	std::string std_output;
	std::string std_error;
	std::string error_msg;
};

typedef hz::intrusive_ptr<CmdexSyncCommandInfoCopy> CmdexSyncCommandInfoRefPtr;



// Information about finished command.
struct CmdexSyncCommandInfo {
	CmdexSyncCommandInfo(const std::string& c, const std::string& p,
			const std::string& so, const std::string& se, const std::string& em)
			: command(c), parameters(p), std_output(so), std_error(se), error_msg(em)
	{ }

	// make a copy for storage.
	CmdexSyncCommandInfoRefPtr copy() const
	{
		return new CmdexSyncCommandInfoCopy(command, parameters, std_output, std_error, error_msg);
	}

	const std::string& command;
	const std::string& parameters;
	const std::string& std_output;
	const std::string& std_error;
	const std::string& error_msg;
};



typedef hz::sync_lock_ptr< sigc::signal<void, const CmdexSyncCommandInfo&>&,
		hz::SyncPolicyMtDefault::ScopedLock > cmdex_signal_execute_finish_type;


// Emitted every time execute() finishes.
cmdex_signal_execute_finish_type cmdex_sync_signal_execute_finish();





// Synchronious Cmdex (command executor) with ticking

class CmdexSync : public hz::intrusive_ptr_referenced, public sigc::trackable {

	public:

		CmdexSync(const std::string& command_exec, const std::string& command_args)
		{
			construct();
			this->set_command(command_exec, command_args);
		}


		CmdexSync()
		{
			construct();
		}


		void construct()
		{
			forced_kill_timeout_msec = 3 * 1000;  // 3 sec default
			running_msg_ = "Running %s...";  // %s will be replaced by command basename
			set_error_header("An error occurred while executing the command:\n\n");
		}


		virtual ~CmdexSync()
		{ }


		void set_command(const std::string& command_name, const std::string& command_args)
		{
			cmdex_.set_command(command_name, command_args);
			// keep a copy locally to avoid locking on get() every time
			command_name_ = command_name;
			command_args_ = command_args;
		}


		std::string get_command_name() const
		{
			return command_name_;
		}

		std::string get_command_args() const
		{
			return command_args_;
		}


		// Returns false if failed to execute, true otherwise.
		// The function will return only after the command exits.
		// Calls signal_execute_tick signal repeatedly while doing stuff.
		// Note: If the command _was_ executed, but there was an error,
		// this will return true. Check get_error_msg() emptiness.
		virtual bool execute();


		// Timeout to send SIGKILL after sending SIGTERM.
		// Used if manual stop was requested through ticker.
		void set_forced_kill_timeout(int timeout_msec)
		{
			forced_kill_timeout_msec = timeout_msec;
		}


		// Call from slot while executing.
		bool try_stop(hz::signal_t sig = hz::SIGNAL_SIGTERM)
		{
			return cmdex_.try_stop(sig);
		}

		// Call from slot while executing.
		bool try_kill()
		{
			return cmdex_.try_kill();
		}

		// Call from slot while executing.
		// set a timeout (since call to this function) to terminate, kill or both (use 0 to ignore the parameter).
		// the timeouts will be unset automatically when the command exits.
		void set_stop_timeouts(int term_timeout_msec = 0, int kill_timeout_msec = 0)
		{
			cmdex_.set_stop_timeouts(term_timeout_msec, kill_timeout_msec);
		}

		// Call from slot while executing.
		// Unset timeouts. This will stop the timeout counters.
		void unset_stop_timeouts()
		{
			cmdex_.unset_stop_timeouts();
		}

		// Call from slot while executing.
		bool is_running() const
		{
			return cmdex_.is_running();
		}

		// Use 0 to ignore the parameter. Call before execute().
		// See Cmdex for details.
		void set_buffer_sizes(int stdout_buffer_size = 0, int stderr_buffer_size = 0)
		{
			cmdex_.set_buffer_sizes(stdout_buffer_size, stderr_buffer_size);
		}

		std::string get_stdout_str(bool clear_existing = false)
		{
			return cmdex_.get_stdout_str(clear_existing);
		}

		std::string get_stderr_str(bool clear_existing = false)
		{
			return cmdex_.get_stderr_str(clear_existing);
		}

		void set_exit_status_translator(Cmdex::exit_status_translator_func_t func, void* user_data)
		{
			cmdex_.set_exit_status_translator(func, user_data);
		}


		std::string get_error_msg(bool with_header = false) const
		{
			if (with_header)
				return error_header_ + error_msg_;
			return error_msg_;
		}

		void set_running_msg(const std::string& msg)
		{
			running_msg_ = msg;
		}

		void set_error_header(const std::string& msg)
		{
			error_header_ = msg;
		}

		std::string get_error_header()
		{
			return error_header_;
		}


		// ----------------- Signals


		// status flags for signal_execute_tick slots, along with possible return values.
		enum tick_status_t {
			status_starting,  // return status will indicate whether to proceed with the execution
			status_failed,  // the execution failed
			status_running,  // return status will indicate whether to abort the execution
			status_stopping,  // the child has been sent a signal
			status_stopped  // the child exited
		};

		// this signal is emitted whenever something happens with the execution.
		sigc::signal<bool, tick_status_t> signal_execute_tick;



	protected:

		// import the last error from cmdex_ and clear all errors there
		virtual void import_error();


		// The warnings are already printed via debug_* in cmdex.
		// Override if needed.
		virtual void on_error_warn(hz::ErrorBase* e)
		{
			if (e) {
				set_error_msg(e->get_message());  // this message will be displayed
			}
		}


		void set_error_msg(const std::string& error_msg)
		{
			error_msg_ = error_msg;
		}


		std::string get_running_msg() const
		{
			return running_msg_;
		}


		Cmdex& get_command_executor()
		{
			return cmdex_;
		}



		Cmdex cmdex_;

		std::string command_name_;
		std::string command_args_;

		std::string running_msg_;  // message to show in the dialogs, etc...

		int forced_kill_timeout_msec;

		std::string error_msg_;
		std::string error_header_;
		hz::ErrorBase* error_;

};






#endif
