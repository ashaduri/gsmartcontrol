/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_ERRNO_STRING_H
#define HZ_ERRNO_STRING_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstddef>  // std::size_t
#include <cstdarg>  // for stdarg.h, va_start, std::va_list and friends.

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
	#else
		#include <cstring>  // std::strerror
	#endif

#endif


#include "portable_snprintf.h"  // portable_snprintf


// Compilation options:
// - Define ENABLE_GLIB to 1 to enable glib-related code (portable errno messages)



namespace hz {


// Note: This function may return messages in native language,
// possibly using LC_MESSAGES to select the language.
// If Glib is enabled, it returns messages in UTF-8 format.

// TODO: make messages always in utf-8 (convert from locale).

// portable strerror() version for std::string.
inline std::string errno_string(int errno_value);




// ------------------------------------------ Implementation



// Portable strerror() implementation
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
				portable_snprintf(buf, buf_size, "Unknown errno: %d.", errno_value);
				msg = buf;
			} else if (errno == ERANGE) {
				portable_snprintf(buf, buf_size, "Insufficient buffer to store description for errno: %d.", errno_value);
				msg = buf;
			}
		}

	#elif defined HAVE_GNU_STRERROR_R && HAVE_GNU_STRERROR_R
		const char* rmsg = strerror_r(errno_value, buf, buf_size);
		if (rmsg) {
			msg = rmsg;
		} else {
			portable_snprintf(buf, buf_size, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		char msg_buf[128] = {0};  // MS docs don't say exactly, but it's somewhere in 80-100 range.
		if (strerror_s(msg_buf, 128, errno_value) != 0) {  // always 0-terminated
			portable_snprintf(buf, buf_size, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#else  // win32 and non-gnu/posix systems.
		// win32 has thread-safe strerror().
		const char* m = std::strerror(errno_value);  // no need to free.
		if (m) {
			msg = m;
		} else {
			portable_snprintf(buf, buf_size, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#endif

#endif

	return msg;
}





}  // ns





#endif
