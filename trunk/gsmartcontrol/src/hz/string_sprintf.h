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

#ifndef HZ_STRING_SPRINTF_H
#define HZ_STRING_SPRINTF_H

#include "hz_config.h"  // feature macros


// Define ENABLE_GLIB to 1 to enable glib string functions (more portable?).


#include <string>
#include <cstdarg>  // for stdarg.h, va_start, std::va_list and friends.

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_strdup_vprintf(), g_printf_string_upper_bound()
#elif defined HAVE_VASPRINTF && HAVE_VASPRINTF && defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	// GNU
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // vasprintf()
	#include <cstdlib>  // std::free
#elif defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	// POSIX, mingw/ISO
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // vsnprintf()
	#include <stdarg.h>  // vsnprintf()
#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	// msvc 2005 and later
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // vsnprintf_s()
	#include <stdarg.h>  // vsnprintf_s()
#else
	// mingw/no-ISO, msvc, possibly other old systems
	#include <cstdio>  // for stdio.h
	#include <stdio.h>  // _vsnprintf()
	#include <stdarg.h>  // _vsnprintf()
#endif

#include "string_sprintf_macros.h"


/**
\file
\brief sprintf()-like formatting to std::string with automatic allocation.

\note These functions use system *printf family of functions.

\note If using mingw runtime >= 3.15 and __USE_MINGW_ANSI_STDIO,
mingw supports both C99/POSIX and msvcrt format specifiers.
This includes proper printing of long double, %lld and %llu, etc...
This does _not_ affect _snprintf() and similar non-standard functions.
Note that you may still get warnings from gcc regarding non-MS
format specifiers (see HZ_FUNC_PRINTF_CHECK).


\note There types are non-portable (the first one is MS variant, the second one is standard):
%I64d, %lld (long long int),
%I64u, %llu (unsigned long long int),
%f, %Lf (long double; needs casting to double under non-ANSI mingw if using %f).

See string_sprintf_macros.h on how to test the feature availability.
*/



namespace hz {



/// Get the buffer size required to safely allocate a buffer (including terminating 0) and call vsnprintf().
inline int string_vsprintf_get_buffer_size(const char* format, std::va_list ap) HZ_FUNC_STRING_SPRINTF_CHECK(1, 0);


/// same as string_vsprintf_get_buffer_size(), but using varargs.
inline int string_sprintf_get_buffer_size(const char* format, ...) HZ_FUNC_STRING_SPRINTF_CHECK(1, 2);


/// auto-allocating std::string-returning portable vsprintf
inline std::string string_vsprintf(const char* format, std::va_list ap) HZ_FUNC_STRING_SPRINTF_CHECK(1, 0);


/// same as string_vsprintf(), but using varargs
inline std::string string_sprintf(const char* format, ...) HZ_FUNC_STRING_SPRINTF_CHECK(1, 2);





// ------------------------------------------- Implementation



inline int string_vsprintf_get_buffer_size(const char* format, std::va_list ap)
{
#if defined ENABLE_GLIB && ENABLE_GLIB
	// the glib version returns gsize
	return static_cast<int>(g_printf_string_upper_bound(format, ap));

#elif defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	// In C99/POSIX, vsnprintf() returns the number of bytes it could have written
	// (without \0) if buffer was large enough.
	// This includes mingw/ISO.
	char c = 0;
	const int size = vsnprintf(&c, 1, format, ap);  // C99 and others
	if (size < 0)
		return -1;
	return size + 1;  // that +1 is for \0.

#else
	// Unfortunately, MS (maybe some very old systems as well) return -1 if buffer
	// is not large enough, so we have to use incremental reallocation until it fits.

	errno = 0;  // reset
	int buf_size = 32;
	int ret = -1;

	do {
		char* buf = new char[buf_size];

		#if defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
			const int size = vsnprintf_s(buf, buf_size, buf_size - 1, format, ap);  // writes (buf_size - 1) chars, appends 0.

		#elif defined HAVE__VSNPRINTF && HAVE__VSNPRINTF
			// Prefer _vsnprintf() to vsnprintf() as MS may change the latter eventually.
			const int size = _vsnprintf(buf, buf_size, format, ap);  // writes buf_size chars, including 0 (if it fits).

		#else
			const int size = vsnprintf(buf, buf_size, format, ap);  // assume same as above
		#endif

		if (size == -1) {
			if (errno != 0) {  // error
				break;  // -1
			} else {  // doesn't fit, increase size.
				buf_size *= 4;  // continue
			}

		} else if (size <= buf_size) {
			ret = size + 1;  // for 0, as the returned value doesn't contain the 0.
			break;

		} else {  // huh?
			break;  // -1, treat as error
		}

		delete[] buf;

	} while (true);

	return ret;

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

#elif defined HAVE_VASPRINTF && HAVE_VASPRINTF && defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	// We check both vasprintf() and ISO compliance above to ensure that
	// string_vsnprintf() and string_vsprintf_get_buffer_size() can work with
	// the same format string.

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
	#if defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
		// we could use vsprintf(), but it's deemed unsafe in many environments.
		written = vsnprintf(buf, size, format, ap);  // writes size chars, including 0.

	#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
		written = vsnprintf_s(buf, size, size - 1, format, ap);  // writes (size - 1) chars, appends 0.

	#elif defined HAVE__VSNPRINTF && HAVE__VSNPRINTF
		written = _vsnprintf(buf, size, format, ap);  // writes size chars, including 0.
	#else
		// Same as the first one, to ensure that the order matches that of string_vsprintf_get_buffer_size().
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

/// @}
