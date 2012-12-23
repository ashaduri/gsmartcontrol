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

#ifndef HZ_HZ_CONFIG_H
#define HZ_HZ_CONFIG_H

/**
\file
\brief This file is included from all hz headers.

You should include this file if you're using any of:
- macros from autoconf-generated config.h
- macros from global_macros.h

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


#ifdef DOXYGEN_ONLY
	/// Define HZ_USE_GLOBAL_MACROS=1 from a compiler option to enable
	/// auto-inclusion of global_macros.h.
	#define HZ_USE_GLOBAL_MACROS
#endif


#if defined HZ_USE_GLOBAL_MACROS && HZ_USE_GLOBAL_MACROS
	// define manually
	#include "global_macros.h"
	#define HZ_NO_GLOBAL_MACROS 0
#else
	#define HZ_NO_GLOBAL_MACROS 1
#endif


// if there was no global_macros.h included, do our own mini version
#if defined HZ_NO_GLOBAL_MACROS && HZ_NO_GLOBAL_MACROS

	// Autoconf's config.h. The macro is defined by ./configure automatically.
	#ifdef HAVE_CONFIG_H
		#include <config.h>
	#endif

	// No assumptions on config.h contents - it may not do any of these
	// checks, so don't rely on them. You should use global_macros.h instead,
	// or manually define them.

#endif




// Functions / feature checks


#if defined _WIN32 && defined __GNUC__
	#include <_mingw.h>  // mingw feature macros, used below
#endif



/// \def DISABLE_RTTI
/// Defined to 0 or 1. If 1, RTTI is disabled.
#ifndef DISABLE_RTTI
	// No auto-detection here...
	// There's __GXX_RTTI (not used here; since gcc >= 4.3 (?)), but I'm not sure if it's valid.
	#define DISABLE_RTTI 0
#endif




/// \def DISABLE_EXCEPTIONS
/// Defined to 0 or 1. If 1, exceptions are disabled.
#ifndef DISABLE_EXCEPTIONS
	// some auto-detection (gcc 3.3 or later (I think))
	#if defined __GNUC__ && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && !defined __EXCEPTIONS
		#define DISABLE_EXCEPTIONS 0
	#else
		#define DISABLE_EXCEPTIONS 1
	#endif
#endif



/// \def HAVE_CXX_EXTERN_C_OVERLOAD
/// Defined to 0 or 1. If 1, compiler supports overloading on extern "C" function pointer arguments.
#ifndef HAVE_CXX_EXTERN_C_OVERLOAD
	#ifdef __SUNPRO_CC
		#define HAVE_CXX_EXTERN_C_OVERLOAD 1
	#else
		#define HAVE_CXX_EXTERN_C_OVERLOAD 0
	#endif
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




/// \def HAVE_CXX___func__
/// Defined to 0 or 1. If 1, compiler supports __func__.
#ifndef HAVE_CXX___func__
	// this is a C99 thing, but I don't know of any other compiler which supports it
	#if defined __GNUC__
		#define HAVE_CXX___func__ 1
	#else
		#define HAVE_CXX___func__ 0
	#endif
#endif


/// \def HAVE_CXX___FUNCTION__
/// Defined to 0 or 1. If 1, compiler supports __FUNCTION__.
#ifndef HAVE_CXX___FUNCTION__
	// suncc supports this, but only with extensions enabled (can we check those?)
	#if defined __GNUC__ || defined _MSC_VER
		#define HAVE_CXX___FUNCTION__ 1
	#else
		#define HAVE_CXX___FUNCTION__ 0
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




/// \def HAVE_ISO_STDIO
/// Defined to 0 or 1. If 1, the *printf() family (but not _*printf() or *printf_s()) behaves
/// according to ISO specification in terms of 0-termination, return value
/// and accepting %lld, %llu and %Lf format specifiers.
/// Note: If using mingw runtime >= 3.15 and __USE_MINGW_ANSI_STDIO,
/// mingw supports both C99/POSIX and msvcrt format specifiers.
/// This includes proper printing of long double (%Lf), %lld and %llu, etc...
/// This does _not_ affect _snprintf() and similar non-standard functions.
#ifndef HAVE_ISO_STDIO
	// On win32, assume only with mingw with ansi/iso extensions.
	// for others, assume it's always there.
	#if !defined _WIN32 || ( defined __GNUC__ && \
			(!defined __NO_ISOCEXT || defined __USE_MINGW_ANSI_STDIO) )
		#define HAVE_ISO_STDIO 1
	#else
		#define HAVE_ISO_STDIO 0
	#endif
#endif



/// \def HAVE__SNPRINTF
/// Defined to 0 or 1. If 1, the compiler has Win32's _snprintf (broken 0-termination).
#ifndef HAVE__SNPRINTF
	#if defined _WIN32
		#define HAVE__SNPRINTF 1
	#else
		#define HAVE__SNPRINTF 0
	#endif
#endif


/// \def HAVE__VSNPRINTF
/// Defined to 0 or 1. If 1, the compiler has Win32's _vsnprintf (broken 0-termination).
#ifndef HAVE__VSNPRINTF
	#if defined _WIN32
		#define HAVE__VSNPRINTF 1
	#else
		#define HAVE__VSNPRINTF 0
	#endif
#endif


/// \def HAVE_SNPRINTF_S
/// Defined to 0 or 1. If 1, the compiler has snprintf_s (msvc 2005/8.0).
#ifndef HAVE_SNPRINTF_S
	#if defined _MSC_VER && _MSC_VER >= 1400
		#define HAVE_SNPRINTF_S 1
	#else
		#define HAVE_SNPRINTF_S 0
	#endif
#endif


/// \def HAVE_VSNPRINTF_S
/// Defined to 0 or 1. If 1, the compiler has vsnprintf_s (msvc 2005/8.0).
#ifndef HAVE_VSNPRINTF_S
	#if defined _MSC_VER && _MSC_VER >= 1400
		#define HAVE_VSNPRINTF_S 1
	#else
		#define HAVE_VSNPRINTF_S 0
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
