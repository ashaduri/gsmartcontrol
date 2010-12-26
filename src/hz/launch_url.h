/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_LAUNCH_URL_H
#define HZ_LAUNCH_URL_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstdlib>  // std::getenv()
#include <glib.h>  // g_*
#include <gdk/gdk.h>  // gdk_spawn_command_line_on_screen, GdkScreen

#ifdef _WIN32
	#include <windows.h>  // ShellExecuteA()
#endif

#include "scoped_ptr.h"


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
	inline std::string launch_url(const std::string& link, GdkScreen* screen = 0)
	{
#ifdef _WIN32
		if (ShellExecuteA(0, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL) > reinterpret_cast<HINSTANCE>(32)) {
			return std::string();
		}
		return "Internal error occurred while executing a command.";

#else
		bool is_email = (link.compare(0, 7, "mailto:") == 0);

		// susehelp lists this, with alternative being TEXTBROWSER
		const char* browser = std::getenv("XBROWSER");
		if (!browser || *browser == '\0')
			browser = std::getenv("BROWSER");  // this is the common method

		// try xfce first - it has the most sensible launcher.
		if (!browser || *browser == '\0')
			browser = "exo-open";

		std::string qlink, qbrowser;

		{
			hz::scoped_ptr<gchar> qbrowser_cstr(g_shell_quote(browser), g_free);  // will this break its parameters?
			if (qbrowser_cstr)
				qbrowser = qbrowser_cstr.get();

			hz::scoped_ptr<gchar> qlink_cstr(g_shell_quote(link.c_str()), g_free);
			if (qlink_cstr)
				qlink = qlink_cstr.get();
		}

		std::string command = qbrowser + " " + qlink;

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
			error_msg = std::string("An error occurred while executing a command")
					+ ((error && error->message) ? (std::string(": ") + error->message) : ".");
		}

		return error_msg;
#endif
	}





}  // ns





#endif
