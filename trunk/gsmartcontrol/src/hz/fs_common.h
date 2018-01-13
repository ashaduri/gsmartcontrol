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

#ifndef HZ_FS_COMMON_H
#define HZ_FS_COMMON_H

#include "hz_config.h"  // feature macros

// #if defined ENABLE_GLIB && ENABLE_GLIB
// 	#include <glib.h>  // lotsa stuff...
// #endif


// This header is mainly internal.


namespace hz {


/// \var const char DIR_SEPARATOR
/// Directory path separator (character)

/// \var static const char* const DIR_SEPARATOR_S
/// Directory path separator (string)


// these have internal linkage (const integral types).

// there's no point in keeping glib here only for this
// #if defined ENABLE_GLIB && ENABLE_GLIB
// 	const char DIR_SEPARATOR = G_DIR_SEPARATOR;
// 	static const char* const DIR_SEPARATOR_S = G_DIR_SEPARATOR_S;
#if defined _WIN32
	const char DIR_SEPARATOR = '\\';  // no need to be static, it's integral
	static const char* const DIR_SEPARATOR_S = "\\";
#else
	const char DIR_SEPARATOR = '/';
	static const char* const DIR_SEPARATOR_S = "/";
#endif




}  // ns hz



#endif

/// @}
