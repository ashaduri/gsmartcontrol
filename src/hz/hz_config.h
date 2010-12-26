/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_HZ_CONFIG_H
#define HZ_HZ_CONFIG_H

// This file is included from all hz headers.

// You should include this file if you're using any of:
// macros from autoconf-generated config.h
// macros from global_macros.h
// This file has effect only if "-include" compiler option doesn't work.

// The purpose of this file is to provide some common compatibility
// solutions for the whole hz, while not depending on "-include"
// compiler switches or pasting autoconf's config.h code in all files.


// Define HZ_NO_COMPILER_AUTOINCLUDE from a compiler option to enable
// auto-inclusion of global_macros.h.


#ifndef APP_GLOBAL_MACROS_INCLUDED  // defined in global_macros.h
	#ifdef HZ_NO_COMPILER_AUTOINCLUDE  // define manually
		#include "global_macros.h"
	#else
		#define HZ_NO_GLOBAL_MACROS
	#endif
#endif


// if there was no global_macros.h included, do our own mini version
#ifdef HZ_NO_GLOBAL_MACROS

	// Autoconf's config.h. The macro is defined by ./configure automatically.
	#ifdef HAVE_CONFIG_H
		#include <config.h>
	#endif

	// No assumptions on config.h contents - it may not do any of these
	// checks, so don't rely on them. You should use global_macros.h instead,
	// or manually define them.

#endif







#endif
