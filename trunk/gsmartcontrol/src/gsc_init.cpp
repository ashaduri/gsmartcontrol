/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <string>
// #include <locale.h>  // _configthreadlocale (win32)
#include <stdexcept>  // std::runtime_error
#include <cstdio>  // std::printf
#include <vector>
#include <sstream>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtk/gtk.h>  // gtk_window_set_default_icon_name, gtk_icon_theme_*
#include <glib.h>  // g_, G*
#include <glibmm/refptr.h>
#include <glibmm/miscutils.h>  // set_application_name
#include <glibmm/thread.h>  // thread_init
// #include <gtkmm.h>
// #include <iostream>

#include "hz/hz_config.h"  // ENABLE_GLIB, VERSION, DEBUG_BUILD

#include "libdebug/libdebug.h"  // include full libdebug here (to add domains, etc...)
#include "rconfig/rconfig.h"  // include full rconfig here
#include "hz/data_file.h"  // data_file_add_search_directory
#include "hz/fs_tools.h"  // get_user_config_dir()
#include "hz/fs_path.h"  // FsPath
#include "hz/locale_tools.h"  // locale_c*
#include "hz/string_algo.h"  // string_join()
#include "hz/win32_tools.h"  // win32_get_registry_value_string()

#include "gsc_main_window.h"
#include "gsc_executor_log_window.h"
#include "gsc_settings.h"
#include "gsc_init.h"



namespace {

	static std::string s_home_config_file;


	static debug_channel_base_ptr s_debug_buf_channel;
	static std::ostringstream s_debug_buf_channel_stream;


	// This function is not thread-safe. The channel must be locked properly.
	inline debug_channel_base_ptr app_get_debug_buf_channel()
	{
		if (!s_debug_buf_channel)
			return (s_debug_buf_channel = new DebugChannelOStream(s_debug_buf_channel_stream));
		return s_debug_buf_channel;
	}

}


// This should be called from one thread only.
std::string app_get_debug_buffer_str()
{
	debug_channel_base_ptr channel = app_get_debug_buf_channel();
	DebugChannelOStream::intrusive_ptr_scoped_lock_type locker(channel->get_ref_mutex());  // lock
	// now the channel is locked, so we have a proper access to the stream.
	return s_debug_buf_channel_stream.str();
}






