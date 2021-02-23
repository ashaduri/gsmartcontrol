/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
License: Zlib
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#include "dflags.h"

#include <map>


namespace {


	/// Debug level names
	const std::map<debug_level::flag, const char*> s_level_names = {
		{debug_level::fatal, "fatal"},
		{debug_level::error, "error"},
		{debug_level::warn, "warn"},
		{debug_level::info, "info"},
		{debug_level::dump, "dump"}
	};


	/// Debug level color start sequences
	const std::map<debug_level::flag, const char*> s_level_colors = {
		{debug_level::fatal, "\033[1;4;31m"},  // red underlined
		{debug_level::error, "\033[1;4;31m"},  // red
		{debug_level::warn, "\033[1;35m"},  // magenta
		{debug_level::info, "\033[1;36m"},  // cyan
		{debug_level::dump, "\033[1;32m"}  // green
	};


}



namespace debug_level {

	const char* get_name(flag level)
	{
		return s_level_names.at(level);
	}


	const char* get_color_start(flag level)
	{
		return s_level_colors.at(level);
	}

	const char* get_color_stop([[maybe_unused]] flag level)
	{
		return "\033[0m";
	}

}







/// @}
