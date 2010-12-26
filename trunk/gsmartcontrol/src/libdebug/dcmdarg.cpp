/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#include "hz/hz_config.h"  // ENABLE_GLIB, DEBUG_BUILD

#if defined ENABLE_GLIB && ENABLE_GLIB

#include <string>
#include <vector>
#include <map>
#include <glib.h>
#include <sstream>
#include <ios>  // std::boolalpha
#include <algorithm>  // std::find

#include "hz/string_algo.h"  // string_split()

#include "dcmdarg.h"
#include "dflags.h"
#include "dstate.h"




namespace debug_internal {


	// internal class. holds values of command line args.
	struct DebugCmdArgs {
		DebugCmdArgs() :
			// defaults
			verbose(FALSE),
			quiet(FALSE),
#ifdef DEBUG_BUILD
			verbosity_level(5),  // all
#else
			verbosity_level(3),  // warn, error, fatal
#endif
	// 		debug_levels(),
#ifndef _WIN32
			debug_colorize(TRUE)
#else
			debug_colorize(FALSE)  // disable colors on win32
#endif
		{ }

		// use glib types here
		gboolean verbose;
		gboolean quiet;
		gint verbosity_level;
		std::vector<std::string> debug_levels;  // comma-separated names to enable
		gboolean debug_colorize;

		debug_level::type levels_enabled;  // final vector - not actually an argument, but filled after the actual parsing.
	};



	static DebugCmdArgs s_debug_cmd_args;

	inline DebugCmdArgs* debug_get_args_holder()
	{
		return &s_debug_cmd_args;
	}


} // ns debug_internal




extern "C" {

	static gboolean debug_internal_parse_levels(const gchar* option_name,
			const gchar* value, gpointer data, GError** error);

	static gboolean debug_internal_post_parse_func(GOptionContext* context,
			GOptionGroup *group, gpointer data, GError** error);

}



// GLib callback
static gboolean debug_internal_parse_levels(const gchar* option_name,
		const gchar* value, gpointer data, GError** error)
{
	if (!value)
		return false;
	debug_internal::DebugCmdArgs* args = static_cast<debug_internal::DebugCmdArgs*>(data);
	std::string levels = value;
	hz::string_split(levels, ',', args->debug_levels, true);
	// will filter out invalid ones later
	return true;
}



// GLib callback
static gboolean debug_internal_post_parse_func(GOptionContext* context,
		GOptionGroup *group, gpointer data, GError** error)
{
	debug_internal::DebugCmdArgs* args = static_cast<debug_internal::DebugCmdArgs*>(data);

	if (!args->debug_levels.empty()) {  // no string levels on command line given
		args->levels_enabled = debug_level::none;  // reset
		args->levels_enabled |= (std::find(args->debug_levels.begin(), args->debug_levels.end(),
				"dump") != args->debug_levels.end() ? debug_level::dump : debug_level::none);
		args->levels_enabled |= (std::find(args->debug_levels.begin(), args->debug_levels.end(),
				"info") != args->debug_levels.end() ? debug_level::info : debug_level::none);
		args->levels_enabled |= (std::find(args->debug_levels.begin(), args->debug_levels.end(),
				"warn") != args->debug_levels.end() ? debug_level::warn : debug_level::none);
		args->levels_enabled |= (std::find(args->debug_levels.begin(), args->debug_levels.end(),
				"error") != args->debug_levels.end() ? debug_level::error : debug_level::none);
		args->levels_enabled |= (std::find(args->debug_levels.begin(), args->debug_levels.end(),
				"fatal") != args->debug_levels.end() ? debug_level::fatal : debug_level::none);

	} else if (args->quiet) {
		args->levels_enabled = debug_level::none;

	} else if (args->verbose) {
		args->levels_enabled = debug_level::all;

	} else {
		args->levels_enabled = debug_level::none;

		if (args->verbosity_level > 0) args->levels_enabled |= debug_level::fatal;
		if (args->verbosity_level > 1) args->levels_enabled |= debug_level::error;
		if (args->verbosity_level > 2) args->levels_enabled |= debug_level::warn;
		if (args->verbosity_level > 3) args->levels_enabled |= debug_level::info;
		if (args->verbosity_level > 4) args->levels_enabled |= debug_level::dump;
	}

	bool color_enabled = args->debug_colorize;


	unsigned long levels_enabled_ulong = args->levels_enabled.to_ulong();
	debug_internal::DebugState::domain_map_t& dm = debug_internal::get_debug_state().get_domain_map();

	for (debug_internal::DebugState::domain_map_t::iterator iter = dm.begin(); iter != dm.end(); ++iter) {
		for (debug_internal::DebugState::level_map_t::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2) {

			iter2->second->set_enabled(levels_enabled_ulong & iter2->first);

			debug_format::type format = iter2->second->get_format();
			if (color_enabled) {
				format |= debug_format::color;
			} else {
				format &= ~(unsigned long)debug_format::color;
			}
			iter2->second->set_format(format);

		}
	}

	return true;
}





std::string debug_get_cmd_args_dump()
{
	debug_internal::DebugCmdArgs* args = debug_internal::debug_get_args_holder();
	std::ostringstream ss;

// 	ss << "\tverbose: " << std::boolalpha << static_cast<bool>(args->verbose) << "\n";
// 	ss << "\tquiet: " << std::boolalpha << static_cast<bool>(args->quiet) << "\n";
// 	ss << "\tverbosity_level: " << args->verbosity_level << "\n";
// 	ss << "\tdebug_levels: " << args->debug_levels << "\n";
	ss << "\tlevels_enabled: " << args->levels_enabled << "\n";
	ss << "\tdebug_colorize: " << std::boolalpha << static_cast<bool>(args->debug_colorize) << "\n";

	return ss.str();
}




// If adding the returned value to context (which you must do),
// no need to free the result.
GOptionGroup* debug_get_option_group()
{
	debug_internal::DebugCmdArgs* args = debug_internal::debug_get_args_holder();

	GOptionGroup* group = g_option_group_new("debug",
			"Libdebug Logging Options", "Show libdebug options",
			args, NULL);

	static const GOptionEntry entries[] =
	{
		{ "verbose", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
				&(args->verbose), "Enable verbose logging; same as --verbosity-level 5", NULL },
		{ "quiet", 'q', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
				&(args->quiet), "Disable logging; same as --verbosity-level 0", NULL },
		{ "verbosity-level", 'b', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT,
				&(args->verbosity_level), "Set verbosity level [0-5]", NULL },
		{ "debug-levels", '\0', 0, G_OPTION_ARG_CALLBACK,
				(gpointer)(&debug_internal_parse_levels),  // reinterpret_cast<> doesn't work here
				"Enable only these logging levels; the argument is a comma-separated list of (dump, info, warn, error, fatal)", NULL },
		{ "debug-colorize", '\0', 0, G_OPTION_ARG_NONE,
				&(args->debug_colorize), "Enable colored output", NULL },
		{ "debug-no-colorize", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,
				&(args->debug_colorize), "Disable colored output", NULL },
		{ NULL }
	};

	g_option_group_add_entries(group, entries);

	g_option_group_set_parse_hooks(group, NULL, &debug_internal_post_parse_func);

	return group;
}





#endif  // glib



