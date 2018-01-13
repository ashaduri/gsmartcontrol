/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include <cstdarg>  // for va_start, std::va_list and friends.
#include <cstdio>
#include <cstdlib>  // std::free

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_strdup_vprintf(), g_printf_string_upper_bound()
#elif defined HAVE_VASPRINTF && HAVE_VASPRINTF
	#include <stdio.h>  // vasprintf()
#endif

#include "system_specific.h"


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
inline int string_vsprintf_get_buffer_size(const char* format, std::va_list ap) HZ_FUNC_PRINTF_ISO_CHECK(1, 0);


/// Same as string_vsprintf_get_buffer_size(), but using varargs.
inline int string_sprintf_get_buffer_size(const char* format, ...) HZ_FUNC_PRINTF_ISO_CHECK(1, 2);


/// Auto-allocating std::string-returning portable vsprintf
inline std::string string_vsprintf(const char* format, std::va_list ap) HZ_FUNC_PRINTF_ISO_CHECK(1, 0);


/// Same as string_vsprintf(), but using varargs
inline std::string string_sprintf(const char* format, ...) HZ_FUNC_PRINTF_ISO_CHECK(1, 2);





// ------------------------------------------- Implementation



inline int string_vsprintf_get_buffer_size(const char* format, std::va_list ap)
{
#if defined ENABLE_GLIB && ENABLE_GLIB
	// the glib version returns gsize
	return static_cast<int>(g_printf_string_upper_bound(format, ap));

#else
	// In C++11, vsnprintf() returns the number of bytes it could have written
	// (without \0) if buffer was large enough.
	// This includes mingw/ISO.
	const int size = std::vsnprintf(nullptr, 0, format, ap);  // C99 and others
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

		written = std::vsnprintf(buf, size, format, ap);  // writes size chars, including 0.

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
