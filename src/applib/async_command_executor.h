/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/

#ifndef ASYNC_COMMAND_EXECUTOR_H
#define ASYNC_COMMAND_EXECUTOR_H

#include <glib.h>
#include <string>
#include <functional>
#include <chrono>

#include "hz/process_signal.h"  // hz::SIGNAL_*
#include "hz/error_holder.h"



/// Command executor.
/// There are two ways to detect when the command exits:
/// 1. Add a callback to signal_exited.
/// 2. Manually poll stopped_cleanup_needed().
/// In both cases, stopped_cleanup() must be called afterwards.
class AsyncCommandExecutor : public hz::ErrorHolder {
	public:

		/// A function that translates the exit error code into a readable string
		using exit_status_translator_func_t = std::function<std::string(int)>;

		/// A function that is called whenever a process exits.
		using exited_callback_func_t = std::function<void()>;


		/// Constructor
		explicit AsyncCommandExecutor(exited_callback_func_t exited_cb = nullptr);

		/// Deleted
		AsyncCommandExecutor(const AsyncCommandExecutor& other) = delete;

		/// Deleted
		AsyncCommandExecutor(AsyncCommandExecutor&& other) = delete;

		/// Deleted
		AsyncCommandExecutor& operator=(const AsyncCommandExecutor& other) = delete;

		/// Deleted
		AsyncCommandExecutor& operator=(AsyncCommandExecutor&& other) = delete;


		/// Destructor. Don't destroy this object unless the child has exited. It will leak stuff
		/// and possibly crash, etc. .
		~AsyncCommandExecutor() override;


		/// Set the command to execute. Call before execute().
		/// Note: The command and the arguments _must_ be shell-escaped.
		/// Use g_shell_quote() or Glib::shell_quote(). Note that each argument
		/// must be escaped separately.
		void set_command(const std::string& command_exec, const std::string& command_args);


		/// Launch the command.
		bool execute();


		/// Send SIGTERM(15) (terminate) to the child process.
		/// Use only after execute(). Using it after the command has exited has no effect.
		bool try_stop(hz::Signal sig = hz::Signal::Terminate);


		/// Send SIGKILL(9) (kill) to the child process. Same as
		/// try_stop(hz::SIGNAL_SIGKILL).
		/// Note that SIGKILL cannot be overridden in child process.
		bool try_kill();


		/// Set a timeout (since call to this function) to terminate the child process,
		/// kill it or both (use 0 to ignore the parameter).
		/// The timeouts will be unset automatically when the command exits.
		/// This has an effect only if the command is running (after execute()).
		void set_stop_timeouts(std::chrono::milliseconds term_timeout_msec,
				std::chrono::milliseconds kill_timeout_msec);

		/// Unset the terminate / kill timeouts. This will stop the timeout counters.
		/// This has an effect only if the command is running (after execute()).
		void unset_stop_timeouts();

		/// If stopped_cleanup_needed() returned true, call this. The command
		/// should be exited by this time. Must be called before the next execute().
		void stopped_cleanup();


		/// Returns true if command has stopped.
		/// Call repeatedly in a waiting function, after execute().
		/// When it returns true, call stopped_cleanup().
		[[nodiscard]] bool stopped_cleanup_needed() const;


		/// Check if the process is running. Note that if this returns false, it doesn't mean that
		/// the io channels have been closed or that the data may be read safely. Poll
		/// stopped_cleanup_needed() instead.
		[[nodiscard]] bool is_running() const;



		/// Call this before execution. There is a race-like condition - when the command
		/// outputs something, the io channel reads it from fd to its buffer and the event
		/// source callback is called. If the command dies, the io channel callback reads
		/// the remaining data to the channel buffer.
		/// Since the event source callbacks (which read from the buffer and empty it)
		/// happen rather sporadically (from the glib loop), the buffer may not get read and
		/// emptied at all (before the command exits). This is why it's necessary to have
		/// a buffer size which potentially can hold _all_ the command output.
		/// A way to fight this is to increase event source priority (which may not help).
		/// Another way is to delay the command exit so that the event source callback
		/// catches on and reads the buffer.
		// Use 0 to ignore the parameter. Call this before execute().
		void set_buffer_sizes(gsize stdout_buffer_size = 0, gsize stderr_buffer_size = 0);



