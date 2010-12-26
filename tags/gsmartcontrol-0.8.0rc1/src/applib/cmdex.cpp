/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <sys/types.h>
#include <cerrno>  // errno (not std::errno, it may be a macro)

#ifdef _WIN32
	#include <io.h>  // close()
#else
	#include <sys/wait.h>  // waitpid()'s W* macros
	#include <unistd.h>  // close()
#endif

#include <glibmm/shell.h>
#include <glibmm/miscutils.h>  // get_current_dir()
#include <glibmm/convert.h>

#include "hz/tls.h"
#include "hz/tls_policy_glib.h"
#include "hz/debug.h"

#include "cmdex.h"



using hz::Error;
using hz::ErrorLevel;




// this is needed because these callbacks are called by glib.
extern "C" {


	// callbacks


	// NOTE: These functions are called in a separate threads!

	inline void command_executor_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data)
	{
		Cmdex::on_child_watch_handler(arg_pid, waitpid_status, data);
	}


	inline gboolean command_executor_on_channel_io_stdout_as_available(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return Cmdex::on_channel_io_as_available(source, cond, static_cast<Cmdex*>(data), Cmdex::channel_type_stdout);
	}

	inline gboolean command_executor_on_channel_io_stderr_as_available(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return Cmdex::on_channel_io_as_available(source, cond, static_cast<Cmdex*>(data), Cmdex::channel_type_stderr);
	}


	inline gboolean command_executor_on_channel_io_stdout_buffered(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return Cmdex::on_channel_io_buffered(source, cond, static_cast<Cmdex*>(data), Cmdex::channel_type_stdout);
	}

	inline gboolean command_executor_on_channel_io_stderr_buffered(GIOChannel* source, GIOCondition cond, gpointer data)
	{
		return Cmdex::on_channel_io_buffered(source, cond, static_cast<Cmdex*>(data), Cmdex::channel_type_stderr);
	}


	inline void command_executor_on_buffered_source_destroy(gpointer data);



	// this is executed in another thread!
	inline gboolean command_executor_on_term_timeout(gpointer data)
	{
		DBG_FUNCTION_ENTER_MSG;
		Cmdex* self = static_cast<Cmdex*>(data);
		self->try_stop(hz::SIGNAL_SIGTERM);
		return false;  // one-time call
	}


	// this is executed in another thread!
	inline gboolean command_executor_on_kill_timeout(gpointer data)
	{
		DBG_FUNCTION_ENTER_MSG;
		Cmdex* self = static_cast<Cmdex*>(data);
		self->try_stop(hz::SIGNAL_SIGTERM);
		return false;  // one-time call
	}



}  // extern "C"





