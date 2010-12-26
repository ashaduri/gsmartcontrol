/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#include "dflags.h"

#include <map>


struct DebugFlags {

	DebugFlags()
	{
		level_names[debug_level::fatal] = "fatal";
		level_names[debug_level::error] = "error";
		level_names[debug_level::warn] = "warn";
		level_names[debug_level::info] = "info";
		level_names[debug_level::dump] = "dump";

		level_colors[debug_level::fatal] = "\033[1;4;31m";  // red underlined
		level_colors[debug_level::error] = "\033[1;31m";  // red
		level_colors[debug_level::warn] = "\033[1;35m";  // magenta
		level_colors[debug_level::info] = "\033[1;36m";  // cyan
		level_colors[debug_level::dump] = "\033[1;32m";  // green
	}

	std::map<debug_level::flag, const char*> level_names;
	std::map<debug_level::flag, const char*> level_colors;
};




static DebugFlags s_debug_flags;



namespace debug_level {

	const char* get_name(flag level)
	{
		return s_debug_flags.level_names[level];
	}


	const char* get_color_start(flag level)
	{
		return s_debug_flags.level_colors[level];
	}

	const char* get_color_stop(flag level)
	{
		return "\033[0m";
	}

}






