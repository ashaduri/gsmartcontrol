/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_CMDEX_H
#define APP_CMDEX_H

#include <string>
#include <glib.h>

#include "hz/process_signal.h"  // hz::SIGNAL_*, hz::process_signal_send (cpp), win32's W* (cpp)
#include "hz/error_holder.h"
#include "hz/sync.h"
#include "hz/sync_policy_glib.h"



/*
There are two ways to detect when the command exits:

1. Add a callback to signal_exited. The callback will be executed
in non-main thread, which means that all data access MUST be
very carefully evaluated (and locked in both threads).

2. Manually poll stopped_cleanup_needed() from the main thread.

In both cases, stopped_cleanup() must be called afterwards,
from the _main_ thread.
*/



// typedef hz::SyncPolicyGlib CmdexSyncPolicy;
typedef hz::SyncPolicyMtDefault CmdexSyncPolicy;



// inheriting from sigc::trackable automatically disconnects this class's
// slots upon destruction.
class Cmdex : public hz::ErrorHolder<CmdexSyncPolicy> {

	public:

		using hz::ErrorHolder<CmdexSyncPolicy>::ptr_error_list_t;  // auto-deleting container
		using hz::ErrorHolder<CmdexSyncPolicy>::error_list_t;

		typedef hz::ErrorHolder<CmdexSyncPolicy>::ErrorScopedLock ScopedLock;
		typedef hz::ErrorHolder<CmdexSyncPolicy>::ErrorLockPolicy LockPolicy;

		typedef std::string (*exit_status_translator_func_t)(int, void*);
		typedef void (*exited_callback_func_t)(void*);


		Cmdex(
			// exit_status_translator_func_t translator_func,
			exited_callback_func_t exited_cb = NULL, void* exited_cb_data = NULL
			)
			: object_mutex_(error_object_mutex_),
			running_(false), kill_signal_sent_(0), child_watch_handler_called_(false),
			pid_(0), waitpid_status_(0),
			timer_(g_timer_new()),
			event_source_id_term(0), event_source_id_kill(0),
			fd_stdout_(0), fd_stderr_(0), channel_stdout_(0), channel_stderr_(0),
			channel_stdout_buffer_size_(100 * 1024), channel_stderr_buffer_size_(10 * 1024),  // 100K and 10K
			event_source_id_stdout_(0), event_source_id_stderr_(0),
			// Don't set false here (in any of these) - there's a bug in windows which causes
			// output to be completely swallowed at random times.
			stdout_make_str_as_available_(true), stderr_make_str_as_available_(true),
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
		// do_lock is kind of private and indicates that mutex should be locked prior to
		// accessing the object members.
		void unset_stop_timeouts(bool do_lock = true);


		// If stopped_cleanup_needed() returned true, call this. The command
		// should be exited by this time. Must be called before the next execute().
		void stopped_cleanup();


		// Returns true if command has stopped.
		// Call repeatedly in a waiting function, after execute(). When returns true, call stopped_cleanup().
		// do_lock is private, set to false only if you already hold the object lock.
		bool stopped_cleanup_needed(bool do_lock = true)
		{
			ScopedLock locker(object_mutex_, do_lock);
			return (child_watch_handler_called_);
		}


		// The process is running. Note that if this returns false, it doesn't mean that
		// the io channels have been closed or that the data may be read safely. Poll
		// stopped_cleanup_needed() instead.
		bool is_running() const
		{
			ScopedLock locker(object_mutex_);
			return running_;
		}



		// Call this before execution. There is a race-like condition - when the command
		// outputs something, the io channel reads it from fd to its buffer and the event
		// source callback is called. If the command dies, the io channel thread reads
		// the remaining data to the channel buffer.
		// Since the event source callbacks (which read from the buffer and empty it)
		// happen rather sporadically in another thread, the buffer may not get read and
		// emptied at all (before the command exits). This is why it's necessary to have
		// a buffer size which potentially can hold _all_ the command output.
		// A way to fight this is to increase event source thread priority (which may not help).
		// Another way is to delay the command exit so that the event source thread
		// catches on and reads the buffer.