		/// If stdout_make_str_as_available_ is false, call this after stopped_cleanup(),
		/// before next execute(). If it's true, you may call this before the command has
		/// stopped, but it will decrease performance significantly.
		[[nodiscard]] std::string get_stdout_str(bool clear_existing = false);


		/// See notes for \ref get_stdout_str().
		[[nodiscard]] std::string get_stderr_str(bool clear_existing = false);


		/// Return execution time, in seconds. Call this after execute().
		[[maybe_unused]] double get_execution_time_sec();


		/// Set exit status translator callback, disconnecting the old one.
		/// Call only before execute().
		void set_exit_status_translator(exit_status_translator_func_t func);


		/// Set exit notifier callback, disconnecting the old one.
		/// You can poll stopped_cleanup_needed() instead of using this function.
		void set_exited_callback(exited_callback_func_t func);



		// these are sort of private

		/// Channel type, for passing to callbacks
		enum class Channel {
			StandardOutput,
			StandardError
		};


		// Callbacks (Note: These are called by the real callbacks)

		/// Child watch handler
		static void on_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data);

		/// Channel I/O handler
		static gboolean on_channel_io(GIOChannel* channel, GIOCondition cond, AsyncCommandExecutor* self, Channel channel_type);


	private:


		/// Clean up the member variables and shut down the channels if needed.
		void cleanup_members();



		// default command and its args. std::strings, not ustrings.
		std::string command_exec_;  /// Binary name to execute. NOT affected by cleanup_members().
		std::string command_args_;  /// Arguments that always go with the binary. NOT affected by cleanup_members().


		bool running_ = false;  ///< If true, the child process is running now. NOT affected by cleanup_members().
		int kill_signal_sent_ = 0;  ///< If non-zero, the process has been sent this signal to terminate
		bool child_watch_handler_called_ = false;  ///< true after child_watch_handler callback, before stopped_cleanup().

		GPid pid_ = 0;  ///< Process ID. int in Unix, pointer in win32
		int waitpid_status_ = 0;  ///< After the command is stopped, before cleanup, this will be available (waitpid() status).


		GTimer* timer_ = nullptr;  ///< Keeps track of elapsed time since command execution. Value is not used by this class, but may be handy.

		guint event_source_id_term = 0;  ///< Timeout event source ID for SIGTERM.
		guint event_source_id_kill = 0;  ///< Timeout event source ID for SIGKILL.


		int fd_stdout_ = 0;  ///< stdout file descriptor
		int fd_stderr_ = 0;  ///< stderr file descriptor

		GIOChannel* channel_stdout_ = nullptr;  ///< stdout channel
		GIOChannel* channel_stderr_ = nullptr;  ///< stderr channel

		gsize channel_stdout_buffer_size_ = 100UL * 1024UL;  ///< stdout channel buffer size. NOT affected by cleanup_members(). 100K.
		gsize channel_stderr_buffer_size_ = 10UL * 1024UL;  ///< stderr channel buffer size. NOT affected by cleanup_members(). 10K.

		guint event_source_id_stdout_ = 0;  ///< IO watcher event source ID for stdout
		guint event_source_id_stderr_ = 0;  ///< IO watcher event source ID for stderr

		std::string str_stdout_;  ///< stdout data read during execution. NOT affected by cleanup_members().
		std::string str_stderr_;  ///< stderr data read during execution. NOT affected by cleanup_members().


		// signals

		// convert command exit status to message string
		exit_status_translator_func_t translator_func_{ };  ///< Exit status translator function. NOT affected by cleanup_members().

		// "command exited" signal callback.
		exited_callback_func_t exited_callback_{ };  ///< Exit notifier function. NOT affected by cleanup_members().

};






#endif
