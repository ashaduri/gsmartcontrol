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

#ifndef HZ_HZ_CONFIG_H
#define HZ_HZ_CONFIG_H

/**
\file
\brief This file is included from all hz headers.

You should include this file if you're using any of:
- macros from autoconf-generated config.h

The purpose of this file is to provide some common compatibility
solutions for the whole hz, while not depending on pasting autoconf's
config.h code into all files.
*/


/**
\namespace hz
HZ library, system-level abstraction and general utilities.
*/


/**
\namespace hz::internal
HZ library internal implementation helpers.
*/


// Autoconf-generated feature macros
#include "config.h"



// Functions / feature checks


#if defined _WIN32 && defined __GNUC__
	#include <_mingw.h>  // mingw feature macros, used below
#endif




/// \def HAVE_VERBOSE_TERMINATE_HANDLER
/// Defined to 0 or 1. If 1, compiler supports __gnu_cxx::__verbose_terminate_handler.
#ifndef HAVE_VERBOSE_TERMINATE_HANDLER
	#if defined __GNUC__
		#define HAVE_VERBOSE_TERMINATE_HANDLER 1
	#else
		#define HAVE_VERBOSE_TERMINATE_HANDLER 0
	#endif
#endif



/// \def HAVE_GCC_ABI_DEMANGLE
/// Defined to 0 or 1. If 1, compiler supports \::abi::__cxa_demangle.
#ifndef HAVE_GCC_ABI_DEMANGLE
	// This also works with intel/linux (__GNUC__ is defined by default in it).
	#if defined __GNUC__
		#define HAVE_GCC_ABI_DEMANGLE 1
	#else
		#define HAVE_GCC_ABI_DEMANGLE 0
	#endif
#endif




/// \def HAVE_WIN_SE_FUNCS
/// Defined to 0 or 1. If 1, compiler supports Win32's "secure" *_s() functions (since msvc 2005/8.0).
/// If some of the functions have different requirements, they are listed separately.
#ifndef HAVE_WIN_SE_FUNCS
	#if defined _MSC_VER && _MSC_VER >= 1400
		#define HAVE_WIN_SE_FUNCS 1
	#else
		#define HAVE_WIN_SE_FUNCS 0
	#endif
#endif




/// \def HAVE_POSIX_OFF_T_FUNCS
/// Defined to 0 or 1. If 1, compiler supports fseeko/ftello. See fs_file.h for details.
#ifndef HAVE_POSIX_OFF_T_FUNCS
	// It's quite hard to detect, so we just enable for any non-win32.
	#if defined _WIN32
		#define HAVE_POSIX_OFF_T_FUNCS 0
	#else
		#define HAVE_POSIX_OFF_T_FUNCS 1
	#endif
#endif




/// \def HAVE_WIN_LFS_FUNCS
/// Defined to 0 or 1. If 1, compiler supports LFS (large file support) functions on win32.
#ifndef HAVE_WIN_LFS_FUNCS
	// enable if it's msvc >= 2005, or we're linking to newer msvcrt with mingw.
	#if defined _WIN32 && ( (defined _MSC_VER >= 1400) \
			|| (defined __GNUC__ && defined __MSVCRT_VERSION__ && __MSVCRT_VERSION__ >= 0x800) )
		#define HAVE_WIN_LFS_FUNCS 1
	#else
		#define HAVE_WIN_LFS_FUNCS 0
	#endif
#endif




/// \def HAVE_XSI_STRERROR_R
/// Defined to 0 or 1. If 1, compiler supports XSI-style strerror_r().
#ifndef HAVE_XSI_STRERROR_R
	#if ((defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L) \
			|| (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600)) && !defined _GNU_SOURCE
		#define HAVE_XSI_STRERROR_R 1
	#else
		#define HAVE_XSI_STRERROR_R 0
	#endif
#endif


/// \def HAVE_GNU_STRERROR_R
/// Defined to 0 or 1. If 1, compiler supports GNU-style strerror_r().
#ifndef HAVE_GNU_STRERROR_R
	// _GNU_SOURCE excludes HAVE_XSI_STRERROR_R
	#if defined _GNU_SOURCE
		#define HAVE_GNU_STRERROR_R 1
	#else
		#define HAVE_GNU_STRERROR_R 0
	#endif
#endif



/// \def HAVE_VASPRINTF
/// Defined to 0 or 1. If 1, the compiler has vasprintf() (GNU/glibc extension).
#ifndef HAVE_VASPRINTF
	#if defined _GNU_SOURCE
		#define HAVE_VASPRINTF 1
	#else
		#define HAVE_VASPRINTF 0
	#endif
#endif



/// \def HAVE_REENTRANT_LOCALTIME
/// Defined to 0 or 1. If 1, localtime() is reentrant.
#ifndef HAVE_REENTRANT_LOCALTIME
	// win32 and solaris localtime() is reentrant
	#if defined _WIN32 || defined sun || defined __sun
		#define HAVE_REENTRANT_LOCALTIME 1
	#else
		#define HAVE_REENTRANT_LOCALTIME 0
	#endif
#endif




/// \def HAVE_SETENV
/// Defined to 0 or 1. If 1, the compiler has setenv() and unsetenv().
#ifndef HAVE_SETENV
	// setenv(), unsetenv() glibc feature test macros:
	// _BSD_SOURCE || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600.
    // However, since they are available almost anywhere except windows,
    // we just test for that.
	#if defined _WIN32
		#define HAVE_SETENV 0
	#else
		#define HAVE_SETENV 1
	#endif
#endif




#endif

/// @}
