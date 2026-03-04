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

#ifndef GSC_INIT_H
#define GSC_INIT_H

#include <string>


/// Initialize the application and run the main loop
bool app_init_and_loop(int& argc, char**& argv);


/// Quit the application (exit the main loop)
void app_quit();


/// Return everything that went through libdebug's channels.
/// Useful for showing logs.
std::string app_get_debug_buffer_str();


/// Get the fractional scaling percentage detected on Windows (0 if not detected).
/// For example, at 150% scaling, this returns 50; at 125% scaling, this returns 25.
int app_get_windows_fractional_scaling_percent();



#endif

/// @}
