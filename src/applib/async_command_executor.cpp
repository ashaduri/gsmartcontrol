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

#include <string>
#include <sys/types.h>
#include <cerrno>  // errno (not std::errno, it may be a macro)
#include <array>

#ifdef _WIN32
// 	#include <io.h>  // close()
#else
	#include <sys/wait.h>  // waitpid()'s W* macros
// 	#include <unistd.h>  // close()
#endif

#include "hz/process_signal.h"  // hz::process_signal_send, win32's W*
#include "hz/debug.h"
#include "hz/fs.h"

#include "async_command_executor.h"
#include "build_config.h"


using hz::Error;
using hz::ErrorLevel;




// this is needed because these callbacks are called by glib.
extern "C" {


	// callbacks

	/// Child process watcher callback
	inline void cmdex_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data)
	{
		AsyncCommandExecutor::on_child_watch_handler(arg_pid, waitpid_status, data);
	}


	/// Child process stdout handler callback
	inline gboolean cmdex_on_channel_io_stdout(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return AsyncCommandExecutor::on_channel_io(source, cond, static_cast<AsyncCommandExecutor*>(data), AsyncCommandExecutor::Channel::StandardOutput);
	}


	/// Child process stderr handler callback
	inline gboolean cmdex_on_channel_io_stderr(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return AsyncCommandExecutor::on_channel_io(source, cond, static_cast<AsyncCommandExecutor*>(data), AsyncCommandExecutor::Channel::StandardError);
	}


	/// Child process termination timeout handler
	inline gboolean cmdex_on_term_timeout(gpointer data)
	{
		DBG_FUNCTION_ENTER_MSG;
		auto* self = static_cast<AsyncCommandExecutor*>(data);
		self->try_stop(hz::Signal::Terminate);
		return FALSE;  // one-time call
	}


	/// Child process kill timeout handler
	inline gboolean cmdex_on_kill_timeout(gpointer data)
	{
		DBG_FUNCTION_ENTER_MSG;
		auto* self = static_cast<AsyncCommandExecutor*>(data);
		self->try_stop(hz::Signal::Kill);
		return FALSE;  // one-time call
	}



}  // extern "C"




AsyncCommandExecutor::AsyncCommandExecutor(AsyncCommandExecutor::exited_callback_func_t exited_cb)
		: timer_(g_timer_new()),
		exited_callback_(std::move(exited_cb))
{ }



AsyncCommandExecutor::~AsyncCommandExecutor()
{
	// This will help if object is destroyed after the command has exited, but before
	// stopped_cleanup() has been called.
	stopped_cleanup();

	g_timer_destroy(timer_);

	// no need to destroy the channels - stopped_cleanup() calls
	// cleanup_members(), which deletes them.
}



void AsyncCommandExecutor::set_command(std::string command_exec, std::vector<std::string> command_args)
{
	command_exec_ = std::move(command_exec);
	command_args_ = std::move(command_args);
}