inline bool app_init_config()
{
	// $XDG_CONFIG_DIRS defaults to /etc/xdg, which is really silly.
	// Actually, the whole specification is silly imho, since instead of
	// removing just one directory/file I have to remove 3 now (and tell
	// the users to do the same thing). And the *_DIRS stuff completely
	// breaks parallel installations of the same program. Implementing
	// only $XDG_CONFIG_HOME is harmless enough, so we do it.

	s_home_config_file = hz::get_user_config_dir() + hz::DIR_SEPARATOR_S
			+ "gsmartcontrol" + hz::DIR_SEPARATOR_S + "gsmartcontrol.conf";

	std::string global_config_file;
	std::string old_local_config;  // pre-XDG and pre-CSIDL_APPDATA file for migration

#ifdef _WIN32
	global_config_file = "gsmartcontrol.conf";  // CWD, installation dir by default.

	std::string old_config_dir;
	hz::win32_get_registry_value_string(HKEY_CURRENT_USER,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Personal", old_config_dir);

	old_local_config = old_config_dir + hz::DIR_SEPARATOR_S + "gsmartcontrol.conf";

#else
	global_config_file = std::string(PACKAGE_SYSCONF_DIR)
			+ hz::DIR_SEPARATOR_S + "gsmartcontrol.conf";

	old_local_config = hz::get_home_dir() + hz::DIR_SEPARATOR_S + ".gsmartcontrolrc";
#endif

	debug_out_dump("app", DBG_FUNC_MSG << "Global config file: \"" << global_config_file << "\"\n");
	debug_out_dump("app", DBG_FUNC_MSG << "Local config file: \"" << s_home_config_file << "\"\n");
	debug_out_dump("app", DBG_FUNC_MSG << "Old local config file: \"" << old_local_config << "\"\n");

	hz::FsPath gp(global_config_file);  // Default system-wide settings. This file is empty by default.
	hz::FsPath hp(s_home_config_file);  // Per-user settings.
	hz::FsPath op(old_local_config);  // Old user settings, should be migrated.

	if (gp.exists() && gp.is_readable()) {  // load global first
		rconfig::load_from_file(gp.str());
	}

	if (hp.exists() && hp.is_readable()) {  // load local
		rconfig::load_from_file(hp.str());

	} else {
		// create the parent directories of the config file
		hz::FsPath config_loc(hp.get_dirname());

		if (!config_loc.exists()) {
			config_loc.make_dir(0700, true);  // with parents.
		}

		if (op.exists() && op.is_readable()) {  // load the old config if the new one doesn't exist.
			debug_print_info("app", "Old configuration file found at \"%s\", migrating to \"%s\".\n", op.c_str(), hp.c_str());
			rconfig::load_from_file(op.str());
			// force saving of the config to the new location (so that it's preserved in case of crash).
			if (rconfig::save_to_file(hp.str())) {
				// remove the old config
				op.remove();
			}
		}
	}


	rconfig::dump_tree();

	init_default_settings();  // initialize /default

#if defined ENABLE_GLIB && ENABLE_GLIB
	rconfig::autosave_set_config_file(s_home_config_file);
	uint32_t autosave_timeout = rconfig::get_data<uint32_t>("system/config_autosave_timeout");
	if (autosave_timeout)
		rconfig::autosave_start(autosave_timeout);
#endif

	return true;
}



inline void app_show_first_boot_message(Gtk::Window* parent)
{
	bool first_boot = false;
	rconfig::get_data("system/first_boot", first_boot);

	if (first_boot) {
// 		Glib::ustring msg = "First boot";
// 		Gtk::MessageDialog(*parent, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true).run();
	}

// 	rconfig::set_data("system/first_boot", false);  // don't show it again
}




// glib message -> libdebug message convertor
extern "C" {
	static void glib_message_handler(const gchar* log_domain, GLogLevelFlags log_level,
			const gchar* message, gpointer user_data)
	{
		// log_domain is already printed as part of message.
		debug_print_error("gtk", "%s\n", message);
	}
}


// Note: Use GLib types here.
struct CmdArgs {
	CmdArgs() :
		// defaults
		arg_locale(TRUE),
		arg_version(FALSE),
		arg_scan(TRUE),
		arg_hide_tabs(TRUE),
		arg_add_virtual(NULL),
		arg_add_device(NULL)
	{ }

	gboolean arg_locale;  // if false, disable using system locale
	gboolean arg_version;  // if true, show version and exit
	gboolean arg_scan;  // if false, don't scan the system for drives on startup
	gboolean arg_hide_tabs;  // if true, hide additional info tabs when smart is disabled. false may help debugging.
	gchar** arg_add_virtual;  // load smartctl data from these files as virtual drives
	gchar** arg_add_device;  // add these device files manually
};



inline bool parse_cmdline_args(CmdArgs& args, int& argc, char**& argv)
{
	static const GOptionEntry arg_entries[] =
	{
		{ "no-locale", 'l', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_locale),
				"Don't use system locale", NULL },
		{ "version", 'V', 0, G_OPTION_ARG_NONE, &(args.arg_version),
				"Display version information", NULL },
		{ "no-scan", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_scan),
				"Don't scan devices on startup", NULL },
		{ "no-hide-tabs", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_hide_tabs),
				"Don't hide non-identity tabs when SMART is disabled. Useful for debugging.", NULL },
		{ "add-virtual", '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &(args.arg_add_virtual),
				"Load smartctl data from file, creating a virtual drive. You can specify this option multiple times.", NULL },
		{ "add-device", '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &(args.arg_add_device),
				"Add this device to device list. The format of the device is \"<device>::<type>::<extra_args>\", where type and extra_args are optional."
				" This option is useful with --no-scan to list certain drives only. You can specify this option multiple times."
				" Example: --add-device /dev/sda --add-device /dev/twa0::3ware,2 --add-device '/dev/sdb::::-T permissive'", NULL },
		{ NULL }
	};

	GError* error = 0;
	GOptionContext* context = g_option_context_new("- A GTK+ GUI for smartmontools");

	// our options
	g_option_context_add_main_entries(context, arg_entries, NULL);

	// gtk options
	g_option_context_add_group(context, gtk_get_option_group(false));

	// libdebug options; this will also automatically apply them
	g_option_context_add_group(context, debug_get_option_group());

	// The command-line parser stops at the first unknown option. Since this
	// is kind of inconsistent, we abort altogether.
	bool parsed = g_option_context_parse(context, &argc, &argv, &error);

	if (error) {
		std::string error_text = "\n" + std::string("Error parsing command-line options: ");
		error_text += (error->message ? error->message : "invalid error");
		error_text += "\n\n";
		g_error_free(error);

#if (GLIB_CHECK_VERSION(2,14,0))
		gchar* help_text = g_option_context_get_help(context, true, NULL);
		if (help_text) {
			error_text += help_text;
			g_free(help_text);
		}
#else
		error_text += "Exiting.\n";
#endif

		std::fprintf(stderr, "%s", error_text.c_str());
	}
	g_option_context_free(context);

	return parsed;
}



