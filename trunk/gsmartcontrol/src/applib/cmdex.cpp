/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <sys/types.h>
#include <cerrno>  // errno (not std::errno, it may be a macro)

#ifdef _WIN32
// 	#include <io.h>  // close()
#else
	#include <sys/wait.h>  // waitpid()'s W* macros
// 	#include <unistd.h>  // close()
#endif

#include "hz/process_signal.h"  // hz::process_signal_send, win32's W*
#include "hz/tls.h"
#include "hz/tls_policy_glib.h"
#include "hz/debug.h"
#include "hz/string_num.h"  // hz::number_to_string()
#include "hz/env_tools.h"  // hz::ScopedEnv
#include "hz/scoped_ptr.h"

#include "cmdex.h"


using hz::Error;
using hz::ErrorLevel;




// this is needed because these callbacks are called by glib.
extern "C" {


	// callbacks


	inline void cmdex_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data)
	{
		Cmdex::on_child_watch_handler(arg_pid, waitpid_status, data);
	}


	inline gboolean cmdex_on_channel_io_stdout(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return Cmdex::on_channel_io(source, cond, static_cast<Cmdex*>(data), Cmdex::channel_type_stdout);
	}

	inline gboolean cmdex_on_channel_io_stderr(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return Cmdex::on_channel_io(source, cond, static_cast<Cmdex*>(data), Cmdex::channel_type_stderr);
	}



	inline gboolean cmdex_on_term_timeout(gpointer data)
	{
		DBG_FUNCTION_ENTER_MSG;
		Cmdex* self = static_cast<Cmdex*>(data);
		self->try_stop(hz::SIGNAL_SIGTERM);
		return false;  // one-time call
	}


	inline gboolean cmdex_on_kill_timeout(gpointer data)
	{
		DBG_FUNCTION_ENTER_MSG;
		Cmdex* self = static_cast<Cmdex*>(data);
		self->try_stop(hz::SIGNAL_SIGTERM);
		return false;  // one-time call
	}



}  // extern "C"





