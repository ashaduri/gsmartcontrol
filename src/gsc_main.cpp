/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <iostream>  // cerr
#include <exception>  // std::exception, std::set_terminate()
#include <cstdlib>  // EXIT_*

#include "hz/hz_config.h"  // HAVE_VERBOSE_TERMINATE_HANDLER

#include "gsc_init.h"  // app_init_and_loop()



int main (int argc, char** argv)
{
	// we still leave __GNUC__ for autoconf-less setups.
#if defined HAVE_VERBOSE_TERMINATE_HANDLER || defined __GNUC__
	// Verbose uncaught exception handler
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
#else
	try {
#endif

	// initialize stuff and enter the main loop
	if (!app_init_and_loop(argc, argv))
		return EXIT_FAILURE;


// print uncaught exceptions for non-gcc-compatible
#if !(defined HAVE_VERBOSE_TERMINATE_HANDLER || defined __GNUC__)
	}
	catch(std::exception& e) {
		// don't use anything other than cerr here, it's the most safe option.
		std::cerr << "main(): Unhandled exception: " << e.what() << std::endl;

		return EXIT_FAILURE;
	}
	catch(...) {  // this guarantees proper unwinding in case of unhandled exceptions (win32 I think)
		std::cerr << "main(): Unhandled unknown exception." << std::endl;

		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}






