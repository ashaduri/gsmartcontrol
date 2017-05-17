/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_LAUNCH_URL_H
#define HZ_LAUNCH_URL_H

#include "hz_config.h"  // feature macros

#include <string>
#include <gtk/gtk.h>

#include "scoped_ptr.h"




namespace hz {



/// Open URL in browser or mailto: link in mail client.
/// Return error message on error, empty string otherwise.
/// The link is in utf-8 in windows.
inline std::string launch_url(GtkWindow* window, const std::string& link)
{
	hz::scoped_ptr<GError> error(0, g_error_free);
#if GTK_CHECK_VERSION(3, 22, 0)
	bool status = gtk_show_uri_on_window(window, link.c_str(), GDK_CURRENT_TIME, &error.get_ref());
#else
	bool status = gtk_show_uri(gtk_window_get_screen(window), link.c_str(), GDK_CURRENT_TIME, &error.get_ref());
#endif

	if (!status) {
		return std::string("Cannot open URL: ")
				+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
	}
	return std::string();
}




}  // ns





#endif

/// @}
