/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_LAUNCH_URL_H
#define HZ_LAUNCH_URL_H

#include <string>
#include <gtk/gtk.h>

#ifdef _WIN32
	#include <windows.h>  // seems to be needed by shellapi.h
	#include <shellapi.h>  // ShellExecuteW()
	#include "win32_tools.h"  // hz::win32_utf8_to_utf16
#else
	#include <memory>
#endif




namespace hz {



/// Open URL in browser or mailto: link in mail client.
/// Return error message on error, empty string otherwise.
/// The link is in utf-8 in windows.
inline std::string launch_url(GtkWindow* window, const std::string& link)
{
	// For some reason, gtk_show_uri() crashes on windows, so use our manual implementation.
#ifdef _WIN32
	std::wstring wlink = hz::win32_utf8_to_utf16(link);
	if (wlink.empty())
		return "Error while executing a command: The specified URI contains non-UTF-8 characters.";

	HINSTANCE inst = ShellExecuteW(0, L"open", wlink.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	if (inst > reinterpret_cast<HINSTANCE>(32)) {
		return std::string();
	}
	return "Error while executing a command: Internal error.";

#else

	GError* error = nullptr;
#if GTK_CHECK_VERSION(3, 22, 0)
	bool status = gtk_show_uri_on_window(window, link.c_str(), GDK_CURRENT_TIME, &error);
#else
	GdkScreen* screen = (window ? gtk_window_get_screen(window) : nullptr);
	bool status = (bool)gtk_show_uri(screen, link.c_str(), GDK_CURRENT_TIME, &error);
#endif
	std::unique_ptr<GError, decltype(&g_error_free)> uerror(error, &g_error_free);

	if (!status) {
		return std::string("Cannot open URL: ")
				+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
	}
	return std::string();
#endif
}




}  // ns





#endif

/// @}