bool Cmdex::execute()
{
	ScopedLock locker(object_mutex_);

	if (this->running || this->stopped_cleanup_needed(false))
		return false;

	cleanup_members();
	clear_errors(false);  // don't lock
	str_stdout.clear();
	str_stderr.clear();


	std::string cmd = command_exec + " " + command_args;
	std::deque<std::string> child_argv;

	try {
		child_argv = Glib::shell_parse_argv(cmd);
	}
	catch (Glib::ShellError& e) {
		push_error(Error<Glib::ShellError>("gshell", ErrorLevel::error, e, e.what()), false);
		return false;
	}


	// Reset the locale - otherwise it may mangle the output.
	bool old_lang_set = false;
	std::string old_lang = Glib::getenv("LANG", old_lang_set);
	if (old_lang_set)
		Glib::setenv("LANG", "C");

	try {
		// execute the command
		debug_out_info("app", DBG_FUNC_MSG << "Executing \"" << cmd << "\".\n");

		Glib::spawn_async_with_pipes(Glib::get_current_dir(), child_argv,
				Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD,
				sigc::slot<void>(), &this->pid,
				0, &fd_stdout, &fd_stderr);
	}
	catch (Glib::SpawnError& e) {
		push_error(Error<Glib::SpawnError>("gspawn", ErrorLevel::error, e, e.what()), false);
		if (old_lang_set)
			Glib::setenv("LANG", old_lang);
		return false;
	}

	if (old_lang_set)
		Glib::setenv("LANG", old_lang);


	this->timer.start();  // start the timer


#ifdef _WIN32
	channel_stdout = Glib::IOChannel::create_from_win32_fd(fd_stdout);
	channel_stderr = Glib::IOChannel::create_from_win32_fd(fd_stderr);
#else
	channel_stdout = Glib::IOChannel::create_from_fd(fd_stdout);
	channel_stderr = Glib::IOChannel::create_from_fd(fd_stderr);
#endif

	// The internal encoding is always UTF8. To read command output correctly, use
	// "" for binary data, or set io encoding to current locale.
	// If using locales, call Glib::locale_to_utf8() or Glib::convert() afterwards.

	// std::string charset;
	// Glib::get_charset(charset);
	try {
		channel_stdout->set_encoding("");  // binary IO
		channel_stderr->set_encoding("");  // binary IO

		// blocking writes if the pipe is full helps for small-pipe systems (see man 7 pipe).
		Glib::IOFlags flags = ~Glib::IO_FLAG_NONBLOCK;
		channel_stdout->set_flags(flags);
		channel_stderr->set_flags(flags);

		channel_stdout->set_buffer_size(channel_stdout_buffer_size);
		channel_stderr->set_buffer_size(channel_stderr_buffer_size);
	}
	catch (Glib::IOChannelError& e) {
		// What do we do? The command is already running, so let's ignore these
		// errors - it's better to get a slightly mangled buffer than to abort the
		// command in the mid-run.
	}


	// Note: glib callbacks _are_ invoked in separate threads,
	// so proper locking is necessary.
	// This is different from gtk callbacks, which are directly invoked from main loop
	// in the same thread.

	// this doesn't really work
// 	Glib::signal_io().connect(sigc::bind(sigc::ptr_fun(&io_handler), this), channel_stdout, cond);

	GIOCondition cond = GIOCondition(G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL);
	gint io_thread_priority = G_PRIORITY_HIGH;  // channel reader callback thread must read enough so that the loss is minimal

	if (this->stdout_make_str_as_available) {
		this->event_source_id_stdout = g_io_add_watch_full(channel_stdout->gobj(), io_thread_priority, cond,
				&command_executor_on_channel_io_stdout_as_available, this, NULL);
	} else {
		this->event_source_id_stdout = g_io_add_watch_full(channel_stdout->gobj(), io_thread_priority, cond,
				&command_executor_on_channel_io_stdout_buffered, this, command_executor_on_buffered_source_destroy);
	}

	if (this->stderr_make_str_as_available) {
		this->event_source_id_stderr = g_io_add_watch_full(channel_stderr->gobj(), io_thread_priority, cond,
				&command_executor_on_channel_io_stderr_as_available, this, NULL);
	} else {
		this->event_source_id_stderr = g_io_add_watch_full(channel_stderr->gobj(), io_thread_priority, cond,
				&command_executor_on_channel_io_stderr_buffered, this, command_executor_on_buffered_source_destroy);
	}


	// If using SPAWN_DO_NOT_REAP_CHILD, this is needed to avoid zombies.
	// Note: Do NOT use glibmm slot, it doesn't work here.
	// The watch function is protected by mutex and won't be called before execute() exits.
	// This is AFTER the channels so that it doesn't get called before the channels are attached (is this needed?).
	// (the child stops being a zombie as soon as wait*() exits and this handler is called).
	g_child_watch_add(static_cast<GPid>(this->pid), &command_executor_child_watch_handler, this);



	this->running = true;  // process is running now.
	// actually, it may be already stopped, but the watcher function is blocked
	// until we exit, so the actions are serialized.

	return true;
}




// send SIGTERM(15) (terminate)
bool Cmdex::try_stop(hz::signal_t sig)
{
	ScopedLock locker(object_mutex_);

	if (!this->running || this->pid <= 0)
		return false;

	// other variants: SIGHUP(1) (terminal closed), SIGINT(2) (Ctrl-C),
	// SIGKILL(9) (kill).
	// Note that SIGKILL cannot be trapped by any process.

	if (process_signal_send(this->pid, sig) == 0) {  // success
		this->kill_signal_sent = static_cast<int>(sig);  // just the number to compare later.
		return true;  // the rest is done by a handler
	}

	// Possible: EPERM (no permissions), ESRCH (no such process, or zombie)
	push_error(Error<int>("errno", ErrorLevel::error, errno), false);

	return false;
}




void Cmdex::set_stop_timeouts(int term_timeout_msec, int kill_timeout_msec)
{
	DBG_ASSERT(term_timeout_msec == 0 || kill_timeout_msec == 0 || kill_timeout_msec > term_timeout_msec);
	ScopedLock locker(object_mutex_);  // member access / serialization lock

	if (!this->running)  // process not running
		return;

	unset_stop_timeouts(false);

	if (term_timeout_msec != 0)
		event_source_id_term = g_timeout_add(term_timeout_msec, &command_executor_on_term_timeout, this);

	if (kill_timeout_msec != 0)
		event_source_id_kill = g_timeout_add(kill_timeout_msec, &command_executor_on_kill_timeout, this);
}




