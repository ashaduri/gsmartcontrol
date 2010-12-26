/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_STRING_SPRINTF_H
#define HZ_STRING_SPRINTF_H

#include "hz_config.h"  // feature macros


// Define ENABLE_GLIB to 1 to enable glib string functions (more portable?).


#include <string>
#include <cstdarg>  // for stdarg.h, va_start, std::va_list and friends.

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_strdup_vprintf(), g_printf_string_upper_bound()
#elif defined HAVE_VASPRINTF && HAVE_VASPRINTF
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // vasprintf()
	#include <cstdlib>  // std::free
#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // vsnprintf_s()
	#include <stdarg.h>  // vsnprintf_s()
#else  // mingw, msvc, posix, c99
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // vsnprintf()
	#include <stdarg.h>  // vsnprintf()
#endif

#include "system_specific.h"  // HZ_FUNC_PRINTF_CHECK



// sprintf()-like formatting to std::string with automatic allocation.

// Note: These functions use system *printf family of functions.

// Note: If using mingw runtime >= 3.15 and __USE_MINGW_ANSI_STDIO,
// mingw supports both C99/POSIX and msvcrt format specifiers.
// This includes proper printing of long double, %lld and %llu, etc...
// This does _not_ affect _snprintf() and similar non-standard functions.
// Note that you may still get warnings from gcc regarding non-MS
// format specifiers (see HZ_FUNC_PRINTF_CHECK).

// For other Windows targets there is no support for %Lf (long double),
// you have to use %I64d instead of %lld for long long int
// and %I64u instead of %llu for unsigned long long int.
// You can use hz::number_to_string() as a workaround.



namespace hz {



// Get the buffer size required to safely allocate a buffer (including terminating 0) and call vsnprintf().
inline int string_vsprintf_get_buffer_size(const char* format, std::va_list ap) HZ_FUNC_PRINTF_CHECK(1, 0);


// same as above, but using varargs.
inline int string_sprintf_get_buffer_size(const char* format, ...) HZ_FUNC_PRINTF_CHECK(1, 2);


// auto-allocating std::string-returning portable vsprintf
inline std::string string_vsprintf(const char* format, std::va_list ap) HZ_FUNC_PRINTF_CHECK(1, 0);


// same as above, but using varargs
inline std::string string_sprintf(const char* format, ...) HZ_FUNC_PRINTF_CHECK(1, 2);





// ------------------------------------------- Implementation



inline int string_vsprintf_get_buffer_size(const char* format, std::va_list ap)
{
#if defined ENABLE_GLIB && ENABLE_GLIB
	// the glib version returns gsize
	return static_cast<int>(g_printf_string_upper_bound(format, ap));

#elif defined _WIN32

	// in C99, vsnprintf() returns the number of bytes it could have written
	// (without \0) if buffer was large enough. Unfortunately, MS returns -1 if buffer
	// is not large enough, so we have to use incremental reallocation until it fits.

	errno = 0;  // reset
	int buf_size = 32;
	int ret = -1;

	do {
		char* buf = new char[buf_size];

		#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
			const int size = vsnprintf_s(buf, buf_size, buf_size - 1, format, ap);  // writes (buf_size - 1) chars, appends 0.
		#else
			const int size = vsnprintf(buf, buf_size, format, ap);  // writes buf_size chars, including 0.
		#endif

		if (size == -1) {
			if (errno != 0) {  // error
				break;  // -1
			} else {  // doesn't fit, increase size.
				buf_size *= 4;  // continue
			}

		} else if (size <= buf_size) {
			ret = size + 1;  // for 0
			break;

		} else {  // huh?
			break;  // -1, treat as error
		}

		delete[] buf;

	} while (true);

	return ret;

#else  // normal systems have C99 or POSIX

	char c = 0;
	const int size = vsnprintf(&c, 1, format, ap);  // C99 and others
	if (size < 0)
		return -1;
	return size + 1;  // that +1 is for \0.

#endif
}



inline int string_sprintf_get_buffer_size(const char* format, ...)
{
	std::va_list ap;
	va_start(ap, format);
	int ret = string_vsprintf_get_buffer_size(format, ap);
	va_end(ap);
	return ret;
}




inline std::string string_vsprintf(const char* format, std::va_list ap)
{
#if defined ENABLE_GLIB && ENABLE_GLIB

	// there's also g_vasprintf(), but only since glib 2.4.
	gchar* buf = g_strdup_vprintf(format, ap);
	std::string ret = (buf ? buf : "");
	if (buf)
		g_free(buf);

#elif defined HAVE_VASPRINTF && HAVE_VASPRINTF

	std::string ret;
	char* str = 0;
	if (vasprintf(&str, format, ap) > 0)  // GNU extension
		ret = (str ? str : "");
	std::free(str);

#else

	std::string ret;
	int size = string_vsprintf_get_buffer_size(format, ap);
	if (size > 0) {
		char* buf = new char[size];
		int written = 0;
	#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		written = vsnprintf_s(buf, size, size - 1, format, ap);  // writes (size - 1) chars, appends 0.
	#else
		// we could use vsprintf(), but it's deemed unsafe in many environments.
		written = vsnprintf(buf, size, format, ap);  // writes size chars, including 0.
	#endif
		if (written > 0)
			ret = buf;
		delete[] buf;
	}

#endif
	return ret;
}



inline std::string string_sprintf(const char* format, ...)
{
	std::va_list ap;
	va_start(ap, format);
	std::string ret = string_vsprintf(format, ap);
	va_end(ap);
	return ret;
}






}  // ns




#endif
