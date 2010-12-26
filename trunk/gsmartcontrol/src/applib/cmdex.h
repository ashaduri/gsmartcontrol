/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_CMDEX_H
#define APP_CMDEX_H

#include <string>
#include <glib.h>

#include "hz/process_signal.h"  // hz::SIGNAL_*
#include "hz/error_holder.h"
#include "hz/sync.h"  // hz::SyncPolicyNone



/*
There are two ways to detect when the command exits:

1. Add a callback to signal_exited.

2. Manually poll stopped_cleanup_needed() (from the same thread).

In both cases, stopped_cleanup() must be called afterwards
(from the main thread).
*/




// inheriting from sigc::trackable automatically disconnects this class's
// slots upon destruction.
class Cmdex : public hz::ErrorHolder<hz::SyncPolicyNone> {

	public:

		using hz::ErrorHolder<hz::SyncPolicyNone>::ptr_error_list_t;  // auto-deleting container
		using hz::ErrorHolder<hz::SyncPolicyNone>::error_list_t;

		typedef std::string (*exit_status_translator_func_t)(int, void*);
		typedef void (*exited_callback_func_t)(void*);


		Cmdex(
			// exit_status_translator_func_t translator_func,
			exited_callback_func_t exited_cb = NULL, void* exited_cb_data = NULL
			)
			: running_(false), kill_signal_sent_(0), child_watch_handler_called_(false),
			pid_(0), waitpid_status_(0),
			timer_(g_timer_new()),
			event_source_id_term(0), event_source_id_kill(0),
			fd_stdout_(0), fd_stderr_(0), channel_stdout_(0), channel_stderr_(0),
			channel_stdout_buffer_size_(100 * 1024), channel_stderr_buffer_size_(10 * 1024),  // 100K and 10K
			event_source_id_stdout_(0), event_source_id_stderr_(0),
			translator_func_(0), translator_func_data_(0),
			exited_callback_(exited_cb), exited_callback_data_(exited_cb_data)
		{ }



		// Please don't destroy this object unless the child has exited. It will leak stuff
		// and possibly crash, etc... .
		~Cmdex()
		{
			// This will help if object is destroyed after the command has exited, but before
			// stopped_cleanup() has been called.
			stopped_cleanup();

			g_timer_destroy(timer_);

			// no need to destroy the channels - stopped_cleanup() calls
			// cleanup_members(), which deletes them.
		}


		// Call before execute.
		// Note: The command and the arguments _must_ be shell-escaped.
		// Use g_shell_quote() or Glib::shell_quote(). Note that each argument
		// must be escaped separately.
		void set_command(const std::string& command_exec, const std::string& command_args)
		{
			command_exec_ = command_exec;
			command_args_ = command_args;
		}


		// launch the command.
		bool execute();


		// send SIGTERM(15) (terminate).
		// use after execute(). using it after the command has exited has no effect.
		bool try_stop(hz::signal_t sig = hz::SIGNAL_SIGTERM);


		// Send SIGKILL(9) (kill). This signal cannot be overridden in child process.
		// use after execute(). using it after the command has exited has no effect.
		bool try_kill()
		{
			return try_stop(hz::SIGNAL_SIGKILL);
		}


		// set a timeout (since call to this function) to terminate, kill or both (use 0 to ignore the parameter).
		// the timeouts will be unset automatically when the command exits.
		// This has an effect only if the command is running (after execute()).
		void set_stop_timeouts(int term_timeout_msec = 0, int kill_timeout_msec = 0);

		// Unset timeouts. This will stop the timeout counters.
		// This has an effect only if the command is running (after execute()).
		void unset_stop_timeouts();


		// If stopped_cleanup_needed() returned true, call this. The command
		// should be exited by this time. Must be called before the next execute().
		void stopped_cleanup();


		// Returns true if command has stopped.
		// Call repeatedly in a waiting function, after execute(). When returns true, call stopped_cleanup().
		bool stopped_cleanup_needed()
		{
			return (child_watch_handler_called_);
		}


		// The process is running. Note that if this returns false, it doesn't mean that
		// the io channels have been closed or that the data may be read safely. Poll
		// stopped_cleanup_needed() instead.
		bool is_running() const
		{
			return running_;
		}



		// Call this before execution. There is a race-like condition - when the command
		// outputs something, the io channel reads it from fd to its buffer and the event
		// source callback is called. If the command dies, the io channel callback reads
		// the remaining data to the channel buffer.
		// Since the event source callbacks (which read from the buffer and empty it)
		// happen rather sporadically (from the glib loop), the buffer may not get read and
		// emptied at all (before the command exits). This is why it's necessary to have
		// a buffer size which potentially can hold _all_ the command output.
		// A way to fight this is to increase event source priority (which may not help).
		// Another way is to delay the command exit so that the event source callback
		// catches on and reads the buffer.

