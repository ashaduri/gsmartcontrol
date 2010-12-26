/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

/*
This file serves as a compile-time configuration for various
library components.

This file is not included directly, but through the compiler option
-include global_macros.h
*/


// Include autoconf's config.h.
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif


// So that others may check if "-include" works
#define APP_GLOBAL_MACROS_INCLUDED


// #ifdef _WIN32
	// needed for hz::send_process_signal(), winxp or later.
// 	#define WINVER 0x0501
// #endif


// List most of the included system headers here - they will
// be precompiled together with this file.

#ifdef ENABLE_PCH

	#ifdef HAVE_STDC___H
		// All libstdc++ headers
		#include <stdc++.h>

	#else
		#include <string>

		#include <vector>
		#include <list>
		#include <stack>
		#include <map>

		#include <iosfwd>
		#include <sstream>
		#include <iomanip>
		#include <fstream>
		#include <iostream>
		#include <ios>

		#include <limits>
		#include <locale>
		#include <exception>
		#include <stdexcept>

		#include <cctype>
		#include <cstdlib>
		#include <cassert>
		#include <cerrno>
		#include <cstddef>
		#include <cstring>

		#include <cxxabi.h>  // is this included in stdc++.h?
	#endif

	// most of the other headers
	#include <stdio.h>
	#include <stdint.h>
	#include <sys/types.h>
	#include <signal.h>
	#include <dirent.h>

	#ifdef ENABLE_GLIB
		#include <glib.h>
	#endif
	#ifdef ENABLE_GLIBMM
		#include <sigc++/sigc++.h>
		#include <gtkmm.h>  // if we have glibmm, we're building everything
		#include <gtk/gtk.h>
		#include <glibmm.h>
	#endif

	#ifdef _WIN32
		#include <windows.h>
	#endif

#endif  // pch



#ifdef HAVE_LONG_LONG_INT
	// pcrecpp uses this
	#ifndef HAVE_LONG_LONG
		#define HAVE_LONG_LONG
	#endif

#else  // there is no long long int
	// explicitly disable it in our code. by default, it assumes that long long int exists.
	#define DISABLE_LL_INT

#endif


#ifdef HAVE_UNSIGNED_LONG_LONG_INT
	// pcrecpp uses this
	#ifndef HAVE_UNSIGNED_LONG_LONG
		#define HAVE_UNSIGNED_LONG_LONG
	#endif

#else  // there is no long long int
	// explicitly disable it in our code. by default, it assumes that long long int exists.
	#define DISABLE_ULL_INT

#endif



// HAVE_func means that func doesn't throw undefined symbol.
// HAVE_DECL_func means that it's declared in a header.
// HAVE_DECL_func is always defined as 0 or 1.

// check if it's not in stdlib.h
#if defined HAVE_DECL_STRTOF && !HAVE_DECL_STRTOF
// 	#ifdef HAVE_STRTOF  // it's not in header, but it is in a library
// 		float strtof(const char *nptr, char **endptr);
// 	#else  // neither header, nor library
		#define DISABLE_STRTOF
// 	#endif
#endif


#if defined HAVE_DECL_STRTOLD && !HAVE_DECL_STRTOLD
// 	#ifdef HAVE_STRTOLD
// 		long double strtold(const char *nptr, char **endptr);
// 	#else
		#define DISABLE_STRTOLD
// 	#endif
#endif




// -- Make most file operations work with large files (replaces
// off_t -> off64_t, stat -> stat64, fopen -> fopen64, etc...).
// Works on glibc. Automatically defined by autoconf.
// #define _FILE_OFFSET_BITS 64


// -- These have effect in rmn, hz error library, hz string_tools.
// These will be defined automatically by autoconf if found.
// #define ENABLE_GLIB
// #define ENABLE_GLIBMM


// -- RMN settings - see rmn/rmn.h for details
// #define RMN_TYPE_TRACKING


// Note: We use DISABLE_* because all the libraries should assume
// complete C++ support, unless indicated otherwise. This also
// makes the headers work in full when there is no config.h.

// -- Define this if using -fno-rtti or similar (automatic from autoconf).
#ifndef HAVE_RTTI
	#define DISABLE_RTTI
#endif

// -- Define this if using -fno-exceptions (automatic from autoconf).
#ifndef HAVE_EXCEPTIONS
	#define DISABLE_EXCEPTIONS
#endif



// -- Default policy for synchronization primitives (sync.h);
// (define only one of these):

// check if any of them are forced
#if (!defined HZ_SYNC_DEFAULT_POLICY_BOOST) \
		&& (!defined HZ_SYNC_DEFAULT_POLICY_GLIB) \
		&& (!defined HZ_SYNC_DEFAULT_POLICY_GLIBMM) \
		&& (!defined HZ_SYNC_DEFAULT_POLICY_POCO) \
		&& (!defined HZ_SYNC_DEFAULT_POLICY_PTHREAD) \
		&& (!defined HZ_SYNC_DEFAULT_POLICY_WIN32)

	#if defined ENABLE_GLIB
		#define HZ_SYNC_DEFAULT_POLICY_GLIB
	#elif defined _WIN32
		#define HZ_SYNC_DEFAULT_POLICY_WIN32
	#else
		#define HZ_SYNC_DEFAULT_POLICY_PTHREAD
	#endif

	// #define HZ_SYNC_DEFAULT_POLICY_NONE
	// #define HZ_SYNC_DEFAULT_POLICY_GLIBMM
	// #define HZ_SYNC_DEFAULT_POLICY_BOOST
	// #define HZ_SYNC_DEFAULT_POLICY_POCO

#endif



// -- Default policy for thread-local storage pointer (tls.h);
// (define only one of these):

// check if any of them are forced
#if (!defined HZ_TLS_DEFAULT_POLICY_BOOST) \
		&& (!defined HZ_TLS_DEFAULT_POLICY_GLIB) \
		&& (!defined HZ_TLS_DEFAULT_POLICY_PTHREAD) \
		&& (!defined HZ_TLS_DEFAULT_POLICY_WIN32)

	#if defined ENABLE_GLIB
		#define HZ_TLS_DEFAULT_POLICY_GLIB

	#elif defined _WIN32
		// use pthread-win32 or boost on win32.
		// our win32 policy lacks good cleanup function support.
		#define HZ_TLS_DEFAULT_POLICY_PTHREAD

	#else
		#define HZ_TLS_DEFAULT_POLICY_PTHREAD
	#endif

	// #define HZ_TLS_DEFAULT_POLICY_BOOST
	// #define HZ_TLS_DEFAULT_POLICY_WIN32

#endif



// -- hz/debug.h settings.
//
#ifndef HZ_USE_LIBDEBUG
	#define HZ_USE_LIBDEBUG 1
#endif

// or
// #ifndef HZ_EMULATE_LIBDEBUG
// 	#define HZ_EMULATE_LIBDEBUG 1
// #endif
// or none of the above to disable debug output completely.


// -- hz/i18n.h settings.
//
#ifndef ENABLE_NLS
	#define ENABLE_NLS 0
#endif


// -- hz/res_data.h settings.
//
#ifndef HZ_ENABLE_COMPILED_RES_DATA
	#define HZ_ENABLE_COMPILED_RES_DATA 1
#endif


// -- increased verbosity levels, etc...; better define this through compiler option.
// #define DEBUG_BUILD

// This enables reference count tracing (very verbose)
// #define INTRUSIVE_PTR_REF_TRACING

// This enables runtime checks for errors (with exception throwing)
#ifdef DEBUG_BUILD
	#define INTRUSIVE_PTR_RUNTIME_CHECKS
#endif





#endif