bool AsyncCommandExecutor::execute()
{
	DBG_FUNCTION_ENTER_MSG;
	if (this->running_ || this->stopped_cleanup_needed()) {
		return false;
	}

	cleanup_members();
	clear_errors();
	str_stdout_.clear();
	str_stderr_.clear();


	// Set the locale for a child to Classic - otherwise it may mangle the output.
	// TODO: Disable this for JSON format.
	bool change_lang = true;
	if constexpr(BuildEnv::is_kernel_family_windows()) {
		// LANG is posix-only, so it has no effect on win32.
		// Unfortunately, I was unable to find a way to execute a child with a different
		// locale in win32. Locale seems to be non-inheritable, so setting it here won't help.
		change_lang = false;
	}

	std::unique_ptr<gchar*, decltype(&g_strfreev)> child_env(g_get_environ(), &g_strfreev);
	if (change_lang) {
		child_env.reset(g_environ_setenv(child_env.release(), "LC_ALL", "C", TRUE));
	}
	const std::vector<std::string> envp = Glib::ArrayHandler<std::string>::array_to_vector(child_env.release(),
			Glib::OWNERSHIP_DEEP);

	// Set the current directory to application directory so CWD does not interfere with finding binaries.
	auto current_path = hz::fs::current_path();
	bool path_changed = false;
	if (auto app_dir = hz::fs_get_application_dir(); !app_dir.empty()) {
		std::error_code ec;
		hz::fs::current_path(app_dir, ec);
		path_changed = !ec;
	}

	debug_out_info("app", DBG_FUNC_MSG << "Executing \"" << command_exec_ << "\".\n");
	debug_out_info("app", DBG_FUNC_MSG << "Arguments:\n");
	for (const auto& arg : command_args_) {
		debug_out_info("app", "  " << arg << "\n");
	}

	std::vector<std::string> argvp = {command_exec_};
	argvp.insert(argvp.end(), command_args_.begin(), command_args_.end());

	// Execute the command
	try {
		Glib::spawn_async_with_pipes(Glib::get_current_dir(), argvp, envp,
				Glib::SpawnFlags::SPAWN_SEARCH_PATH | Glib::SpawnFlags::SPAWN_DO_NOT_REAP_CHILD,
				Glib::SlotSpawnChildSetup(),
				&this->pid_, nullptr, &fd_stdout_, &fd_stderr_);
	}
	catch(Glib::SpawnError& e) {
		// no data is returned to &-parameters on error.
		push_error(Error<void>("gspawn", ErrorLevel::Error, e.what()));
		// Restore CWD
		if (path_changed) {
			std::error_code dummy_ec;
			hz::fs::current_path(current_path, dummy_ec);
		}
		return false;
	}

	// Restore CWD
	if (path_changed) {
		std::error_code dummy_ec;
		hz::fs::current_path(current_path, dummy_ec);
	}

	g_timer_start(timer_);  // start the timer


	#ifdef _WIN32
		channel_stdout_ = g_io_channel_win32_new_fd(fd_stdout_);
		channel_stderr_ = g_io_channel_win32_new_fd(fd_stderr_);
	#else
		channel_stdout_ = g_io_channel_unix_new(fd_stdout_);
		channel_stderr_ = g_io_channel_unix_new(fd_stderr_);
	#endif

	// The internal encoding is always UTF8. To read command output correctly, use
	// "" for binary data, or set io encoding to current locale.
	// If using locales, call g_locale_to_utf8() or g_convert() afterwards.

	// blocking writes if the pipe is full helps for small-pipe systems (see man 7 pipe).
	const int channel_flags = ~G_IO_FLAG_NONBLOCK;

	// Note about GError's here:
	// What do we do? The command is already running, so let's ignore these
	// errors - it's better to get a slightly mangled buffer than to abort the
	// command in the mid-run.
	if (channel_stdout_) {
		// Since we invoke shutdown() manually before unref(), this would cause
		// a double-shutdown.
		// g_io_channel_set_close_on_unref(channel_stdout_, true);  // close() on fd
		g_io_channel_set_encoding(channel_stdout_, nullptr, nullptr);  // binary IO
		g_io_channel_set_flags(channel_stdout_, GIOFlags(g_io_channel_get_flags(channel_stdout_) & channel_flags), nullptr);
		g_io_channel_set_buffer_size(channel_stdout_, channel_stdout_buffer_size_);
	}
	if (channel_stderr_) {
		// g_io_channel_set_close_on_unref(channel_stderr_, true);  // close() on fd
		g_io_channel_set_encoding(channel_stderr_, nullptr, nullptr);  // binary IO
		g_io_channel_set_flags(channel_stderr_, GIOFlags(g_io_channel_get_flags(channel_stderr_) & channel_flags), nullptr);
		g_io_channel_set_buffer_size(channel_stderr_, channel_stderr_buffer_size_);
	}


	auto cond = GIOCondition(G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL);
	// Channel reader callback must be called before other stuff so that the loss is minimal.
	const int io_priority = G_PRIORITY_HIGH;

	this->event_source_id_stdout_ = g_io_add_watch_full(channel_stdout_, io_priority, cond,
			&cmdex_on_channel_io_stdout, this, nullptr);
// 	g_io_channel_unref(channel_stdout_);  // g_io_add_watch_full() holds its own reference

	this->event_source_id_stderr_ = g_io_add_watch_full(channel_stderr_, io_priority, cond,
			&cmdex_on_channel_io_stderr, this, nullptr);
// 	g_io_channel_unref(channel_stderr_);  // g_io_add_watch_full() holds its own reference


	// If using SPAWN_DO_NOT_REAP_CHILD, this is needed to avoid zombies.
	// Note: Do NOT use glibmm slot, it doesn't work here.
	// (the child stops being a zombie as soon as wait*() exits and this handler is called).
	g_child_watch_add(this->pid_, &cmdex_child_watch_handler, this);


	this->running_ = true;  // the process is running now.

	DBG_FUNCTION_EXIT_MSG;
	return true;
}



