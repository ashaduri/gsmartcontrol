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

#include "local_glibmm.h"
#include <gtkmm.h>
#include <glib.h>  // g_, G*

#include <string>
// #include <locale.h>  // _configthreadlocale (win32)
#include <stdexcept>  // std::runtime_error
#include <cstdio>  // std::printf
#include <vector>
#include <sstream>
#include <limits>
#include <memory>
#include <cmath>

#ifdef _WIN32
	#include <windows.h>
	#include <versionhelpers.h>
#endif

#include "libdebug/libdebug.h"  // include full libdebug here (to add domains, etc...)
#include "rconfig/rconfig.h"
#include "rconfig/loadsave.h"
#include "rconfig/autosave.h"
#include "hz/data_file.h"  // data_file_add_search_directory
#include "hz/fs.h"
#include "hz/locale_tools.h"  // locale_c*
#include "hz/string_algo.h"  // string_join()
#include "hz/win32_tools.h"  // win32_get_registry_value_string()
#include "hz/env_tools.h"
#include "hz/string_num.h"
#include "build_config.h"  // VERSION, *PACKAGE*, ...

#include "applib/window_instance_manager.h"
#include "gsc_main_window.h"
#include "gsc_executor_log_window.h"
#include "gsc_settings.h"
#include "gsc_init.h"
#include "gsc_startup_settings.h"



namespace {

	/// Config file in user's HOME
	inline const hz::fs::path& get_home_config_file()
	{
		static hz::fs::path home_config_file = hz::fs_get_user_config_dir() / "gsmartcontrol" / "gsmartcontrol2.conf";
		return home_config_file;
	}



	/// Libdebug channel buffer stream
	inline std::ostringstream& get_debug_buf_channel_stream()
	{
		static std::ostringstream stream;
		return stream;
	}


	/// Get libdebug buffer channel (create new one if unavailable).
	inline DebugChannelBasePtr get_debug_buf_channel()
	{
		static DebugChannelBasePtr channel = std::make_shared<DebugChannelOStream>(get_debug_buf_channel_stream());
		return channel;
	}

}



std::string app_get_debug_buffer_str()
{
	return get_debug_buf_channel_stream().str();
}





namespace {


	/// Find the configuration files and load them.
	inline bool app_init_config()
	{
		// Default system-wide settings. This file is empty by default.
		hz::fs::path global_config_file;
	#ifdef _WIN32
		global_config_file = hz::fs::u8path("gsmartcontrol2.conf");  // CWD, installation dir by default.
	#else
		global_config_file = hz::fs::u8path(PACKAGE_SYSCONF_DIR) / "gsmartcontrol2.conf";
	#endif

		debug_out_dump("app", DBG_FUNC_MSG << "Global config file: \"" << global_config_file.u8string() << "\"\n");
		debug_out_dump("app",
				DBG_FUNC_MSG << "Local config file: \"" << get_home_config_file().u8string() << "\"\n");

		// load global first
		std::error_code ec;
		if (hz::fs::exists(global_config_file, ec) && hz::fs_path_is_readable(global_config_file, ec)) {
			rconfig::load_from_file(global_config_file);
		}

		// load local
		if (hz::fs::exists(get_home_config_file(), ec) && hz::fs_path_is_readable(get_home_config_file(), ec)) {
			rconfig::load_from_file(get_home_config_file());

		} else {
			// create the parent directories of the config file
			hz::fs::path config_loc = get_home_config_file().parent_path();
			if (!hz::fs::exists(config_loc, ec)) {
				hz::fs::create_directories(config_loc, ec);
				hz::fs::permissions(config_loc, hz::fs::perms::owner_all, ec);
			}
		}

		init_default_settings();  // initialize /default

		rconfig::dump_config();

		rconfig::autosave_set_config_file(get_home_config_file());
		int autosave_timeout_sec = rconfig::get_data<int>("system/config_autosave_timeout_sec");
		if (autosave_timeout_sec > 0) {
			rconfig::autosave_start(std::chrono::seconds(autosave_timeout_sec));
		}

		return true;
	}


/*
	/// If it's the first time the application was started by this user, show a message.
	inline void app_show_first_boot_message(Gtk::Window* parent)
	{
		bool first_boot = rconfig::get_data<bool>("system/first_boot");

		if (first_boot) {
	// 		Glib::ustring msg = "First boot";
	// 		Gtk::MessageDialog(*parent, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true).run();
		}

	// 	rconfig::set_data("system/first_boot", false);  // don't show it again
	}
*/

}  // anon. ns



