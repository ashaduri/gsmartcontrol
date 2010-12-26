/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_STRING_SPRINTF_H
#define HZ_STRING_SPRINTF_H

#include "hz_config.h"  // feature macros

#include <string>
#include <stdarg.h>  // va_start, va_list macro and friends; vsnprintf() (C99, various other extensions)


// Define ENABLE_GLIB to enable glib string functions (more portable?).

#if defined ENABLE_GLIB
#	include <glib.h>  // g_strdup_vprintf(), g_printf_string_upper_bound()
#elif defined _GNU_SOURCE
#	include <stdio.h>  // vasprintf() (GNU extension)
#	include <stdlib.h>  // free()
#endif

#include "system_specific.h"


// sprintf()-like formatting to std::string with automatic allocation.


namespace hz {



// get the buffer size required to safely allocate a buffer and call vsnprintf().
// return -1 if something is wrong.
inline int string_vsprintf_get_buffer_size(const char* format, va_list ap) HZ_FUNC_PRINTF_CHECK(1, 0);

// same as above, but using varargs.
// return -1 if something is wrong.
inline int string_sprintf_get_buffer_size(const char* format, ...) HZ_FUNC_PRINTF_CHECK(1, 2);


// auto-allocating std::string-returning portable vsprintf
inline std::string string_vsprintf(const char* format, va_list ap) HZ_FUNC_PRINTF_CHECK(1, 0);


// same as above, but using varargs
inline std::string string_sprintf(const char* format, ...) HZ_FUNC_PRINTF_CHECK(1, 2);





// ------------------------------------------- Implementation



inline int string_vsprintf_get_buffer_size(const char* format, va_list ap)
{
#ifdef ENABLE_GLIB
	// the glib version returns gsize
	return static_cast<int>(g_printf_string_upper_bound(format, ap));
#else
	char c = 0;
	// vsnprintf() returns the number of bytes it could have written (without \0) if buffer was large enough.
	const int size = vsnprintf(&c, 1, format, ap);  // C99 and others
	if (size < 0)
		return -1;
	return size + 1;  // that +1 is for \0.
#endif
}



inline int string_sprintf_get_buffer_size(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = string_vsprintf_get_buffer_size(format, ap);
	va_end(ap);
	return ret;
}




inline std::string string_vsprintf(const char* format, va_list ap)
{
#if defined ENABLE_GLIB

	// there's also g_vasprintf(), but only since glib 2.4.
	gchar* buf = g_strdup_vprintf(format, ap);
	std::string ret = (buf ? buf : "");
	if (buf)
		g_free(buf);

#elif defined _GNU_SOURCE

	std::string ret;
	char* str = 0;
	if (vasprintf(&str, format, ap) > 0)  // GNU extension
		ret = (str ? str : "");
	free(str);

#else

	std::string ret;
	int size = string_vsprintf_get_buffer_size(format, ap);
	if (size > 0) {
		char* buf = new char[size];
		// write at most size (with \0). we could use vsprintf(), but it's deemed unsafe in many environments.
		if (vsnprintf(buf, size, format, ap) > 0)
			ret = buf;
		delete[] buf;
	}

#endif
	return ret;
}



inline std::string string_sprintf(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	std::string ret = string_vsprintf(format, ap);
	va_end(ap);
	return ret;
}






}  // ns




#endif