		// Use 0 to ignore the parameter. Call before execute().
		void set_buffer_sizes(int stdout_buffer_size = 0, int stderr_buffer_size = 0)
		{
			if (stdout_buffer_size)
				channel_stdout_buffer_size_ = stdout_buffer_size;  // 100K by default
			if (stderr_buffer_size)
				channel_stderr_buffer_size_ = stderr_buffer_size;  // 10K by default
		}



		// If stdout_make_str_as_available_ is false, call this after stopped_cleanup(),
		// before next execute(). If it's true, you may call this before the command has
		// stopped, but it will decrease performance significantly.
		std::string get_stdout_str(bool clear_existing = false)
		{
			// debug_out_dump("app", str_stdout_);
			if (clear_existing) {
				std::string ret = str_stdout_;
				str_stdout_.clear();
				return ret;
			}
			return str_stdout_;
		}


		// See notes on get_stdout_str().
		std::string get_stderr_str(bool clear_existing = false)
		{
			if (clear_existing) {
				std::string ret = str_stderr_;
				str_stderr_.clear();
				return ret;
			}
			return str_stderr_;
		}


		// Return execution time, in seconds. Call after execute().
		double get_execution_time()
		{
			gulong microsec = 0;
			return g_timer_elapsed(timer_, &microsec);
		}


		// this will disconnect earlier slots. Call before execute().
		void set_exit_status_translator(exit_status_translator_func_t func, void* user_data)
		{
			translator_func_ = func;
			translator_func_data_ = user_data;
		}


		// You can use stopped_cleanup_needed() polling instead.
		void set_exited_callback(exited_callback_func_t func, void* user_data)
		{
			exited_callback_ = func;
			exited_callback_data_ = user_data;
		}



	// these are sorta-private

		// for passing to callback
		enum channel_t {
			channel_type_stdout,
			channel_type_stderr
		};


		// Callbacks (Note: These are called by the real callbacks)

		static void on_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data);

		static gboolean on_channel_io(GIOChannel* source,
				GIOCondition cond, Cmdex* self, channel_t type);


	private:


		void cleanup_members()
		{
			kill_signal_sent_ = 0;
			child_watch_handler_called_ = false;
			pid_ = 0;
			waitpid_status_ = 0;
			event_source_id_stdout_ = 0;
			event_source_id_stderr_ = 0;
			fd_stdout_ = 0;
			fd_stderr_ = 0;

			if (channel_stdout_) {
				g_io_channel_shutdown(channel_stdout_, false, NULL);
				g_io_channel_unref(channel_stdout_);
				channel_stdout_ = 0;
			}
			if (channel_stderr_) {
				g_io_channel_shutdown(channel_stderr_, false, NULL);
				g_io_channel_unref(channel_stderr_);
				channel_stderr_ = 0;
			}
		}



		// default command and its args. std::strings, not ustrings.
		std::string command_exec_;  // binary name. NOT affected by cleanup().
		std::string command_args_;  // args that always go with binary. NOT affected by cleanup().


		bool running_;  // child process is running now. NOT affected by cleanup().
		int kill_signal_sent_;  // command has been sent this signal to terminate
		bool child_watch_handler_called_;  // true after child_watch_handler callback, before stopped_cleanup().

		GPid pid_;  // int in Unix, pointer in win32
		int waitpid_status_;  // after the command is stopped, before cleanup, this will be available.


		GTimer* timer_;  // keep track of elapsed time since command execution. not used by this class, but may be handy.

		int event_source_id_term;
		int event_source_id_kill;


		int fd_stdout_;
		int fd_stderr_;

		GIOChannel* channel_stdout_;
		GIOChannel* channel_stderr_;

		int channel_stdout_buffer_size_;  // NOT affected by cleanup().
		int channel_stderr_buffer_size_;  // NOT affected by cleanup().

		int event_source_id_stdout_;
		int event_source_id_stderr_;

		std::string str_stdout_;  // NOT affected by cleanup().
		std::string str_stderr_;  // NOT affected by cleanup().


		// signals

		// convert command exit status to message string
		exit_status_translator_func_t translator_func_;  // NOT affected by cleanup().
		void* translator_func_data_;

		// "command exited" signal callback.
		exited_callback_func_t exited_callback_;  // NOT affected by cleanup().
		void* exited_callback_data_;


};






#endif
