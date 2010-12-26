/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_ERRNO_STRING_H
#define HZ_ERRNO_STRING_H

#include "hz_config.h"  // feature macros

#include <string>

#if defined ENABLE_GLIB
	#include <glib.h>  // g_strerror(), g_strsignal()

#else
	#include <cstdio>  // std::snprintf

	// glibc has two versions of strerror_r:
	// GNU (may or may not use buf):
	// 		char *strerror_r(int errnum, char *buf, size_t buflen);
	// XSI (posix) (returns 0 on success):
	// 		int strerror_r(int errnum, char *buf, size_t buflen);

	#if ((defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L) \
			|| (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600)) && !defined _GNU_SOURCE
		#define HAVE_XSI_STRERROR_R
		#include <string.h>  // strerror_r
		#include <cerrno>  // errno

	#elif defined _GNU_SOURCE
		#define HAVE_GNU_STRERROR_R
		#include <string.h>  // strerror_r

	#else
		#include <cstring>  // std::strerror

	#endif

#endif


// Compilation options:
// - Define ENABLE_GLIB to enable glib-related code (portable errno messages)



namespace hz {


// Note: This function may return messages in native language,
// possibly using LC_MESSAGES to select the language.
// If Glib is enabled, it returns messages in UTF-8 format.

// portable strerror() version for std::string.
inline std::string errno_string(int errno_value);




// ------------------------------------------ Implementation



// Portable strerror() implementation
inline std::string errno_string(int errno_value)
{
	std::string msg;

#if defined ENABLE_GLIB

	msg = g_strerror(errno_value);  // no need to free. won't return 0. message is in utf8.

#else

	char buf[128] = {0};

	#if defined HAVE_XSI_STRERROR_R
		if (strerror_r(errno_value, buf, 128) == 0 && *buf) {  // always writes terminating 0 in those 128.
			msg = buf;
		} else {  // errno is set on error
			if (errno == EINVAL) {
				std::snprintf(buf, 128, "Unknown errno: %d.", errno_value);
				msg = buf;
			} else if (errno == ERANGE) {
				std::snprintf(buf, 128, "Insufficient buffer to store description for errno: %d.", errno_value);
				msg = buf;
			}
		}

	#elif defined HAVE_GNU_STRERROR_R
		const char* rmsg = strerror_r(errno_value, buf, 128);
		if (rmsg) {
			msg = rmsg;
		} else {
			std::snprintf(buf, 128, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#else  // win32 and non-gnu/posix systems.
		// use strerror(). win32 has thread-safe strerror (I think).

		const char* m = std::strerror(errno_value);  // no need to free.
		if (m) {
			msg = m;
		} else {
			std::snprintf(buf, 128, "Error while getting description for errno: %d.", errno_value);
			msg = buf;
		}

	#endif

#endif

	return msg;
}





}  // ns





#endif
