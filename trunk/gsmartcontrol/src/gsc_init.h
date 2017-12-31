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



#endif

/// @}