void Cmdex::unset_stop_timeouts(bool do_lock)
{
	ScopedLock locker(object_mutex_, do_lock);  // member access / serialization lock

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
	ScopedLock locker(object_mutex_);  // member access / serialization lock

	if (this->running || !this->stopped_cleanup_needed(false))  // huh?
		return;

	// remove stop timeout callbacks
	unset_stop_timeouts(false);


	// various statuses (see waitpid (2)):
	if (WIFEXITED(waitpid_status)) {  // exited normally
		int exit_status = WEXITSTATUS(waitpid_status);

		if (exit_status != 0) {
			// translate the exit_code into a message
			Glib::ustring msg = signal_exit_status_translator.emit(exit_status);
			push_error(Error<int>("exit", ErrorLevel::warn, exit_status, msg), false);
		}

	} else {
		if (WIFSIGNALED(waitpid_status)) {  // exited by signal
			int sig_num = WTERMSIG(waitpid_status);

			// If it's not our signal, treat as error.
			// Note: they will never match under win32
			if (sig_num != this->kill_signal_sent) {
				push_error(Error<int>("signal", ErrorLevel::error, sig_num), false);
			} else {  // it's our signal, treat as warning
				push_error(Error<int>("signal", ErrorLevel::warn, sig_num), false);
			}
		}
	}

	Glib::spawn_close_pid(this->pid);  // needed to avoid zombies

	cleanup_members();

	this->running = false;
}





// NOTE: This function is called in a separate thread!
void Cmdex::on_child_watch_handler(GPid arg_pid, int waitpid_status, gpointer data)
{
// 	DBG_FUNCTION_ENTER_MSG;
	Cmdex* self = static_cast<Cmdex*>(data);

	self->lock();

	self->timer.stop();  // stop the timer

	// wait until the IO channels are closed
// 	while (!(self->event_source_stdout_closed && self->event_source_stderr_closed)) {
// 		self->unlock();
// 		Glib::usleep(75*1000);  // 75ms
// 		self->lock();
// 	}

	self->waitpid_status = waitpid_status;
	self->child_watch_handler_called = true;
	self->running = false;  // process is not running anymore


	// Remove fd IO callbacks. They may actually be removed already.
	// This will force calling the iochannel callback (they may not be called
	// otherwise at all if there was no output).
	if (self->event_source_id_stdout) {
		GSource* source_stdout = g_main_context_find_source_by_id(NULL, self->event_source_id_stdout);
		if (source_stdout)
			g_source_destroy(source_stdout);
	}

	if (self->event_source_id_stderr) {
		GSource* source_stderr = g_main_context_find_source_by_id(NULL, self->event_source_id_stderr);
		if (source_stderr)
			g_source_destroy(source_stderr);
	}

	// Cose std pipes
	close(self->fd_stdout);
	close(self->fd_stderr);


	self->signal_exited.emit();  // ALL slots are called in non-main thread!

	self->unlock();
// 	DBG_FUNCTION_EXIT_MSG;
}






// NOTE: This function is called in a separate thread!
// This means that all data access must be guarded by mutexes, etc...
gboolean Cmdex::on_channel_io_as_available(GIOChannel* source,
		GIOCondition cond, Cmdex* self, channel_t type)
{
// 	debug_out_dump("app", "Cmdex::on_channel_io_as_available("
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
	char buf[count] = {0};

	Cmdex::ScopedLock locker(self->object_mutex_);

	Glib::RefPtr<Glib::IOChannel> channel;
	std::string* output_str = 0;
	if (type == channel_type_stdout) {
		channel = self->channel_stdout;
		output_str = &self->str_stdout;
	} else if (type == channel_type_stderr) {
		channel = self->channel_stderr;
		output_str = &self->str_stderr;
	}


	try {  // read() may throw something

		// while there's anything to read, read it
		do {
			gsize bytes_read = 0;
			Glib::IOStatus status = channel->read(buf, count, bytes_read);
			if (bytes_read)
				output_str->append(buf, bytes_read);

			// IO_STATUS_NORMAL and IO_STATUS_AGAIN (resource unavailable) are continuable.
			if (status == Glib::IO_STATUS_ERROR || status == Glib::IO_STATUS_EOF) {
				continue_events = false;
				break;
			}
		} while (channel->get_buffer_condition() & Glib::IO_IN);

	}
	catch (Glib::IOChannelError& e) {
		self->push_error(Error<Glib::IOChannelError>("giochannel", ErrorLevel::error, e, e.what()), false);
	}
	catch (Glib::ConvertError& e) {  // no conversion in binary mode, shouldn't happen
		self->push_error(Error<void>("custom", ErrorLevel::error,
				"Cmdex::on_channel_io_as_available(): Glib::ConvertError thrown."), false);
	}

	// false if the source should be removed, true otherwise.
	return continue_events;
}