extern "C" {
	/// Glib message -> libdebug message convertor
	static void glib_message_handler([[maybe_unused]] const gchar* log_domain,
			[[maybe_unused]] GLogLevelFlags log_level,
			const gchar* message,
			[[maybe_unused]] gpointer user_data)
	{
		// log_domain is already printed as part of message.
		switch(log_level) {
			case G_LOG_FLAG_RECURSION:
			case G_LOG_FLAG_FATAL:
			case G_LOG_LEVEL_ERROR:  // fatal
			case G_LOG_LEVEL_CRITICAL:
				debug_print_error("gtk", "%s\n", message);
				break;
			case G_LOG_LEVEL_WARNING:
				debug_print_warn("gtk", "%s\n", message);
				break;
			case G_LOG_LEVEL_MESSAGE:
			case G_LOG_LEVEL_INFO:
				debug_print_info("gtk", "%s\n", message);
				break;
			case G_LOG_LEVEL_DEBUG:
				debug_print_dump("gtk", "%s\n", message);
				break;
			case G_LOG_LEVEL_MASK:
				break;
		}
	}
}




namespace {


	/// Command-line argument values
	struct CmdArgs {
		// Note: Use GLib types here:
		gboolean arg_locale = TRUE;  ///< if false, disable using system locale
		gboolean arg_version = FALSE;  ///< if true, show version and exit
		gboolean arg_scan = TRUE;  ///< if false, don't scan the system for drives on startup
		gboolean arg_hide_tabs = TRUE;  ///< if true, hide additional info tabs when smart is disabled. false may help debugging.
		gchar** arg_add_virtual = nullptr;  ///< load smartctl data from these files as virtual drives
		gchar** arg_add_device = nullptr;  ///< add these device files manually
		double arg_gdk_scale = std::numeric_limits<double>::quiet_NaN();  ///< The value of GDK_SCALE environment variable
		double arg_gdk_dpi_scale = std::numeric_limits<double>::quiet_NaN();  ///< The value of GDK_DPI_SCALE environment variable
	};



	/// Parse command-line arguments (fills \c args)
	inline bool parse_cmdline_args(CmdArgs& args, int& argc, char**& argv)
	{
		static const std::vector<GOptionEntry> arg_entries = {
			{ "no-locale", 'l', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_locale),
					N_("Don't use system locale"), nullptr },
			{ "version", 'V', 0, G_OPTION_ARG_NONE, &(args.arg_version),
					N_("Display version information"), nullptr },
			{ "no-scan", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_scan),
					N_("Don't scan devices on startup"), nullptr },
			{ "no-hide-tabs", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_hide_tabs),
					N_("Don't hide non-identity tabs when SMART is disabled. Useful for debugging."), nullptr },
			{ "add-virtual", '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &(args.arg_add_virtual),
					N_("Load smartctl data from file, creating a virtual drive. You can specify this option multiple times."), nullptr },
			{ "add-device", '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &(args.arg_add_device),
					N_("Add this device to device list. The format of the device is \"<device>::<type>::<extra_args>\", where type and extra_args are optional."
					" This option is useful with --no-scan to list certain drives only. You can specify this option multiple times."
					" Example: --add-device /dev/sda --add-device /dev/twa0::3ware,2 --add-device '/dev/sdb::::-T permissive'"), nullptr },
#ifndef _WIN32
			// X11-specific
			{ "gdk-scale", 'l', 0, G_OPTION_ARG_DOUBLE, &(args.arg_gdk_scale),
					N_("The value of GDK_SCALE environment variable (useful when executing with pkexec)"), nullptr },
			{ "gdk-dpi-scale", 'l', 0, G_OPTION_ARG_DOUBLE, &(args.arg_gdk_dpi_scale),
					N_("The value of GDK_DPI_SCALE environment variable (useful when executing with pkexec)"), nullptr },
