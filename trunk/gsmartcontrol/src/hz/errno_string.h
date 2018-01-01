/**************************************************************************
 Copyright:
      (C) 2008 - 2013  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_ERRNO_STRING_H
#define HZ_ERRNO_STRING_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstddef>  // std::size_t
#include <cstdarg>  // for stdarg.h, va_start, std::va_list and friends.
#include <cstdio>  // std::snprintf

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_strerror(), g_strsignal()

#else
	// glibc has two versions of strerror_r:
	// GNU (may or may not use buf):
	// 		char *strerror_r(int errnum, char *buf, size_t buflen);
	// XSI (posix) (returns 0 on success):
	// 		int strerror_r(int errnum, char *buf, size_t buflen);

	#if defined HAVE_XSI_STRERROR_R && HAVE_XSI_STRERROR_R
		#include <cstring>  // for string.h
		#include <string.h>  // strerror_r
		#include <cerrno>  // errno
	#elif defined HAVE_GNU_STRERROR_R && HAVE_GNU_STRERROR_R
		#include <cstring>  // for string.h
		#include <string.h>  // strerror_r
	#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		#include <cstring>  // for string.h
		#include <string.h>  // strerror_s
		#include "win32_tools.h"
	#else
		#include <cstring>  // std::strerror
	#endif

#endif


// Compilation options:
// - Define ENABLE_GLIB to 1 to enable glib-related code (portable errno messages)



namespace hz {


// TODO: make messages always in utf-8 (convert from locale).


/// Portable version of strerror() for std::string.
/// Note: This function may return messages in native language,
/// possibly using LC_MESSAGES to select the language.
/// If Glib is enabled, it returns messages in UTF-8 format.
inline std::string errno_string(int errno_value);




// ------------------------------------------ Implementation



inline std::string errno_string(int errno_value)
{
	std::string msg;

#if defined ENABLE_GLIB && ENABLE_GLIB

	msg = g_strerror(errno_value);  // no need to free. won't return 0. message is in utf8.

#else

	const int buf_size = 128;
	char buf[buf_size] = {0};

	#if defined HAVE_XSI_STRERROR_R && HAVE_XSI_STRERROR_R
		if (strerror_r(errno_value, buf, buf_size) == 0 && *buf) {  // always writes terminating 0 in those buf_size bytes.
			msg = buf;
		} else {  // errno is set on error
			if (errno == EINVAL) {  // errno may be a macro
				std::snprintf(buf, buf_size, "Unknown errno: %d.", errno_value);
				msg = buf;
			} else if (errno == ERANGE) {
				std::snprintf(buf, buf_size, "Insufficient buffer to store description for errno: %d.", errno_value);
				msg = buf;
			}
		}

	#elif defined HAVE_GNU_STRERROR_R && HAVE_GNU_STRERROR_R
		const char* rmsg = strerror_r(errno_value, buf, buf_size);
		if (rmsg) {
			msg = rmsg;
		} else {
			std::snprintf(buf, buf_size, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		wchar_t msg_buf[128] = {0};  // MS docs don't say exactly, but it's somewhere in 80-100 range.
		if (_wcserror_s(msg_buf, 128, errno_value) != 0) {  // always 0-terminated
			std::snprintf(buf, buf_size, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}
		msg = hz::win32_utf16_to_utf8(msg_buf);

	#else  // win32 and non-gnu/posix systems.
		// win32 has thread-safe strerror().
		const char* m = std::strerror(errno_value);  // no need to free.
		if (m) {
			msg = m;
		} else {
			std::snprintf(buf, buf_size, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#endif

#endif

	return msg;
}





}  // ns





#endif

/// @}
