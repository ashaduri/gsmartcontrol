/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // NOT NEEDED
#undef throw

#include <pango/pango.h>  // pango_parse_markup()

#include "app_pango_utils.h"



// Returns true if markup was successfully stripped
bool app_pango_strip_markup(const Glib::ustring& str, Glib::ustring& stripped)
{
	gchar* gstripped = 0;
	bool ok = false;
	if (pango_parse_markup(str.c_str(), -1, 0, nullptr, &gstripped, nullptr, nullptr) && gstripped) {
		stripped = gstripped;
		ok = true;
	}
	g_free(gstripped);

	// alternative method, check if it works (AttrList has bool() cast since gtkmm 2.10):
	// char dummy = 0;
	// if (Pango::AttrList(str, 0, stripped, dummy).gobj()) {
	//	return true;
	// }

	return ok;
}



bool app_pango_strip_markup(const std::string& str, std::string& stripped)
{
	gchar* gstripped = 0;
	bool ok = false;
	if (pango_parse_markup(str.c_str(), -1, 0, nullptr, &gstripped, nullptr, nullptr) && gstripped) {
		stripped = gstripped;
		ok = true;
	}
	g_free(gstripped);
	return ok;
}






/// @}
