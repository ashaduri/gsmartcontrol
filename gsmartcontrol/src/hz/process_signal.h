/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_PROCESS_SIGNAL_H
#define HZ_PROCESS_SIGNAL_H

#include "hz_config.h"  // feature macros

#include <string>

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_strsignal()
#else
	#include <cstring>  // for string.h
	#include <string.h>  // strsignal()
#endif

#ifdef _WIN32
	// Note: #define WINVER 0x0501 (aka winxp) before this!
	// This is needed for GetProcessId() (winxp or later).
	// Just writing a prototype doesn't work - we get undefined symbols.
	#include <windows.h>  // all that winapi stuff
	#include <cerrno>  // errno
#else
	#include <sys/types.h>  // pid_t
	#include <csignal>  // for signal.h
	#include <signal.h>  // kill()
#endif

#include "portable_snprintf.h"  // portable_snprintf



/**
\file
Compilation options:
- Define ENABLE_GLIB to 1 to enable glib-related code (portable signal messages).
*/


// win32 doesn't have signal W* macros, define them (this is _very_ rough,
// but win32 doesn't provide any better way AFAIK).
// checking one macro is enough.
#if defined _WIN32 && !defined WIFEXITED
	#define WIFEXITED(w) (((w) & 0XFFFFFF00) == 0)
	#define WIFSIGNALED(w) (!WIFEXITED(w))
	#define WEXITSTATUS(w) (w)
	#define WTERMSIG(w) (w)
#endif




namespace hz {



/// Portable strsignal() version for std::string.
/// Note: This function may return messages in native language,
/// possibly using LC_MESSAGES to select the language.
/// If Glib is enabled, it returns messages in UTF-8 format.
inline std::string signal_string(int signal_value);


/// \typedef process_id_t
/// Process handle

/// \enum signal_t
/// Sendable signals

/// \var signal_t SIGNAL_SIGNONE
/// Verify that the process exists

/// \var signal_t SIGNAL_SIGTERM
/// Ask the process to terminate itself

/// \var signal_t SIGNAL_SIGKILL
/// Nuke the process

#ifdef _WIN32
	typedef HANDLE process_id_t;  // process handle, not process id

	enum signal_t {
		SIGNAL_SIGNONE,
		SIGNAL_SIGTERM,
		SIGNAL_SIGKILL
	};

#else

	typedef pid_t process_id_t;

	enum signal_t {
		SIGNAL_SIGNONE = 0,
		SIGNAL_SIGTERM = SIGTERM,
		SIGNAL_SIGKILL = SIGKILL
	};

#endif


/// Portable kill(). Works with signal_t signals only.
/// Process groups are not supported under win32.
inline int process_signal_send(process_id_t process_handle, signal_t sig);




// ------------------------------------------ Implementation



inline std::string signal_to_string(int signal_value)
{
	std::string msg;

#if defined ENABLE_GLIB && ENABLE_GLIB
	msg = g_strsignal(signal_value);  // no need to free. won't return 0. message is in utf8.

// mingw doesn't have strsignal()!
#elif defined _WIN32

	char buf[64] = {0};
	portable_snprintf(buf, 64, "Unknown signal: %d.", signal_value);
	msg = buf;

#else  // no glib and not win32

	const char* m = strsignal(signal_value);  // this may return 0, but not on Linux.
	if (m) {
		msg = m;
	} else {
		char buf[64] = {0};
		portable_snprintf(buf, 64, "Unknown signal: %d.", signal_value);
		msg = buf;
	}
#endif

	return msg;
}




#ifdef _WIN32

namespace internal {

	// structure used to pass parameters to process_signal_find_by_pid.
	struct process_signal_find_by_pid_arg {
		process_signal_find_by_pid_arg(DWORD pid_) : pid(pid), hwnd(0)
		{ }
		DWORD pid;  // pid we're looking from
		HWND hwnd;  // hwnd used to return the result
	};

	// signal_send() helper
	inline BOOL CALLBACK process_signal_find_by_pid(HWND hwnd, LPARAM cb_arg)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);

		process_signal_find_by_pid_arg* arg = reinterpret_cast<process_signal_find_by_pid_arg*>(cb_arg);
		if (pid == arg->pid) {
			arg->hwnd = hwnd;
			return FALSE;  // stop the caller
		}

		return TRUE;  // continue searching
	}

}

#endif



#ifndef _WIN32

	inline int process_signal_send(process_id_t process_handle, signal_t sig)
	{
		return kill(process_handle, static_cast<int>(sig));  // aah, the beauty of simplicity...
	}


#else

	inline int process_signal_send(process_id_t process_handle, signal_t sig)
	{
		if (process_handle <= 0) {
			errno = ESRCH;  // The pid or process group does not exist.
			return -1;
		}

		// just check if the process exists
		if (sig == SIGNAL_SIGNONE) {
			// Warning: GetProcessId() requires winxp or higher.
			// Without it we can't do anything meaningful.
		#if defined(WINVER) && WINVER >= 0x0501
			if (GetProcessId(process_handle) == 0) {
				// this may indicate many things, but let's not be picky.
				errno = ESRCH;  // The pid or process group does not exist.
				return -1;
			}
		#endif
			return 0;  // everything ok


		// unconditionall kill, no cleanups
		} else if (sig == SIGNAL_SIGKILL) {

			// This is an ugly way of murder, but such is the life of processes in win32...
			// GetExitCodeProcess() will return UINT(-1) as exit code.
			if (TerminateProcess(process_handle, static_cast<UINT>(-1)) == 0) {
				errno = ESRCH;  // The pid or process group does not exist.
				return -1;
			}


		// try euthanasia
		} else if (sig == SIGNAL_SIGTERM) {

			// Warning: GetProcessId() requires winxp or higher.
			// Without it we can't do anything meaningful.
		#if defined(WINVER) && WINVER >= 0x0501
			internal::process_signal_find_by_pid_arg arg(GetProcessId(process_handle));

			if (EnumWindows(&internal::process_signal_find_by_pid, reinterpret_cast<LPARAM>(&arg)) != 0) {
				if (arg.hwnd) {  // we found something
					// tell it to close.
					// not sure what's the difference between ANSI/UNICODE here.
					PostMessageA(arg.hwnd, WM_QUIT, 0, 0);  // check the status later

				} else {  // error, not found
					errno = EPERM;  // no permission
					return -1;
				}

			} else {  // no windows for this pid, can't kill without them...
				errno = EPERM;  // no permission
				return -1;
			}

		#else
			errno = EPERM;  // no permission
			return -1;
		#endif

		// huh? some bad enum / int screwup happened
		} else {
			errno = EINVAL;  // invalid signal
			return -1;
		}


		// The signal was sent, wait for confirmation or something.
		DWORD exit_code = STILL_ACTIVE;

		// wait for its status to change for 500 msec.
		if (WaitForSingleObject(process_handle, 500) == WAIT_OBJECT_0) {
			// condition reached.
			// this puts STILL_ACTIVE into exit_code if it's not terminated yet.
			if (GetExitCodeProcess(process_handle, &exit_code) != 0) {
				errno = EPERM;  // no permission
				return -1;
			}
		}

		if (exit_code == STILL_ACTIVE) {
			errno = EPERM;  // no permission
			return -1;
		}

		return 0;
	}


#endif




}  // ns




#endif

/// @}