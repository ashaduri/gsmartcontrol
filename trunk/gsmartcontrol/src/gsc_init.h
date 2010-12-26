/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_INIT_H
#define GSC_INIT_H

#include <string>


bool app_init_and_loop(int& argc, char**& argv);


void app_quit();


// Returns everything that went through libdebug's channels.
std::string app_get_debug_buffer_str();



#endif
