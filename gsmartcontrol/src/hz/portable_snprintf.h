/**************************************************************************
 Copyright:
      (C) 2009 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_PORTABLE_SNPRINTF_H
#define HZ_PORTABLE_SNPRINTF_H

#include "hz_config.h"  // feature macros


/**
\file
Compilation options:
- Define \c ENABLE_GLIB to 1 to enable glib-related code (portable ISO-compatible (v)snprintf).

Keep in mind that these format types are non-portable (the first one is MS variant,
the second one is standard):
- %I64d, %lld (long long int),
- %I64u, %llu (unsigned long long int),
- %f, %Lf (long double; needs casting to double under non-ANSI mingw if using %f).
*/


namespace hz {


// Note: There are macros, so they are not namespaced.


/// \def HAVE_PORTABLE_SNPRINTF_MS
/// If 1, portable_snprintf() and portable_vsnprintf() accept I64d, I64u.
/// Users are expected to check this macro to see if the format is supported.

/// \def HAVE_PORTABLE_SNPRINTF_ISO
/// If 1, portable_snprintf() and portable_vsnprintf() accept lld, llu, Lf.
/// Users are expected to check this macro to see if the format is supported.



/// \def portable_snprintf(buf, buf_size, ...)
/// snprintf() wrapper that always behaves according to ISO standard in terms of 0-termination.
/// Note that this is a macro, and therefore not namespaced.
/// Use it like this:
/// \code void portable_snprintf(char *str, size_t size, const char *format, ...); \endcode

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_snprintf

	// It's ISO-compatible
	#define portable_snprintf(buf, buf_size, ...) \
		(void)g_snprintf((buf), (buf_size), __VA_ARGS__)

	#define HAVE_PORTABLE_SNPRINTF_MS 0
	#define HAVE_PORTABLE_SNPRINTF_ISO 1

#elif defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	// mingw/ISO, all non-windows platforms.
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // snprintf

	#define portable_snprintf(buf, buf_size, ...) \
		(void)snprintf((buf), (buf_size), __VA_ARGS__)

	// mingw/ISO has both the MS and ISO specifiers
	#if defined _WIN32 && defined __GNUC__
		#define HAVE_PORTABLE_SNPRINTF_MS 1
	#else
		#define HAVE_PORTABLE_SNPRINTF_MS 0
	#endif

	#define HAVE_PORTABLE_SNPRINTF_ISO 1

#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _snprintf_s

	// writes at most buf_size bytes, always including 0.
	#define portable_snprintf(buf, buf_size, ...) \
		(void)_snprintf_s((buf), (buf_size), _TRUNCATE, __VA_ARGS__)

	#define HAVE_PORTABLE_SNPRINTF_MS 1
	#define HAVE_PORTABLE_SNPRINTF_ISO 0

#elif defined HAVE__SNPRINTF && HAVE__SNPRINTF
	// Prefer snprintf() to _snprintf() if we're sure it's good.
	// Note that old versions of mingw used to define snprintf() as _snprintf(),
	// which is not proper.

	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _snprintf

	// _snprintf() writes at most buf_size-1 bytes. the buffer should be zeroed out
	// beforehand.
	#define portable_snprintf(buf, buf_size, ...) \
		(void)((_snprintf((buf), (buf_size) - 1, __VA_ARGS__) == ((buf_size) - 1)) && ((buf)[(buf_size)-1] = '\0'))

	#define HAVE_PORTABLE_SNPRINTF_MS 0
	#define HAVE_PORTABLE_SNPRINTF_ISO 1

#else
	#error Cannot find suitable snprintf() implementation for portable_snprintf()

#endif




/// \def portable_vsnprintf(buf, buf_size, ...)
/// vsnprintf() wrapper that always behaves according to ISO standard in terms of 0-termination.
/// Note that this is a macro, and therefore not namespaced.
/// Use it like this:
/// \code void portable_vsnprintf(char *str, size_t size, const char *format, va_list ap); \endcode

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_vsnprintf

	// It's ISO-compatible
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)g_vsnprintf((buf), (buf_size), (format), (ap))

	#define HAVE_PORTABLE_VSNPRINTF_MS 0
	#define HAVE_PORTABLE_VSNPRINTF_ISO 1

#elif defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // vsnprintf (mingw, posix(?))
	#include <cstdarg>  // for cstdarg
	#include <stdarg.h>  // vsnprintf (gnu)

	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)vsnprintf((buf), (buf_size), (format), (ap))

	// mingw/ISO has both the MS and ISO specifiers
	#if defined _WIN32 && defined __GNUC__
		#define HAVE_PORTABLE_VSNPRINTF_MS 1
	#else
		#define HAVE_PORTABLE_VSNPRINTF_MS 0
	#endif
	#define HAVE_PORTABLE_VSNPRINTF_ISO 1

#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _vsnprintf_s

	// writes at most buf_size bytes, always including 0.
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)vsnprintf_s((buf), (buf_size), _TRUNCATE, (format), (ap))

	#define HAVE_PORTABLE_VSNPRINTF_MS 1
	#define HAVE_PORTABLE_VSNPRINTF_ISO 0

#elif defined HAVE__VSNPRINTF && HAVE__VSNPRINTF
	// prefer snprintf() to _snprintf() if we're sure it's good.

	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _vsnprintf
	#include <cstdarg>  // for cstdarg
	#include <stdarg.h>  // _vsnprintf

	// _vsnprintf() writes at most buf_size-1 bytes. the buffer should be zeroed out
	// beforehand.
	// Note that MS's vsnprintf() (which is the same as their _vsnprintf())
	// doesn't always write 0.
	// We use _vsnprintf() instead of vsnprintf() to avoid using the proper vsnprintf()
	// by chance (_vsnprintf() is always broken, vsnprintf() may not be).
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)((_vsnprintf((buf), (buf_size) - 1, (format), (ap)) == ((buf_size) - 1)) && ((buf)[(buf_size)-1] = '\0'))

	#define HAVE_PORTABLE_VSNPRINTF_MS 1
	#define HAVE_PORTABLE_VSNPRINTF_ISO 0

#else
	#error Cannot find suitable vsnprintf() implementation for portable_vsnprintf()
#endif





}  // ns





#endif

/// @}
