/**************************************************************************
 Copyright:
      (C) 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_PORTABLE_SNPRINTF_H
#define HZ_PORTABLE_SNPRINTF_H

#include "hz_config.h"  // feature macros


// Compilation options:
// - Define ENABLE_GLIB to 1 to enable glib-related code (portable ISO-compatible (v)snprintf).



namespace hz {


// Note: There are macros, so they are not namespaced.


// Usage: void portable_snprintf(char *str, size_t size, const char *format, ...);

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_snprintf

	// It's ISO-compatible
	#define portable_snprintf(buf, buf_size, ...) \
		(void)g_snprintf((buf), (buf_size), __VA_ARGS__)

#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _snprintf_s

	// writes at most buf_size bytes, always including 0.
	#define portable_snprintf(buf, buf_size, ...) \
		(void)_snprintf_s((buf), (buf_size), _TRUNCATE, __VA_ARGS__)

#elif !(defined HAVE_PROPER_SNPRINTF && HAVE_PROPER_SNPRINTF) \
		&& (defined HAVE__SNPRINTF && HAVE__SNPRINTF)
	// Prefer snprintf() to _snprintf() if we're sure it's good.
	// Note that old versions of mingw used to define snprintf() as _snprintf(),
	// which is not proper.

	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _snprintf

	// _snprintf() writes at most buf_size-1 bytes. the buffer should be zeroed out
	// beforehand.
	#define portable_snprintf(buf, buf_size, ...) \
		(void)((_snprintf((buf), (buf_size) - 1, __VA_ARGS__) == ((buf_size) - 1)) && ((buf)[(buf_size)-1] = '\0'))

#else
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // snprintf

	// essentially (defined HAVE_PROPER_SNPRINTF && HAVE_PROPER_SNPRINTF), but
	// since we have no other option, let's require snprintf().
	#define portable_snprintf(buf, buf_size, ...) \
		(void)snprintf((buf), (buf_size), __VA_ARGS__)
#endif




// Usage: void portable_vsnprintf(char *str, size_t size, const char *format, va_list ap);

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>  // g_vsnprintf

	// It's ISO-compatible
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)g_vsnprintf((buf), (buf_size), (format), (ap))

#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _vsnprintf_s

	// writes at most buf_size bytes, always including 0.
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)vsnprintf_s((buf), (buf_size), _TRUNCATE, (format), (ap))

#elif !(defined HAVE_PROPER_VSNPRINTF && HAVE_PROPER_VSNPRINTF) \
		&& (defined HAVE__VSNPRINTF && HAVE__VSNPRINTF)
	// prefer snprintf() to _snprintf() if we're sure it's good.

	#include <cstdio>  // for cstdio
	#include <stdio.h>  // _vsnprintf
	#include <cstdarg>  // for cstdarg
	#include <stdarg.h>  // _vsnprintf

	// _vsnprintf() writes at most buf_size-1 bytes. the buffer should be zeroed out
	// beforehand.
	// Note that MS's vsnprintf (which is the same as their _vsnprintf())
	// doesn't always write 0, that's why we have HAVE_PROPER_VSNPRINTF
	// instead of HAVE_VSNPRINTF.
	// We use _vsnprintf() instead of vsnprintf() to avoid using the proper vsnprintf()
	// by chance (_vsnprintf() is always broken, vsnprintf() may not be).
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)((_vsnprintf((buf), (buf_size) - 1, (format), (ap)) == ((buf_size) - 1)) && ((buf)[(buf_size)-1] = '\0'))

#else
	#include <cstdio>  // for cstdio
	#include <stdio.h>  // vsnprintf (mingw, posix(?))
	#include <cstdarg>  // for cstdarg
	#include <stdarg.h>  // vsnprintf (gnu)

	// essentially (defined HAVE_PROPER_VSNPRINTF && HAVE_PROPER_VSNPRINTF), but
	// since we have no other option, let's require vsnprintf().
	#define portable_vsnprintf(buf, buf_size, format, ap) \
		(void)vsnprintf((buf), (buf_size), (format), (ap))
#endif





}  // ns





#endif
