/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
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
This file has effect only if "-include" compiler option doesn't work.

The purpose of this file is to provide some common compatibility
solutions for the whole hz, while not depending on "-include"
compiler switches or pasting autoconf's config.h code in all files.
*/


/**
\namespace hz
HZ library, system-level abstraction and general utilities.
*/


/**
\namespace hz::internal
HZ library internal implementation helpers.
*/


// Define HZ_NO_COMPILER_AUTOINCLUDE=1 from a compiler option to enable
// auto-inclusion of global_macros.h.

#ifndef APP_GLOBAL_MACROS_INCLUDED  // defined in global_macros.h
	#if defined HZ_NO_COMPILER_AUTOINCLUDE && HZ_NO_COMPILER_AUTOINCLUDE
		// define manually
		#include "global_macros.h"
	#else
		#define HZ_NO_GLOBAL_MACROS 1
	#endif
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




#ifndef DISABLE_RTTI
	// No auto-detection here...
	// There's __GXX_RTTI (not used here; since gcc >= 4.3 (?)), but I'm not sure if it's valid.
	#define DISABLE_RTTI 0
#endif




// some auto-detection (gcc 3.3 or later (I think))
#ifndef DISABLE_EXCEPTIONS
	#if defined __GNUC__ && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && !defined __EXCEPTIONS
		#define DISABLE_EXCEPTIONS 0
	#else
		#define DISABLE_EXCEPTIONS 1
	#endif
#endif




#ifndef HAVE_CXX_EXTERN_C_OVERLOAD  // overloading on extern "C" function pointer arguments
	#ifdef __SUNPRO_CC
		#define HAVE_CXX_EXTERN_C_OVERLOAD 1
	#else
		#define HAVE_CXX_EXTERN_C_OVERLOAD 0
	#endif
#endif




#ifndef HAVE_VERBOSE_TERMINATE_HANDLER  // __gnu_cxx::__verbose_terminate_handler
	#if defined __GNUC__
		#define HAVE_VERBOSE_TERMINATE_HANDLER 1
	#else
		#define HAVE_VERBOSE_TERMINATE_HANDLER 0
	#endif
#endif




#ifndef HAVE_GCC_ABI_DEMANGLE  // ::abi::__cxa_demangle
	// This also works with intel/linux (__GNUC__ is defined by default in it).
	#if defined __GNUC__
		#define HAVE_GCC_ABI_DEMANGLE 1
	#else
		#define HAVE_GCC_ABI_DEMANGLE 0
	#endif
#endif




#ifndef HAVE_CXX___func__
	// this is a C99 thing, but I don't know of any other compiler which supports it
	#if defined __GNUC__
		#define HAVE_CXX___func__ 1
	#else
		#define HAVE_CXX___func__ 0
	#endif
#endif

#ifndef HAVE_CXX___FUNCTION__
	// suncc supports this, but only with extensions enabled (can we check those?)
	#if defined __GNUC__ || defined _MSC_VER
		#define HAVE_CXX___FUNCTION__ 1
	#else
		#define HAVE_CXX___FUNCTION__ 0
	#endif
#endif




// If some of the functions have different requirements, they are listed separately.
#ifndef HAVE_WIN_SE_FUNCS  // Win32's *_s() functions (since msvc 2005/8.0)
	#if defined _MSC_VER && _MSC_VER >= 1400
		#define HAVE_WIN_SE_FUNCS 1
	#else
		#define HAVE_WIN_SE_FUNCS 0
	#endif
#endif




#ifndef HAVE_POSIX_OFF_T_FUNCS  // fseeko/ftello. See fs_file.h for details.
	// It's quite hard to detect, so we just enable for any non-win32.
	#if defined _WIN32
		#define HAVE_POSIX_OFF_T_FUNCS 0
	#else
		#define HAVE_POSIX_OFF_T_FUNCS 1
	#endif
#endif




#ifndef HAVE_WIN_LFS_FUNCS  // LFS on win32
	// enable if it's msvc >= 2005, or we're linking to newer msvcrt with mingw.
	#if defined _WIN32 && ( (defined _MSC_VER >= 1400) \
			|| (defined __GNUC__ && defined __MSVCRT_VERSION__ && __MSVCRT_VERSION__ >= 0x800) )
		#define HAVE_WIN_LFS_FUNCS 1
	#else
		#define HAVE_WIN_LFS_FUNCS 0
	#endif
#endif




#ifndef HAVE_XSI_STRERROR_R  // XSI-style strerror_r()
	#if ((defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L) \
			|| (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 600)) && !defined _GNU_SOURCE
		#define HAVE_XSI_STRERROR_R 1
	#else
		#define HAVE_XSI_STRERROR_R 0
	#endif
#endif

#ifndef HAVE_GNU_STRERROR_R  // GNU-style strerror_r()
	// _GNU_SOURCE excludes HAVE_XSI_STRERROR_R
	#if defined _GNU_SOURCE
		#define HAVE_GNU_STRERROR_R 1
	#else
		#define HAVE_GNU_STRERROR_R 0
	#endif
#endif




// Whether the *printf() family (but not _*printf() or *printf_s()) behaves
// according to ISO specification in terms of 0-termination, return value
// and accepting %lld, %llu and %Lf format specifiers.

// Note: If using mingw runtime >= 3.15 and __USE_MINGW_ANSI_STDIO,
// mingw supports both C99/POSIX and msvcrt format specifiers.
// This includes proper printing of long double (%Lf), %lld and %llu, etc...
// This does _not_ affect _snprintf() and similar non-standard functions.

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




#ifndef HAVE__SNPRINTF  // Win32's _snprintf (broken 0-termination)
	#if defined _WIN32
		#define HAVE__SNPRINTF 1
	#else
		#define HAVE__SNPRINTF 0
	#endif
#endif


#ifndef HAVE__VSNPRINTF  // Win32's _vsnprintf (broken 0-termination)
	#if defined _WIN32
		#define HAVE__SNPRINTF 1
	#else
		#define HAVE__SNPRINTF 0
	#endif
#endif


#ifndef HAVE_SNPRINTF_S  // snprintf_s (msvc 2005/8.0)
	#if defined _MSC_VER && _MSC_VER >= 1400
		#define HAVE_SNPRINTF_S 1
	#else
		#define HAVE_SNPRINTF_S 0
	#endif
#endif


#ifndef HAVE_VSNPRINTF_S  // vsnprintf_s (msvc 2005/8.0)
	#if defined _MSC_VER && _MSC_VER >= 1400
		#define HAVE_VSNPRINTF_S 1
	#else
		#define HAVE_VSNPRINTF_S 0
	#endif
#endif


#ifndef HAVE_VASPRINTF  // vasprintf() (GNU/glibc extension)
	#if defined _GNU_SOURCE
		#define HAVE_VASPRINTF 1
	#else
		#define HAVE_VASPRINTF 0
	#endif
#endif




#ifndef HAVE_REENTRANT_LOCALTIME
	// win32 and solaris localtime() is reentrant
	#if defined _WIN32 || defined sun || defined __sun
		#define HAVE_REENTRANT_LOCALTIME 1
	#else
		#define HAVE_REENTRANT_LOCALTIME 0
	#endif
#endif




// setenv, unsetenv
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