		// Use 0 to ignore the parameter. Call before execute().
		void set_buffer_sizes(int stdout_buffer_size = 0, int stderr_buffer_size = 0)
		{
			ScopedLock locker(object_mutex_);
			if (stdout_buffer_size)
				channel_stdout_buffer_size_ = stdout_buffer_size;  // 100K by default
			if (stderr_buffer_size)
				channel_stderr_buffer_size_ = stderr_buffer_size;  // 10K by default
		}


		// Set the flags for child stdout and stderr. If stdout_str_as_available is true,
		// then get_stdout_str() will return the stdout string as it becomes available,
		// while false will make it available only after the command exits. Note that
		// if the command outputs a lot of data in a very short period of time and
		// then exits, false is much better choice because it's much faster.
		// If you want to monitor the data as it becomes available and clean it to
		// save some memory, use true.
		// Call before execute().
		void set_str_available(bool stdout_str_as_available = false, bool stderr_str_as_available = true)
		{
			ScopedLock locker(object_mutex_);
			stdout_make_str_as_available_ = stdout_str_as_available;
			stderr_make_str_as_available_ = stderr_str_as_available;
		}



		// If stdout_make_str_as_available_ is false, call this after stopped_cleanup(),
		// before next execute(). If it's true, you may call this before the command has
		// stopped, but it will decrease performance significantly.
		std::string get_stdout_str(bool clear_existing = false)
		{
			ScopedLock locker(object_mutex_);
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
			ScopedLock locker(object_mutex_);
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
			ScopedLock locker(object_mutex_);
			gulong microsec = 0;
			return g_timer_elapsed(timer_, &microsec);
		}


		// don't return sigc::connection - it's better for thread-safety this way.
		// this will disconnect earlier slots. Call before execute().
		void set_exit_status_translator(exit_status_translator_func_t func, void* user_data)
		{
			ScopedLock locker(object_mutex_);
			translator_func_ = func;
			translator_func_data_ = user_data;
		}


		// NOTE: The callback will be called in non-main thread! (the call is enclosed
		// in lock/unlock).
		// Don't use this unless absolutely necessary, use stopped_cleanup_needed()
		// polling instead.
		void set_exited_callback(exited_callback_func_t func, void* user_data)
		{
			ScopedLock locker(object_mutex_);
			exited_callback_ = func;
			exited_callback_data_ = user_data;
		}



		// use lock() / unlock() if using members of this class outside of it.

		void lock()
		{
			errors_lock();  // defer to parent's mutex - we use it for the whole object.
		}


		void unlock()
		{
			errors_unlock();
		}



		// protect all member variable access by this mutex, as well as
		// serialize event callback execution (in respect of main thread too).
		// it's possible to implement mutexes for each member individually, but
		// it's impractical.
		// mutable allows the definition of const member functions - they only
		// read the data, but do it in a lock-protected manner.
// 		mutable Glib::Mutex object_mutex;  // aka access serializer mutex
		// use ErrorHolder's mutex




	// these are sorta-private

		// for passing to callback
		enum channel_t {
			channel_type_stdout,
			channel_type_stderr
		};


		// Callbacks (Note: These are called by the real callbacks)

		static void on_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data);

		static gboolean on_channel_io_buffered(GIOChannel* source,
				GIOCondition cond, Cmdex* self, channel_t type);

		static gboolean on_channel_io_as_available(GIOChannel* source,
				GIOCondition cond, Cmdex* self, channel_t type);


	private:


		// The object MUST be locked by the caller.
		void cleanup_members()
		{
			// ScopedLock locker(object_mutex_);
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


		mutable ErrorLockPolicy::Mutex& object_mutex_;  // reference to parent's member


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

		int stdout_make_str_as_available_;  // NOT affected by cleanup().
		int stderr_make_str_as_available_;  // NOT affected by cleanup().

		std::string str_stdout_;  // NOT affected by cleanup().
		std::string str_stderr_;  // NOT affected by cleanup().


		// signals

		// convert command exit status to message string
		exit_status_translator_func_t translator_func_;  // NOT affected by cleanup().
		void* translator_func_data_;

		// "command exited" signal callback.
		// NOTE: This will be called in separate thread!
		exited_callback_func_t exited_callback_;  // NOT affected by cleanup().
		void* exited_callback_data_;


};






#endif
