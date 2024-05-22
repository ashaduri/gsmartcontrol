/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#include <cstdlib>  // EXIT_*

#include "hz/win32_tools.h"  // hz::win32_*
#include "hz/main_tools.h"

#include "gsc_init.h"  // app_init_and_loop()



/// Application main function
int main(int argc, char** argv)
{
	return hz::main_exception_wrapper([&argc, &argv]()
	{
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
		if (!app_init_and_loop(argc, argv)) {
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	});
}





/// @}