bool Cmdex::execute()
{
	if (this->running_ || this->stopped_cleanup_needed())
		return false;

	cleanup_members();
	clear_errors();
	str_stdout_.clear();
	str_stderr_.clear();


	std::string cmd = command_exec_ + " " + command_args_;


	// Make command vector

	hz::scoped_ptr<gchar*> argvp(0, g_strfreev);  // args vector

	{
		int argcp = 0;  // number of args
		hz::scoped_ptr<GError> shell_error(0, g_error_free);
		if (!g_shell_parse_argv(cmd.c_str(), &argcp, &argvp.get_ref(), &shell_error.get_ref())) {
			push_error(Error<void>("gshell", ErrorLevel::error, shell_error->message), false);
			return false;
		}
	}


	// Set the locale for a child to Classic - otherwise it may mangle the output.
	// TODO: make this controllable.
	bool change_lang = true;
	#ifdef _WIN32
		// LANG is posix-only, so it has no effect on win32.
		// Unfortunately, I was unable to find a way to execute a child with a different
		// locale in win32. Locale seems to be non-inheritable, so setting it here won't help.
		change_lang = false;
	#endif

	hz::ScopedEnv lang_env("LANG", "C", change_lang);


	debug_out_info("app", DBG_FUNC_MSG << "Executing \"" << cmd << "\".\n");
/*
	if (argvp) {
		debug_out_dump("app", DBG_FUNC_MSG << "Dumping argvp:\n");
		gchar** elem = argvp.get();
		while (*elem) {
			debug_out_dump("app", *elem << "\n");
			++elem;
		}
	}
*/

	// Execute the command

	hz::scoped_ptr<gchar> curr_dir(g_get_current_dir(), g_free);
	hz::scoped_ptr<GError> spawn_error(0, g_error_free);

/*
#if defined APP_CMDEX_USE_SYNC && APP_CMDEX_USE_SYNC

	hz::scoped_ptr<gchar*> stdout_str(0, g_free);
	hz::scoped_ptr<gchar*> stderr_str(0, g_free);
	gint exit_status = 0;

	g_timer_start(timer_);  // start the timer

	if (!g_spawn_sync(curr_dir.get(), argvp.get(), NULL,
			GSpawnFlags(G_SPAWN_SEARCH_PATH),
			NULL, NULL,  // child setup function
			&stdout_str.get_ref(), &stderr_str.get_ref(), &exit_status, &spawn_error.get_ref()))
	{
		// no data is returned to &-parameters on error.
		push_error(Error<void>("gspawn", ErrorLevel::error, spawn_error->message), false);
		return false;
	}

#else // async way:
*/
	if (!g_spawn_async_with_pipes(curr_dir.get(), argvp.get(), NULL,
			GSpawnFlags(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
			NULL, NULL,  // child setup function
			&this->pid_, 0, &fd_stdout_, &fd_stderr_, &spawn_error.get_ref()))
	{
		// no data is returned to &-parameters on error.
		push_error(Error<void>("gspawn", ErrorLevel::error, spawn_error->message), false);
		return false;
	}

	g_timer_start(timer_);  // start the timer

// #endif


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
	int channel_flags = ~G_IO_FLAG_NONBLOCK;

	// Note about GError's here:
	// What do we do? The command is already running, so let's ignore these
	// errors - it's better to get a slightly mangled buffer than to abort the
	// command in the mid-run.
	if (channel_stdout_) {
		// Since we invoke shutdown() manually before unref(), this would cause
		// a double-shutdown.
		// g_io_channel_set_close_on_unref(channel_stdout_, true);  // close() on fd
		g_io_channel_set_encoding(channel_stdout_, NULL, 0);  // binary IO
		g_io_channel_set_flags(channel_stdout_, GIOFlags(g_io_channel_get_flags(channel_stdout_) & channel_flags), 0);
		g_io_channel_set_buffer_size(channel_stdout_, channel_stdout_buffer_size_);
	}
	if (channel_stderr_) {
		// g_io_channel_set_close_on_unref(channel_stderr_, true);  // close() on fd
		g_io_channel_set_encoding(channel_stderr_, NULL, 0);  // binary IO
		g_io_channel_set_flags(channel_stderr_, GIOFlags(g_io_channel_get_flags(channel_stderr_) & channel_flags), 0);
		g_io_channel_set_buffer_size(channel_stderr_, channel_stderr_buffer_size_);
	}


	GIOCondition cond = GIOCondition(G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL);
	// Channel reader callback must be called before other stuff so that the loss is minimal.
	gint io_priority = G_PRIORITY_HIGH;

	this->event_source_id_stdout_ = g_io_add_watch_full(channel_stdout_, io_priority, cond,
			&cmdex_on_channel_io_stdout, this, NULL);

	this->event_source_id_stderr_ = g_io_add_watch_full(channel_stderr_, io_priority, cond,
			&cmdex_on_channel_io_stderr, this, NULL);


	// If using SPAWN_DO_NOT_REAP_CHILD, this is needed to avoid zombies.
	// Note: Do NOT use glibmm slot, it doesn't work here.
	// (the child stops being a zombie as soon as wait*() exits and this handler is called).
	g_child_watch_add(this->pid_, &cmdex_child_watch_handler, this);


	this->running_ = true;  // the process is running now.

	return true;
}




// send SIGTERM(15) (terminate)
bool Cmdex::try_stop(hz::signal_t sig)
{
	if (!this->running_ || this->pid_ <= 0)
		return false;

	// other variants: SIGHUP(1) (terminal closed), SIGINT(2) (Ctrl-C),
	// SIGKILL(9) (kill).
	// Note that SIGKILL cannot be trapped by any process.

	if (process_signal_send(this->pid_, sig) == 0) {  // success
		this->kill_signal_sent_ = static_cast<int>(sig);  // just the number to compare later.
		return true;  // the rest is done by a handler
	}

	// Possible: EPERM (no permissions), ESRCH (no such process, or zombie)
	push_error(Error<int>("errno", ErrorLevel::error, errno), false);

	return false;
}




void Cmdex::set_stop_timeouts(int term_timeout_msec, int kill_timeout_msec)
{
	DBG_ASSERT(term_timeout_msec == 0 || kill_timeout_msec == 0 || kill_timeout_msec > term_timeout_msec);

	if (!this->running_)  // process not running
		return;

	unset_stop_timeouts();

	if (term_timeout_msec != 0)
		event_source_id_term = g_timeout_add(term_timeout_msec, &cmdex_on_term_timeout, this);

	if (kill_timeout_msec != 0)
		event_source_id_kill = g_timeout_add(kill_timeout_msec, &cmdex_on_kill_timeout, this);
}




void Cmdex::unset_stop_timeouts()
{
	if (event_source_id_term) {
		GSource* source_term = g_main_context_find_source_by_id(NULL, event_source_id_term);
		if (source_term)
			g_source_destroy(source_term);
		event_source_id_term = 0;
	}

	if (event_source_id_kill) {
		GSource* source_kill = g_main_context_find_source_by_id(NULL, event_source_id_kill);
		if (source_kill)
			g_source_destroy(source_kill);
		event_source_id_kill = 0;
	}
}



// executed in main thread, manually by the caller.
void Cmdex::stopped_cleanup()
{
// 	DBG_FUNCTION_ENTER_MSG;

	if (this->running_ || !this->stopped_cleanup_needed())  // huh?
		return;

	// remove stop timeout callbacks
	unset_stop_timeouts();

	// various statuses (see waitpid (2)):
	if (WIFEXITED(waitpid_status_)) {  // exited normally
		int exit_status = WEXITSTATUS(waitpid_status_);

		if (exit_status != 0) {
			// translate the exit_code into a message
			std::string msg = (translator_func_ ? translator_func_(exit_status, translator_func_data_)
					: "[no translator function, exit code: " + hz::number_to_string(exit_status));
			push_error(Error<int>("exit", ErrorLevel::warn, exit_status, msg), false);
		}

	} else {
		if (WIFSIGNALED(waitpid_status_)) {  // exited by signal
			int sig_num = WTERMSIG(waitpid_status_);

			// If it's not our signal, treat as error.
			// Note: they will never match under win32
			if (sig_num != this->kill_signal_sent_) {
				push_error(Error<int>("signal", ErrorLevel::error, sig_num), false);
			} else {  // it's our signal, treat as warning
				push_error(Error<int>("signal", ErrorLevel::warn, sig_num), false);
			}
		}
	}

	g_spawn_close_pid(this->pid_);  // needed to avoid zombies

	cleanup_members();

	this->running_ = false;
}





// Called when child exits
void Cmdex::on_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data)
{
// 	DBG_FUNCTION_ENTER_MSG;
	Cmdex* self = static_cast<Cmdex*>(data);

	g_timer_stop(self->timer_);  // stop the timer

	self->waitpid_status_ = waitpid_status;
	self->child_watch_handler_called_ = true;
	self->running_ = false;  // process is not running anymore

	// Remove fd IO callbacks. They may actually be removed already (note sure about this).
	// This will force calling the iochannel callback (they may not be called
	// otherwise at all if there was no output).
	if (self->event_source_id_stdout_) {
		GSource* source_stdout = g_main_context_find_source_by_id(NULL, self->event_source_id_stdout_);
		if (source_stdout)
			g_source_destroy(source_stdout);
	}

	if (self->event_source_id_stderr_) {
		GSource* source_stderr = g_main_context_find_source_by_id(NULL, self->event_source_id_stderr_);
		if (source_stderr)
			g_source_destroy(source_stderr);
	}

	// Close std pipes.
	// The channel closes them now.
// 	close(self->fd_stdout_);
// 	close(self->fd_stderr_);

	if (self->exited_callback_)
		self->exited_callback_(self->exited_callback_data_);

// 	DBG_FUNCTION_EXIT_MSG;
}





