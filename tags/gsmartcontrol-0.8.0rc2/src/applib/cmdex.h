/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_CMDEX_H
#define APP_CMDEX_H

#include <string>
#include <sigc++/sigc++.h>
#include <glibmm/iochannel.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>  // Timer
#include <glibmm/spawn.h>  // Pid
#include <glib.h>  // G*, g_*

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





// inheriting from sigc::trackable automatically disconnects this class's
// slots upon destruction.
class Cmdex : public hz::ErrorHolder<hz::SyncPolicyGlib>, public sigc::trackable {

	public:

		using hz::ErrorHolder<hz::SyncPolicyGlib>::ptr_error_list_t;  // auto-deleting container
		using hz::ErrorHolder<hz::SyncPolicyGlib>::error_list_t;

		typedef hz::ErrorHolder<hz::SyncPolicyGlib>::ErrorScopedLock ScopedLock;
		typedef hz::ErrorHolder<hz::SyncPolicyGlib>::ErrorLockPolicy LockPolicy;


		Cmdex(
			//const sigc::slot<Glib::ustring, int>& slot_exit_status_translator,
			const sigc::slot<void>& slot_exited = sigc::slot<void>()
			)
			: object_mutex_(error_object_mutex_),
			running(false), kill_signal_sent(0), child_watch_handler_called(false),
			pid(0), waitpid_status(0),
			event_source_id_term(0), event_source_id_kill(0),
			fd_stdout(0), fd_stderr(0),
			channel_stdout_buffer_size(100 * 1024), channel_stderr_buffer_size(10 * 1024),  // 100K and 10K
			event_source_id_stdout(0), event_source_id_stderr(0),
			stdout_make_str_as_available(false), stderr_make_str_as_available(true)
		{
			// signal_exit_status_translator.connect(slot_exit_status_translator);
			if (!slot_exited.empty())
				signal_exited.connect(slot_exited);
		}



		// Please don't destroy this object unless the child has exited. It will leak stuff
		// and possibly crash, etc... .
		~Cmdex()
		{
			// This will help if object is destroyed after the command has exited, but before
			// stopped_cleanup() has been called.
			stopped_cleanup();
		}


		// Call before execute.
		void set_command(const std::string& command_exec_, const std::string& command_args_)
		{
			command_exec = command_exec_;
			command_args = command_args_;
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
			return (child_watch_handler_called);
		}


		// The process is running. Note that if this returns false, it doesn't mean that
		// the io channels have been closed or that the data may be read safely. Poll
		// stopped_cleanup_needed() instead.
		bool is_running() const
		{
			ScopedLock locker(object_mutex_);
			return running;
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
				channel_stdout_buffer_size = stdout_buffer_size;  // 100K by default
			if (stderr_buffer_size)
				channel_stderr_buffer_size = stderr_buffer_size;  // 10K by default
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
			stdout_make_str_as_available = stdout_str_as_available;
			stderr_make_str_as_available = stderr_str_as_available;
		}



		// If stdout_make_str_as_available is false, call this after stopped_cleanup(),
		// before next execute(). If it's true, you may call this before the command has
		// stopped, but it will decrease performance significantly.
		std::string get_stdout_str(bool clear_existing = false)
		{
			ScopedLock locker(object_mutex_);
			// debug_out_dump("app", str_stdout);
			if (clear_existing) {
				std::string ret = str_stdout;
				str_stdout.clear();
				return ret;
			}
			return str_stdout;
		}


		// See notes on get_stdout_str().
		std::string get_stderr_str(bool clear_existing = false)
		{
			ScopedLock locker(object_mutex_);
			if (clear_existing) {
				std::string ret = str_stderr;
				str_stderr.clear();
				return ret;
			}
			return str_stderr;
		}


		// don't return sigc::connection - it's better for thread-safety this way.
		// this will disconnect earlier slots. Call before execute().
		void set_exit_status_translator(const sigc::slot<Glib::ustring, int>& slot_exit_status_translator)
		{
			ScopedLock locker(object_mutex_);
			signal_exit_status_translator.slots().erase(signal_exit_status_translator.slots().begin(), signal_exit_status_translator.slots().end());
			signal_exit_status_translator.connect(slot_exit_status_translator);
		}


		// Returns sigc::connection; you must use lock() / unlock() before modifying it.
		// NOTE: The slot will be called in non-main thread! (the call is enclosed
		// in lock/unlock).
		// Don't use this unless absolutely necessary, use stopped_cleanup_needed()
		// polling instead.
		sigc::connection connect_signal_exited(const sigc::slot<void>& slot_exited)
		{
			ScopedLock locker(object_mutex_);
			return signal_exited.connect(slot_exited);
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
			kill_signal_sent = 0;
			child_watch_handler_called = false;
			pid = 0;
			waitpid_status = 0;
			channel_stdout.clear();
			channel_stderr.clear();
			event_source_id_stdout = 0;
			event_source_id_stderr = 0;
			fd_stdout = 0;
			fd_stderr = 0;
		}


		mutable ErrorLockPolicy::Mutex& object_mutex_;  // reference to parent's member


		// default command and its args. std::strings, not ustrings.
		std::string command_exec;  // binary name. NOT affected by cleanup().
		std::string command_args;  // args that always go with binary. NOT affected by cleanup().


		bool running;  // child process is running now. NOT affected by cleanup().
		int kill_signal_sent;  // command has been sent this signal to terminate
		bool child_watch_handler_called;  // true after child_watch_handler callback, before stopped_cleanup().

		Glib::Pid pid;  // int in Unix, pointer in win32
		int waitpid_status;  // after the command is stopped, before cleanup, this will be available.


		Glib::Timer timer;  // keep track of elapsed time since command execution. not used by this class, but may be handy.

		int event_source_id_term;
		int event_source_id_kill;


		int fd_stdout;
		int fd_stderr;

		Glib::RefPtr<Glib::IOChannel> channel_stdout;
		Glib::RefPtr<Glib::IOChannel> channel_stderr;

		int channel_stdout_buffer_size;  // NOT affected by cleanup().
		int channel_stderr_buffer_size;  // NOT affected by cleanup().

		int event_source_id_stdout;
		int event_source_id_stderr;

		int stdout_make_str_as_available;  // NOT affected by cleanup().
		int stderr_make_str_as_available;  // NOT affected by cleanup().

		std::string str_stdout;  // NOT affected by cleanup().
		std::string str_stderr;  // NOT affected by cleanup().


		// signals

		// convert command exit status to message string
		sigc::signal<Glib::ustring, int> signal_exit_status_translator;  // NOT affected by cleanup().

		// command exited signal
		sigc::signal<void> signal_exited;  // NOT affected by cleanup().



};






#endif
