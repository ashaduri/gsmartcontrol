/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_EXCEPTIONS_H
#define HZ_EXCEPTIONS_H

#include "hz_config.h"  // feature macros

// These headers may be needed for object creation even if exceptions are disabled.
// #include <exception>  // std::exception
// #include <stdexcept>  // standard exceptions, derived from std::exception



// some auto-detection (gcc 3.3 or later (I think))
#ifdef __GNUC__
	#if ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && !defined __EXCEPTIONS
		#define DISABLE_EXCEPTIONS
	#endif
#endif


// Define DISABLE_EXCEPTIONS to disable exception use
// (only applicable when using the macros defined below).
// Useful for e.g. gcc's -fno-exceptions, etc...


#ifndef DISABLE_EXCEPTIONS


	// If you use -fno-exceptions gcc switch (or similar), define
	// DISABLE_EXCEPTIONS and no throw statement will occur if
	// you use THROW_FATAL instead of throw.

	// Note: The exceptions MUST have a what() member function
	// for this to work.


	#define THROW_FATAL(ex) \
		throw (ex)

	// same as above, but no abort() in case of no-exceptions
	#define THROW_WARN(ex) \
		throw (ex)


#else  // no-exceptions alternative:


	#include <cstdlib>  // std::exit(), EXIT_FAILURE
	#include <iostream>  // std::cerr


	#define THROW_FATAL(ex) \
		if (true) { \
			std::cerr << "Fatal exception thrown (exceptions are disabled): " << ex.what() << std::endl; \
			std::exit(EXIT_FAILURE); \
		} else (void)0

	#define THROW_WARN(ex) \
		if (true) { \
			std::cerr << "Warn exception thrown (exceptions are disabled): " << ex.what() << std::endl; \
		} else (void)0


#endif





#endif
