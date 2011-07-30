/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_PANGO_UTILS_H
#define APP_PANGO_UTILS_H

#include <string>
#include <glibmm.h>  // ustring



/// Strip a string of all markup tags.
/// \return false if there was some error.
bool app_pango_strip_markup(const Glib::ustring& str, Glib::ustring& stripped);


/// Strip a string of all markup tags.
/// \return false if there was some error.
bool app_pango_strip_markup(const std::string& str, std::string& stripped);





#endif

/// @}