namespace {

	// this must be thread-local, because many instances of Cmdex may call this
	// from various threads. stdout and stderr are in different threads, so they won't get mixed.
	// there's one-channel-per-thread situation here, so thread-local works well.

	static hz::thread_local_ptr<std::string, hz::TlsPolicyGlib> s_final_str;

}



// The problem with final_str is that it may acummulate data if this function
// is never called with (continue_events = false) condition. This may
// actually happen with terminated spawned programs.
// So, we clean final_str manually from source_destroy callback.
// This callback is executed from in the same thread as on_channel_io_buffered,
// so we get the same variable (tests show cleared s_final_str, but is this really
// so? this function is executed during on_child_watch_handler (if spawn-terminated)).
inline void command_executor_on_buffered_source_destroy(gpointer data)
{
// 	DBG_FUNCTION_ENTER_MSG;
	s_final_str.reset();  // .reset(), not ->clear - we don't want memory leaks in stopped threads.
}




// NOTE: This function is called in a separate thread!
// This means that all data access must be guarded by mutexes, etc...
gboolean Cmdex::on_channel_io_buffered(GIOChannel* source,
		GIOCondition cond, Cmdex* self, channel_t type)
{
// 	debug_out_dump("app", "Cmdex::on_channel_io_buffered("
// 			<< (type == channel_type_stdout ? "STDOUT" : "STDERR") << ") " << int(cond) << "\n");

	bool continue_events = true;
	if ((cond & G_IO_ERR) || (cond & G_IO_HUP) || (cond & G_IO_NVAL)) {
		continue_events = false;  // there'll be no more data
	}

	if (!s_final_str.get()) {
		s_final_str.reset(new std::string);
		s_final_str->reserve(g_io_channel_get_buffer_size(source));
	}

// 	const gsize count = 4 * 1024;
	// read the bytes one by one. without this, a buffered iochannel hangs while waiting for data.
	// we don't use unbuffered iochannels - they may lose data on program exit.
	const gsize count = 1;
	static char buf[count] = {0};

// 	Glib::RefPtr<Glib::IOChannel> channel = Glib::wrap(source);  // this segfaults on large data (?)

	// while there's anything to read, read it
	GError* error = 0;
	do {
		gsize bytes_read = 0;
		GIOStatus status = g_io_channel_read_chars(source, buf, count, &bytes_read, &error);
		if (bytes_read)
			s_final_str->append(buf, bytes_read);

		// IO_STATUS_NORMAL and IO_STATUS_AGAIN (resource unavailable) are continuable.
		if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF) {
			continue_events = false;
			break;
		}
		if (error)
			break;

	} while (g_io_channel_get_buffer_condition(source) & G_IO_IN);

	if (!continue_events || error) {  // these conditions need locking
		Cmdex::ScopedLock locker(self->object_mutex_);

		if (error) {
			try {
				Glib::Error::throw_exception(error);  // convert GError to exception for easy storage.
			}
			catch (Glib::IOChannelError& e) {
				self->push_error(Error<Glib::IOChannelError>("giochannel", ErrorLevel::error, e, e.what()), false);
			}
			catch (Glib::ConvertError& e) {  // no conversion in binary mode, shouldn't happen
				self->push_error(Error<void>("custom", ErrorLevel::error,
						"Cmdex::on_channel_io_buffered(): Glib::ConvertError thrown."), false);
			}
		}

		if (!continue_events) {
			if (type == channel_type_stdout) {
				// set the source to be closed (the actual closing is done when we return false).
				// this is needed for the caller to wait until all the channels are stopped.
				self->str_stdout = *s_final_str;

			} else if (type == channel_type_stderr) {
				self->str_stderr = *s_final_str;
			}

			// glib may re-use this thread, so do a TLS cleanup.
			// also, manual cleanup is needed for policies which don't support automatic one.
			// source_destroy callback already does this, but it won't hurt.
			s_final_str.reset();  // this will delete the previous contents.
		}
	}

// 	DBG_FUNCTION_EXIT_MSG;

	// false if the source should be removed, true otherwise.
	return continue_events;
}