bool AsyncCommandExecutor::try_stop(hz::Signal sig)
{
	DBG_FUNCTION_ENTER_MSG;
	if (!this->running_ || this->pid_ == 0)
		return false;

	// other variants: SIGHUP(1) (terminal closed), SIGINT(2) (Ctrl-C),
	// SIGKILL(9) (kill).
	// Note that SIGKILL cannot be trapped by any process.

	if (process_signal_send(this->pid_, sig) == 0) {  // success
		this->kill_signal_sent_ = static_cast<int>(sig);  // just the number to compare later.
		return true;  // the rest is done by a handler
	}

	// Possible: EPERM (no permissions), ESRCH (no such process, or zombie)
	push_error(Error<int>("errno", ErrorLevel::Error, errno));

	DBG_FUNCTION_EXIT_MSG;
	return false;
}



bool AsyncCommandExecutor::try_kill()
{
	DBG_TRACE_POINT_AUTO;
	return try_stop(hz::Signal::Kill);
}



void AsyncCommandExecutor::set_stop_timeouts(std::chrono::milliseconds term_timeout_msec, std::chrono::milliseconds kill_timeout_msec)
{
	DBG_FUNCTION_ENTER_MSG;
	DBG_ASSERT(term_timeout_msec.count() == 0 || kill_timeout_msec.count() == 0 || (kill_timeout_msec > term_timeout_msec));

	if (!this->running_)  // process not running
		return;

	unset_stop_timeouts();

	if (term_timeout_msec.count() != 0)
		event_source_id_term = g_timeout_add(guint(term_timeout_msec.count()), &cmdex_on_term_timeout, this);

	if (kill_timeout_msec.count() != 0)
		event_source_id_kill = g_timeout_add(guint(kill_timeout_msec.count()), &cmdex_on_kill_timeout, this);

	DBG_FUNCTION_EXIT_MSG;
}



void AsyncCommandExecutor::unset_stop_timeouts()
{
	DBG_FUNCTION_ENTER_MSG;
	if (event_source_id_term != 0) {
		GSource* source_term = g_main_context_find_source_by_id(nullptr, event_source_id_term);
		if (source_term)
			g_source_destroy(source_term);
		event_source_id_term = 0;
	}

	if (event_source_id_kill != 0) {
		GSource* source_kill = g_main_context_find_source_by_id(nullptr, event_source_id_kill);
		if (source_kill)
			g_source_destroy(source_kill);
		event_source_id_kill = 0;
	}
	DBG_FUNCTION_EXIT_MSG;
}



