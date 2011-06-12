/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_STRING_SPRINTF_MACROS_H
#define HZ_STRING_SPRINTF_MACROS_H

#include "hz_config.h"  // feature macros

#include "system_specific.h"  // HZ_FUNC_PRINTF_*_CHECK


// This is a helper file for string_sprintf.h. It is useful if you need
// the macros only, but not the implementation (useful for header files).



// HAVE_STRING_SPRINTF_MS - whether the MS (%I64d and %I64u)
// specifiers are supported.

// HAVE_STRING_SPRINTF_ISO - whether the ISO
// (%lld, %llu, %Lf) specifiers are supported.

#if defined ENABLE_GLIB && ENABLE_GLIB
	#define HAVE_STRING_SPRINTF_MS 0
	#define HAVE_STRING_SPRINTF_ISO 1

#elif defined HAVE_ISO_STDIO && HAVE_ISO_STDIO
	// mingw/ISO has both the MS and ISO specifiers
	#if defined _WIN32 && defined __GNUC__
		#define HAVE_STRING_SPRINTF_MS 1
	#else
		#define HAVE_STRING_SPRINTF_MS 0
	#endif
	#define HAVE_STRING_SPRINTF_ISO 1

#elif defined HAVE_WIN_SE_FUNCS && HAVE_WIN_SE_FUNCS
	#define HAVE_STRING_SPRINTF_MS 1
	#define HAVE_STRING_SPRINTF_ISO 0

#elif defined HAVE__VSNPRINTF && HAVE__VSNPRINTF
	#define HAVE_STRING_SPRINTF_MS 0
	#define HAVE_STRING_SPRINTF_ISO 1

#else
	#warning Cannot detect vsnprintf() availability

	#define HAVE_STRING_SPRINTF_MS 0
	#define HAVE_STRING_SPRINTF_ISO 0
#endif




// HZ_FUNC_STRING_SPRINTF_CHECK - gcc printf format attribute checker,
// configured according to MS or ISO features (prefers ISO over MS).

#if HAVE_STRING_SPRINTF_ISO
	#define HZ_FUNC_STRING_SPRINTF_CHECK(format_idx, check_idx) HZ_FUNC_PRINTF_ISO_CHECK(format_idx, check_idx)
#elif HAVE_STRING_SPRINTF_MS
	#define HZ_FUNC_STRING_SPRINTF_CHECK(format_idx, check_idx) HZ_FUNC_PRINTF_MS_CHECK(format_idx, check_idx)
#else
	#define HZ_FUNC_STRING_SPRINTF_CHECK(format_idx, check_idx)
#endif





#endif

/// @}