#endif
			{ nullptr, '\0', 0, G_OPTION_ARG_NONE, nullptr, nullptr, nullptr }
		};

		GError* error = nullptr;
		GOptionContext* context = g_option_context_new("- A GTK+ GUI for smartmontools");

		// our options
		g_option_context_add_main_entries(context, arg_entries.data(), nullptr);

		// gtk options
		g_option_context_add_group(context, gtk_get_option_group(FALSE));

		// libdebug options; this will also automatically apply them
		g_option_context_add_group(context, debug_get_option_group());

		// The command-line parser stops at the first unknown option. Since this
		// is kind of inconsistent, we abort altogether.
		bool parsed = static_cast<bool>(g_option_context_parse(context, &argc, &argv, &error));

		if (error) {
			std::string error_text = "\n" + Glib::ustring::compose(_("Error parsing command-line options: %1"), (error->message ? error->message : "invalid error"));
			error_text += "\n\n";
			g_error_free(error);

			gchar* help_text = g_option_context_get_help(context, TRUE, nullptr);
			if (help_text) {
				error_text += help_text;
				g_free(help_text);
			}

			std::fprintf(stderr, "%s", error_text.c_str());
		}
		g_option_context_free(context);

		return parsed;
	}



	/// Print application version information
	inline void app_print_version_info()
	{
		std::string versiontext = "\n" + Glib::ustring::compose(_("GSmartControl version %1"), PACKAGE_VERSION) + "\n";

		std::string warningtext = std::string("\n") + _("Warning: GSmartControl comes with ABSOLUTELY NO WARRANTY.\n"
				"See LICENSE.txt file for details.") + "\n\n";
		/// %1 is years, %2 is email address
		warningtext += Glib::ustring::compose(_("Copyright (C) %1 Alexander Shaduri %2"), "2008 - 2021",
				"<ashaduri@gmail.com>") + "\n\n";

		std::fprintf(stdout, "%s%s", versiontext.c_str(), warningtext.c_str());
	}

}