void AsyncCommandExecutor::stopped_cleanup()
{
	DBG_FUNCTION_ENTER_MSG;
	if (this->running_ || !this->stopped_cleanup_needed())  // huh?
		return;

	// remove stop timeout callbacks
	unset_stop_timeouts();

	// various statuses (see waitpid (2)):
	if (WIFEXITED(waitpid_status_)) {  // exited normally
		const int exit_status = WEXITSTATUS(waitpid_status_);

		if (exit_status != 0) {
			// translate the exit_code into a message
			const std::string msg = (translator_func_ ? translator_func_(exit_status)
					: "[no translator function, exit code: " + std::to_string(exit_status));
			push_error(Error<int>("exit", ErrorLevel::Warn, exit_status, msg));
		}

	} else {
		if (WIFSIGNALED(waitpid_status_)) {  // exited by signal
			const int sig_num = WTERMSIG(waitpid_status_);

			// If it's not our signal, treat as error.
			// Note: they will never match under win32
			if (sig_num != this->kill_signal_sent_) {
				push_error(Error<int>("signal", ErrorLevel::Error, sig_num));
			} else {  // it's our signal, treat as warning
				push_error(Error<int>("signal", ErrorLevel::Warn, sig_num));
			}
		}
	}

	g_spawn_close_pid(this->pid_);  // needed to avoid zombies

	cleanup_members();

	this->running_ = false;
	DBG_FUNCTION_EXIT_MSG;
}



void AsyncCommandExecutor::on_child_watch_handler([[maybe_unused]] GPid arg_pid, int waitpid_status, gpointer data)
{
// 	DBG_FUNCTION_ENTER_MSG;
	auto* self = static_cast<AsyncCommandExecutor*>(data);

	g_timer_stop(self->timer_);  // stop the timer

	self->waitpid_status_ = waitpid_status;
	self->child_watch_handler_called_ = true;
	self->running_ = false;  // process is not running anymore

	// These are needed because Windows doesn't read the remaining data otherwise.
	g_io_channel_flush(self->channel_stdout_, nullptr);
	on_channel_io(self->channel_stdout_, GIOCondition(0), self, Channel::StandardOutput);

	g_io_channel_flush(self->channel_stderr_, nullptr);
	on_channel_io(self->channel_stderr_, GIOCondition(0), self, Channel::StandardError);

	if (self->channel_stdout_) {
		g_io_channel_shutdown(self->channel_stdout_, FALSE, nullptr);
		g_io_channel_unref(self->channel_stdout_);
		self->channel_stdout_ = nullptr;
	}

	if (self->channel_stderr_) {
		g_io_channel_shutdown(self->channel_stderr_, FALSE, nullptr);
		g_io_channel_unref(self->channel_stderr_);
		self->channel_stderr_ = nullptr;
	}

	// Remove fd IO callbacks. They may actually be removed already (note sure about this).
	// This will force calling the iochannel callback (they may not be called
	// otherwise at all if there was no output).
	if (self->event_source_id_stdout_ != 0) {
		GSource* source_stdout = g_main_context_find_source_by_id(nullptr, self->event_source_id_stdout_);
		if (source_stdout)
			g_source_destroy(source_stdout);
	}

	if (self->event_source_id_stderr_ != 0) {
		GSource* source_stderr = g_main_context_find_source_by_id(nullptr, self->event_source_id_stderr_);
		if (source_stderr)
			g_source_destroy(source_stderr);
	}

	// Close std pipes.
	// The channel closes them now.
// 	close(self->fd_stdout_);
// 	close(self->fd_stderr_);

	if (self->exited_callback_)
		self->exited_callback_();

// 	DBG_FUNCTION_EXIT_MSG;
}