gboolean Cmdex::on_channel_io(GIOChannel* source,
		GIOCondition cond, Cmdex* self, channel_t type)
{
// 	debug_out_dump("app", "Cmdex::on_channel_io("
// 			<< (type == channel_type_stdout ? "STDOUT" : "STDERR") << ") " << int(cond) << "\n");

	bool continue_events = true;
	if ((cond & G_IO_ERR) || (cond & G_IO_HUP) || (cond & G_IO_NVAL)) {
		continue_events = false;  // there'll be no more data
	}

	DBG_ASSERT(type == channel_type_stdout || type == channel_type_stderr);

// 	const gsize count = 4 * 1024;
	// read the bytes one by one. without this, a buffered iochannel hangs while waiting for data.
	// we don't use unbuffered iochannels - they may lose data on program exit.
	const gsize count = 1;
	gchar buf[count] = {0};

	GIOChannel* channel = 0;
	std::string* output_str = 0;
	if (type == channel_type_stdout) {
		channel = self->channel_stdout_;
		output_str = &self->str_stdout_;
	} else if (type == channel_type_stderr) {
		channel = self->channel_stderr_;
		output_str = &self->str_stderr_;
	}


	// while there's anything to read, read it
	do {
		hz::scoped_ptr<GError> channel_error(0, g_error_free);
		gsize bytes_read = 0;
		GIOStatus status = g_io_channel_read_chars(channel, buf, count, &bytes_read, &channel_error.get_ref());
		if (bytes_read)
			output_str->append(buf, bytes_read);

		if (channel_error) {
			self->push_error(Error<void>("giochannel", ErrorLevel::error, channel_error->message), false);
			break;  // stop on next invocation (is this correct?)
		}

		// IO_STATUS_NORMAL and IO_STATUS_AGAIN (resource unavailable) are continuable.
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF) {
			continue_events = false;
			break;
		}
	} while (g_io_channel_get_buffer_condition(channel) & G_IO_IN);

// 	DBG_FUNCTION_EXIT_MSG;

	// false if the source should be removed, true otherwise.
	return continue_events;
}