bool app_init_and_loop(int& argc, char**& argv)
{
#ifdef _WIN32
	// Disable client-side decorations (enable native windows decorations) under Windows.
	hz::env_set_value("GTK_CSD", "0");
#endif

	// Set up gettext. This has to be before gtk is initialized.
	bindtextdomain(PACKAGE_NAME, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
	textdomain(PACKAGE_NAME);

	// Glib needs the C locale set to system locale for command line args.
	// We will reset it later if needed.
	hz::locale_c_set("");  // set the current locale to system locale

	// Parse command line args.
	// Due to gtk_get_option_group()/g_option_context_parse() calls, this
	// will also initialize GTK and set the C locale to system locale (as well
	// as do some locale-specific gdk initialization).
	CmdArgs args;
	if (! parse_cmdline_args(args, argc, argv)) {
		return true;
	}

	// If locale setting is explicitly disabled, revert to the original classic C locale.
	// Note that changing GTK locale after it's inited isn't really supported by GTK,
	// but we have no other choice - glib needs system locale when parsing the
	// arguments, and gtk is inited while the parsing is performed.
	if (args.arg_locale == FALSE) {
		hz::locale_c_set("C");
	} else {
		// change the C++ locale to match the C one.
		hz::locale_cpp_set("");  // this may fail on some systems
	}


	if (args.arg_version == TRUE) {
		// show version information and exit
		app_print_version_info();
		return true;
	}


	// register libdebug domains
	debug_register_domain("gtk");
	debug_register_domain("app");
	debug_register_domain("hz");
	debug_register_domain("rconfig");


	// Add special debug channel to collect all libdebug output into a buffer.
	debug_add_channel("all", debug_level::get_all_flags(), get_debug_buf_channel());



	std::vector<std::string> load_virtuals;
	if (args.arg_add_virtual) {
		const gchar* entry = nullptr;
		while ( (entry = *(args.arg_add_virtual)++) != nullptr ) {
			load_virtuals.emplace_back(entry);
		}
	}
	std::string load_virtuals_str = hz::string_join(load_virtuals, ", ");  // for display purposes only

	std::vector<std::string> load_devices;
	if (args.arg_add_device) {
		const gchar* entry = nullptr;
		while ( (entry = *(args.arg_add_device)++) != nullptr ) {
			load_devices.emplace_back(entry);
		}
	}
	std::string load_devices_str = hz::string_join(load_devices, "; ");  // for display purposes only


	// it's here because earlier there are no domains
	debug_out_dump("app", "Application options:\n"
		<< "\tlocale: " << args.arg_locale << "\n"
		<< "\tversion: " << args.arg_version << "\n"
		<< "\thide_tabs: " << args.arg_hide_tabs << "\n"
		<< "\tscan: " << args.arg_scan << "\n"
		<< "\targ_add_virtual: " << (load_virtuals_str.empty() ? "[empty]" : load_virtuals_str) << "\n"
		<< "\targ_add_device: " << (load_devices_str.empty() ? "[empty]" : load_devices_str) << "\n"
		<< "\targ_gdk_scale: " << args.arg_gdk_scale << "\n"
		<< "\targ_gdk_dpi_scale: " << args.arg_gdk_dpi_scale << "\n");

	debug_out_dump("app", "LibDebug options:\n" << debug_get_cmd_args_dump());

#ifndef _WIN32
	if (!std::isnan(args.arg_gdk_scale)) {
		hz::env_set_value("GDK_SCALE", hz::number_to_string_nolocale(args.arg_gdk_scale));
	}
	if (!std::isnan(args.arg_gdk_dpi_scale)) {
		hz::env_set_value("GDK_DPI_SCALE", hz::number_to_string_nolocale(args.arg_gdk_dpi_scale));
	}
#endif


	// Load config files
	app_init_config();


	// Redirect all GTK+/Glib and related messages to libdebug.
	// Do this before GTK+ init, to capture its possible warnings as well.
	const std::vector<const char*> gtkdomains = {
			// no atk or cairo, they don't log. libgnomevfs may be loaded by gtk file chooser.
			"GLib", "GModule", "GLib-GObject", "GLib-GRegex", "GLib-GIO", "GThread",
			"Pango", "Gtk", "Gdk", "GdkPixbuf", "libgnomevfs",
			"glibmm", "giomm", "atkmm", "pangomm", "gdkmm", "gtkmm"
	};

	for (const auto* domain : gtkdomains) {
		g_log_set_handler(domain, GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
				| G_LOG_FLAG_RECURSION), glib_message_handler, nullptr);
	}


	// Save the locale
	std::locale final_loc_cpp = hz::locale_cpp_get<std::locale>();

	// Initialize GTK+ (it's already initialized by command-line parser,
	// so this doesn't do much).
	// Newer gtkmm will try to set the C++ locale here.
	// Note: passing false (as use_locale) as the third parameter here
	// will generate a gtk_disable_setlocale() warning (due to gtk being
	// already initialized), so manually save / restore the C++ locale
	// (C locale won't be touched).
	// Nothing is affected in gtkmm itself by C++ locale, so it's ok to do it.
	Gtk::Main m(argc, argv);

	// Restore the locale
	hz::locale_cpp_set(final_loc_cpp);


	debug_out_info("app", "Current C locale: " << hz::locale_c_get() << "\n");
	debug_out_info("app", "Current C++ locale: " << hz::locale_cpp_get<std::string>() << "\n");

	debug_out_info("app", "Current working directory: " << Glib::get_current_dir() << "\n");


#ifdef _WIN32
	// Now that all program-specific locale setup has been performed,
	// make sure the future locale changes affect only current thread.
	// Not available in mingw, so disable for now.
// 	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif


	// This shows up in About dialog gtk.
	Glib::set_application_name(_("GSmartControl"));

	// Check whether we're running from build directory, or it's a packaged program.
	auto application_dir = hz::fs_get_application_dir();
	debug_out_info("app", "Application directory: " << application_dir << "\n");

	bool is_from_source = !application_dir.empty() && hz::fs::exists((application_dir / "src"));  // this covers standard cmake builds, but not VS.

	// Add data file search paths
	if (is_from_source) {
		hz::data_file_add_search_directory("icons", hz::fs::u8path(PACKAGE_TOP_SOURCE_DIR) / "data");
		hz::data_file_add_search_directory("ui", hz::fs::u8path(PACKAGE_TOP_SOURCE_DIR) / "src/ui");
		hz::data_file_add_search_directory("doc", hz::fs::u8path(PACKAGE_TOP_SOURCE_DIR) / "doc");
	} else {
	#ifdef _WIN32
		hz::data_file_add_search_directory("icons", application_dir);
		hz::data_file_add_search_directory("ui", application_dir / "ui");
		hz::data_file_add_search_directory("doc", application_dir / "doc");
	#else
		hz::data_file_add_search_directory("icons", hz::fs::u8path(PACKAGE_PKGDATA_DIR) / "icons");  // /usr/share/program_name/icons
		hz::data_file_add_search_directory("ui", hz::fs::u8path(PACKAGE_PKGDATA_DIR) / "ui");  // /usr/share/program_name/ui
		hz::data_file_add_search_directory("doc", hz::fs::u8path(PACKAGE_DOC_DIR));  // /usr/share/doc/[packages/]gsmartcontrol
	#endif
	}


#ifdef _WIN32
	// Windows "Classic" theme is broken under GTK+3's "win32" theme.
	// Make sure we fall back to Adwaita (which works, but looks non-native)
	// for platforms which support "Classic" theme - Windows Server and Windows Vista / 7.
	// Windows 8 / 10 don't support "Classic" so native look is preferred.
	{
		Glib::RefPtr<Gtk::Settings> gtk_settings = Gtk::Settings::get_default();
		if (gtk_settings) {
			Glib::ustring theme_name = gtk_settings->property_gtk_theme_name().get_value();
			debug_out_dump("app", "Current GTK theme: " << theme_name << "\n");
			if (IsWindowsServer() || !IsWindows8OrGreater()) {
				if (theme_name == "win32") {
					debug_out_dump("app", "Windows with Classic theme support detected, switching to Adwaita theme.\n");
					gtk_settings->property_gtk_theme_name().set_value("Adwaita");
				}
			}
		}
	}
#endif

	// Set default icon for all windows.
	// Win32 version has its icon compiled-in, so no need to set it there.
#ifndef _WIN32
	{
		// we load it via icontheme to provide multi-size version.

		// application-installed, /usr/share/icons/<theme_name>/apps/<size>
		if (Gtk::IconTheme::get_default()->has_icon("gsmartcontrol")) {
			Gtk::Window::set_default_icon_name("gsmartcontrol");

		// try the gnome icon, it's higher quality / resolution
		} else if (Gtk::IconTheme::get_default()->has_icon("gnome-dev-harddisk")) {
			Gtk::Window::set_default_icon_name("gnome-dev-harddisk");

		// gtk built-in, always available
		} else {
			Gtk::Window::set_default_icon_name("gtk-harddisk");
		}
	}
#endif


	// Export some command line arguments to rconfig

	// obey the command line option for no-scan on startup
	get_startup_settings().no_scan = !bool(args.arg_scan);

	// load virtual drives on startup if specified.
	get_startup_settings().load_virtuals = load_virtuals;

	// add devices to the list on startup if specified.
	get_startup_settings().add_devices = load_devices;

	// hide tabs if SMART is disabled
	get_startup_settings().hide_tabs_on_smart_disabled = bool(args.arg_hide_tabs);


	// Create executor log window, but don't show it.
	// It will track all command executor outputs.
	// The window is destroyed by the instance manager.
	GscExecutorLogWindow::create();


	// Open the main window.
	// The window is destroyed by the instance manager.
	{
		auto main_window = GscMainWindow::create();
		if (!main_window) {
			debug_out_fatal("app", "Cannot create the main window. Exiting.\n");
			return false;  // cannot create main window
		}

		// first-boot message
		// app_show_first_boot_message(win);

		// The Main Loop
		debug_out_info("app", "Entering main loop.\n");
		Gtk::Main::run();
		debug_out_info("app", "Main loop exited.\n");
	}

	// Destroy all windows manually, to avoid surprises
	WindowInstanceManagerStorage::destroy_all_instances();

	// std::cerr << app_get_debug_buffer_str();  // this will output everything that went through libdebug.

	return true;
}




void app_quit()
{
	debug_out_info("app", "Saving config before exit...\n");

	// save the config
#if defined ENABLE_GLIB && ENABLE_GLIB
	rconfig::autosave_force_now();
#else
	rconfig::save_to_file(get_home_config_file());
#endif

	// exit the main loop
	debug_out_info("app", "Trying to exit the main loop...\n");

	Gtk::Main::quit();

	// don't destroy main window here - we may be in one of its callbacks
}








/// @}