gboolean AsyncCommandExecutor::on_channel_io(GIOChannel* channel,
		GIOCondition cond, AsyncCommandExecutor* self, Channel channel_type)
{
// 	DBG_FUNCTION_ENTER_MSG;
// 	debug_out_dump("app", "AsyncCommandExecutor::on_channel_io("
// 			<< (type == Channel::standard_output ? "STDOUT" : "STDERR") << ") " << int(cond) << "\n");

	bool continue_events = true;
	if (bool(cond & G_IO_ERR) || bool(cond & G_IO_HUP) || bool(cond & G_IO_NVAL)) {
		continue_events = false;  // there'll be no more data
	}

	DBG_ASSERT_RETURN(channel_type == Channel::StandardOutput || channel_type == Channel::StandardError, false);

// 	const gsize count = 4 * 1024;
	// read the bytes one by one. without this, a buffered iochannel hangs while waiting for data.
	// we don't use unbuffered iochannels - they may lose data on program exit.
	constexpr gsize count = 1;
	std::array<gchar, count> buf = {0};

	std::string* output_str = nullptr;
	if (channel_type == Channel::StandardOutput) {
		output_str = &self->str_stdout_;
	} else if (channel_type == Channel::StandardError) {
		output_str = &self->str_stderr_;
	}
	DBG_ASSERT_RETURN(output_str, false);


	// while there's anything to read, read it
	do {
		GError* channel_error = nullptr;
		gsize bytes_read = 0;
		const GIOStatus status = g_io_channel_read_chars(channel, buf.data(), count, &bytes_read, &channel_error);
		if (bytes_read != 0)
			output_str->append(buf.data(), bytes_read);

		if (channel_error) {
			self->push_error(Error<void>("giochannel", ErrorLevel::Error, channel_error->message));
			g_error_free(channel_error);
			break;  // stop on next invocation (is this correct?)
		}

		// IO_STATUS_NORMAL and IO_STATUS_AGAIN (resource unavailable) are continuable.
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF) {
			continue_events = false;
			break;
		}
	} while (bool(g_io_channel_get_buffer_condition(channel) & G_IO_IN));

// 	DBG_FUNCTION_EXIT_MSG;

	// false if the source should be removed, true otherwise.
	return gboolean(continue_events);
}



bool AsyncCommandExecutor::stopped_cleanup_needed() const
{
	return (child_watch_handler_called_);
}



bool AsyncCommandExecutor::is_running() const
{
	return running_;
}



void AsyncCommandExecutor::set_buffer_sizes(gsize stdout_buffer_size, gsize stderr_buffer_size)
{
	if (stdout_buffer_size > 0) {
		channel_stdout_buffer_size_ = stdout_buffer_size;  // 100K by default
	}
	if (stderr_buffer_size > 0) {
		channel_stderr_buffer_size_ = stderr_buffer_size;  // 10K by default
	}
}



std::string AsyncCommandExecutor::get_stdout_str(bool clear_existing)
{
	// debug_out_dump("app", str_stdout_);
	if (clear_existing) {
		std::string ret = str_stdout_;
		str_stdout_.clear();
		return ret;
	}
	return str_stdout_;
}



std::string AsyncCommandExecutor::get_stderr_str(bool clear_existing)
{
	if (clear_existing) {
		std::string ret = str_stderr_;
		str_stderr_.clear();
		return ret;
	}
	return str_stderr_;
}



double AsyncCommandExecutor::get_execution_time_sec()
{
	gulong microsec = 0;
	return g_timer_elapsed(timer_, &microsec);
}



void AsyncCommandExecutor::set_exit_status_translator(AsyncCommandExecutor::exit_status_translator_func_t func)
{
	translator_func_ = std::move(func);
}



void AsyncCommandExecutor::set_exited_callback(AsyncCommandExecutor::exited_callback_func_t func)
{
	exited_callback_ = std::move(func);
}



void AsyncCommandExecutor::cleanup_members()
{
	kill_signal_sent_ = 0;
	child_watch_handler_called_ = false;
	pid_ = 0;
	waitpid_status_ = 0;
	event_source_id_stdout_ = 0;
	event_source_id_stderr_ = 0;
	fd_stdout_ = 0;
	fd_stderr_ = 0;
}






/// @}
