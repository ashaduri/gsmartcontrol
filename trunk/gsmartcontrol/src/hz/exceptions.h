/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_EXCEPTIONS_H
#define HZ_EXCEPTIONS_H

#include "hz_config.h"  // feature macros

// These headers may be needed for object creation even if exceptions are disabled.
// #include <exception>  // std::exception
// #include <stdexcept>  // standard exceptions, derived from std::exception



// Define DISABLE_EXCEPTIONS=1 to disable exception use
// (only applicable when using the macros defined below).
// Useful for e.g. gcc's -fno-exceptions, etc...


#if !(defined DISABLE_EXCEPTIONS && DISABLE_EXCEPTIONS)


	// If you use -fno-exceptions gcc switch (or similar), define
	// DISABLE_EXCEPTIONS and no throw statement will occur if
	// you use THROW_FATAL instead of throw.

	// Note: The exceptions MUST have a what() member function
	// for this to work.

	// Do NOT put ex into parentheses. gcc-3.3 gives syntax errors about that (huh?).

	#define THROW_FATAL(ex) \
		throw ex

	// same as above, but no abort() in case of no-exceptions
	#define THROW_WARN(ex) \
		throw ex


#else  // no-exceptions alternative:


	#include <cstdlib>  // std::abort()
	#include <cstdio>  // std::fprintf
	// #include <iostream>  // std::cerr

	// We could use std::exit(EXIT_FAILURE), but it's somewhat inconsistent
	// in regards of stack unwinding and destructors, so use abort()
	// std::cerr << "Fatal exception thrown (exceptions are disabled): " << ex.what() << std::endl;
	#define THROW_FATAL(ex) \
		if (true) { \
			const char* ex_what = ex.what(); \
			std::fprintf(stderr, "Fatal exception thrown (exceptions are disabled): %s\n", ex_what ? ex_what : "[unknown]"); \
			std::abort(); \
		} else (void)0

	// std::cerr << "Warn exception thrown (exceptions are disabled): " << ex.what() << std::endl;
	#define THROW_WARN(ex) \
		if (true) { \
			std::fprintf(stderr, "Warn exception thrown (exceptions are disabled): %s\n", ex_what ? ex_what : "[unknown]"); \
		} else (void)0


#endif





#endif
