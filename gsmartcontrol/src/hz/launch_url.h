/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_LAUNCH_URL_H
#define HZ_LAUNCH_URL_H

#include "hz_config.h"  // feature macros

#include <string>
#include <glib.h>  // g_*
#include <gdk/gdk.h>  // gdk_spawn_command_line_on_screen, GdkScreen

#ifdef _WIN32
	#include <windows.h>  // seems to be needed by shellapi.h
	#include <shellapi.h>  // ShellExecuteW()
	#include "scoped_array.h"
	#include "win32_tools.h"  // hz::win32_utf8_to_utf16
#else
	#include "scoped_ptr.h"
	#include "env_tools.h"
#endif




// Note: Glib / GDK only.

// TODO: Use gtk_show_uri() if gtk 2.14.


namespace hz {



	namespace internal {

		inline bool launch_url_helper_do_launch(const std::string& command, GError** errorptr, GdkScreen* screen)
		{
			if (screen)
				return gdk_spawn_command_line_on_screen(screen, command.c_str(), errorptr);
			return g_spawn_command_line_async(command.c_str(), errorptr);
		}

	}



	// Open URL in browser or mailto: link in mail client.
	// Return error message on error, empty string otherwise.
	// The link is in utf-8 in windows.
	inline std::string launch_url(const std::string& link, GdkScreen* screen = 0)
	{
		if (link.empty())
			return "Error while executing a command: Empty URI specified.";

#ifdef _WIN32
		hz::scoped_array<wchar_t> wlink(hz::win32_utf8_to_utf16(link.c_str()));
		if (!wlink)
			return "Error while executing a command: The specified URI contains non-UTF-8 characters.";

		HINSTANCE inst = ShellExecuteW(0, L"open", wlink.get(), NULL, NULL, SW_SHOWNORMAL);
		if (inst > reinterpret_cast<HINSTANCE>(32)) {
			return std::string();
		}
		return "Error while executing a command: Internal error.";

#else
		bool is_email = (link.compare(0, 7, "mailto:") == 0);

		std::string browser;

		// susehelp lists this, with alternative being TEXTBROWSER
		hz::env_get_value("XBROWSER", browser);

		if (browser.empty())
			hz::env_get_value("BROWSER", browser);  // this is the common method

		// try xfce first - it has the most sensible launcher.
		if (browser.empty())
			browser = "exo-open";

		std::string qlink;

		{
			// will this break its embedded parameters?
			hz::scoped_ptr<gchar> browser_cstr(g_shell_quote(browser.c_str()), g_free);
			if (browser_cstr)
				browser = browser_cstr.get();

			hz::scoped_ptr<gchar> qlink_cstr(g_shell_quote(link.c_str()), g_free);
			if (qlink_cstr)
				qlink = qlink_cstr.get();
		}

		std::string command = browser + " " + qlink;

		hz::scoped_ptr<GError> error(0, g_error_free);
		bool status = internal::launch_url_helper_do_launch(command.c_str(), &error.get_ref(), screen);

		if (!status) {  // try kde4
			status = internal::launch_url_helper_do_launch(std::string("kde-open ") + qlink, 0, screen);
		}

		if (!status) {  // try kde3
			// launches both konq and kmail on mailto:.
			status = internal::launch_url_helper_do_launch(std::string("kfmclient openURL") + qlink, 0, screen);
		}

		if (!status) {  // try gnome
			// errors out with "no handler" on mailto: on my system.
			status = internal::launch_url_helper_do_launch(std::string("gnome-open ") + qlink, 0, screen);
		}

		if (!status && !is_email) {  // try XDG
			// doesn't support emails at all.
			status = internal::launch_url_helper_do_launch(std::string("xdg-open ") + qlink, 0, screen);
		}

		// we use the error of the first command, because it could have user-specified.
		std::string error_msg;
		if (!status) {
			error_msg = std::string("Error while executing a command: ")
					+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
		}

		return error_msg;
#endif
	}





}  // ns





#endif