inline void app_print_version_info()
{
	std::string versiontext = std::string("\nGSmartControl version ") + VERSION + "\n";

	std::string warningtext = std::string("\nWarning: GSmartControl");
	warningtext += " comes with ABSOLUTELY NO WARRANTY.\n";
	warningtext += "See LICENSE_gsmartcontrol.txt file for details.\n";
	warningtext += "\nCopyright (C) 2008 - 2012  Alexander Shaduri <ashaduri" "" "@" "" "" "gmail.com>\n\n";

	std::fprintf(stdout, "%s%s", versiontext.c_str(), warningtext.c_str());
}



bool app_init_and_loop(int& argc, char**& argv)
{
	// initialize GThread (for mutexes, etc... to work). Must be called before any other glib function.
// 	Glib::thread_init();


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
	if (!args.arg_locale) {
		hz::locale_c_set("C");
	} else {
		// change the C++ locale to match the C one.
		hz::locale_cpp_set("");  // this may fail on some systems
	}


	if (args.arg_version) {
		// show version information and exit
		app_print_version_info();
		return true;
	}


	// register libdebug domains
	debug_register_domain("gtk");
	debug_register_domain("app");
	debug_register_domain("hz");
	debug_register_domain("rmn");
	debug_register_domain("rconfig");


	// Add special debug channel to collect all libdebug output into a buffer.
	debug_add_channel("all", debug_level::all, app_get_debug_buf_channel());



	std::vector<std::string> load_virtuals;
	if (args.arg_add_virtual) {
		const gchar* entry = 0;
		while ( (entry = *(args.arg_add_virtual)++) != NULL ) {
			load_virtuals.push_back(entry);
		}
	}
	std::string load_virtuals_str = hz::string_join(load_virtuals, ", ");  // for display purposes only

	std::vector<std::string> load_devices;
	if (args.arg_add_device) {
		const gchar* entry = 0;
		while ( (entry = *(args.arg_add_device)++) != NULL ) {
			load_devices.push_back(entry);
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
		<< "\targ_add_device: " << (load_devices_str.empty() ? "[empty]" : load_devices_str) << "\n");

	debug_out_dump("app", "LibDebug options:\n" << debug_get_cmd_args_dump());


	// Load config files
	app_init_config();


	// Redirect all GTK+/Glib and related messages to libdebug.
	// Do this before GTK+ init, to capture its possible warnings as well.
	static const char* const gtkdomains[] = {
			// no atk or cairo, they don't log. libgnomevfs may be loaded by gtk file chooser.
			"GLib", "GModule", "GLib-GObject", "GLib-GRegex", "GLib-GIO", "GThread",
			"Pango", "Gtk", "Gdk", "GdkPixbuf", "libglade", "libgnomevfs",
			"glibmm", "giomm", "atkmm", "pangomm", "gdkmm", "gtkmm", "libglademm" };

	for (unsigned int i = 0; i < G_N_ELEMENTS(gtkdomains); ++i) {
		g_log_set_handler(gtkdomains[i], GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
				| G_LOG_FLAG_RECURSION), glib_message_handler, NULL);
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


#ifdef _WIN32
	// Now that all program-specific locale setup has been performed,
	// make sure the future locale changes affect only current thread.
	// Not available in mingw, so disable for now.
// 	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif


	// This shows up in About dialog gtk.
	Glib::set_application_name("GSmartControl");  // should be localized


	// Add data file search paths
#ifdef _WIN32
	// In windows the program is distributed with all the data files in the same directory.
	hz::data_file_add_search_directory(".");
#else
	#ifdef DEBUG_BUILD
		hz::data_file_add_search_directory(std::string(TOP_SRC_DIR) + "/src/res");  // application data resources
		hz::data_file_add_search_directory(std::string(TOP_SRC_DIR) + "/data");  // application data resources
	#else
		hz::data_file_add_search_directory(PACKAGE_PKGDATA_DIR);  // /usr/share/program_name
	#endif
#endif


	// set default icon for all windows.
	// set_default_icon_name is available since 2.12 in gtkmm, but since 2.6 in gtk.

#ifndef _WIN32  // win32 version has its icon compiled-in, so no need to set it there.
	{
		// we load it via icontheme to provide multi-size version.
		GtkIconTheme* default_icon_theme = gtk_icon_theme_get_default();

		// application-installed, /usr/share/icons/<theme_name>/apps/<size>
		if (gtk_icon_theme_has_icon(default_icon_theme, "gsmartcontrol")) {
			gtk_window_set_default_icon_name("gsmartcontrol");

		// try the gnome icon, it's higher quality / resolution
		} else if (gtk_icon_theme_has_icon(default_icon_theme, "gnome-dev-harddisk")) {
			gtk_window_set_default_icon_name("gnome-dev-harddisk");

		// gtk built-in, always available
		} else {
			gtk_window_set_default_icon_name("gtk-harddisk");
		}
	}
#endif


	// Export some command line arguments to rmn

	// obey the command line option for no-scan on startup
	rconfig::set_data("/runtime/gui/force_no_scan_on_startup", !bool(args.arg_scan));

	// load virtual drives on startup if specified.
	rconfig::set_data("/runtime/gui/add_virtuals_on_startup", load_virtuals);

	// add devices to the list on startup if specified.
	rconfig::set_data("/runtime/gui/add_devices_on_startup", load_devices);

	// hide tabs if SMART is disabled
	rconfig::set_data("/runtime/gui/hide_tabs_on_smart_disabled", bool(args.arg_hide_tabs));


	// Create executor log window, but don't show it.
	// It will track all command executor outputs.
	GscExecutorLogWindow::create();


	// Open the main window
	GscMainWindow* win = GscMainWindow::create();
	if (!win) {
		debug_out_fatal("app", "Cannot create the main window. Exiting.\n");
		return false;  // cannot create main window
	}


	// first-boot message
	app_show_first_boot_message(win);


	// The Main Loop (tm)
	debug_out_info("app", "Entering main loop.\n");
	m.run();
	debug_out_info("app", "Main loop exited.\n");


	// close the main window and delete its object
	GscMainWindow::destroy();

	GscExecutorLogWindow::destroy();


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
	rconfig::save_to_file(s_home_config_file);
#endif

	// exit the main loop
	debug_out_info("app", "Trying to exit the main loop...\n");

	Gtk::Main::quit();

	// don't destroy main window here - we may be in one of its callbacks
}







