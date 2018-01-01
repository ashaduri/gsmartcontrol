/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef GLOBAL_MACROS_H
#define GLOBAL_MACROS_H

/**
\file
This file serves as a compile-time configuration for various
library components.

This file is included from hz_config.h.
Additionally, it may be included through compiler's "-include" option
(if supported) for pch support.
*/


// Include autoconf's config.h.
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif


/// So that others may check whether this file was included or not.
#define APP_GLOBAL_MACROS_INCLUDED 1



// -- Make most file operations work with large files (replaces
// off_t -> off64_t, stat -> stat64, fopen -> fopen64, etc...).
// Works on glibc. Automatically defined by autoconf.
// #define _FILE_OFFSET_BITS 64


// -- These have effect in rmn, hz error library, hz string_tools.
// These will be defined automatically by autoconf if found.
// #define ENABLE_GLIB 1
// #define ENABLE_GLIBMM 1


// Note: We use DISABLE_* because all the libraries should assume
// complete C++ support, unless indicated otherwise. This also
// makes the headers work in full when there is no config.h.



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
#if defined DEBUG_BUILD && !defined INTRUSIVE_PTR_RUNTIME_CHECKS
	#define INTRUSIVE_PTR_RUNTIME_CHECKS 1
#endif





#endif
