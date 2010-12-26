/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_PANGO_UTILS_H
#define APP_PANGO_UTILS_H

#include <string>
#include <glibmm.h>  // ustring



// strip markup of a string. returns true on success.
bool app_pango_strip_markup(const Glib::ustring& str, Glib::ustring& stripped);


bool app_pango_strip_markup(const std::string& str, std::string& stripped);





#endif
