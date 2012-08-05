/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#include <iostream>  // cerr
#include <exception>  // std::exception, std::set_terminate()
#include <cstdlib>  // EXIT_*

#include "hz/hz_config.h"  // HAVE_VERBOSE_TERMINATE_HANDLER
#include "hz/win32_tools.h"  // hz::win32_*

#include "gsc_init.h"  // app_init_and_loop()



/// Application main function
int main(int argc, char* argv[])
{
	// we still leave __GNUC__ for autoconf-less setups.
#if defined HAVE_VERBOSE_TERMINATE_HANDLER && HAVE_VERBOSE_TERMINATE_HANDLER
	// Verbose uncaught exception handler
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
#else
	try {
#endif

	// disable "Send to MS..." dialog box in non-debug builds
#if defined _WIN32 && !(defined DEBUG_BUILD && DEBUG_BUILD)
	SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

	// debug builds already have a console, no need to create one.
#if defined _WIN32 && !(defined DEBUG_BUILD && DEBUG_BUILD)
	// if the console is not open, or unsupported (win2k), use files.
	if (!hz::win32_redirect_stdio_to_console()) {  // redirect stdout/stderr to console (if open and supported)
		hz::win32_redirect_stdio_to_files();  // redirect stdout/stderr to output files
	}
#endif

	// initialize stuff and enter the main loop
	if (!app_init_and_loop(argc, argv))
		return EXIT_FAILURE;


// print uncaught exceptions for non-gcc-compatible
#if !(defined HAVE_VERBOSE_TERMINATE_HANDLER && HAVE_VERBOSE_TERMINATE_HANDLER)
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







/// @}
