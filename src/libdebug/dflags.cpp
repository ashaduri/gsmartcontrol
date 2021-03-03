/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#include "dflags.h"

#include <map>


namespace debug_level {


/// Get debug level name
const char* get_name(flag level)
{
	static const std::map<debug_level::flag, const char*> level_names = {
		{debug_level::fatal, "fatal"},
		{debug_level::error, "error"},
		{debug_level::warn, "warn"},
		{debug_level::info, "info"},
		{debug_level::dump, "dump"}
	};
	return level_names.at(level);
}



/// Get debug level color start sequence
const char* get_color_start(flag level)
{
	static const std::map<debug_level::flag, const char*> level_colors = {
		{debug_level::fatal, "\033[1;4;31m"},  // red underlined
		{debug_level::error, "\033[1;4;31m"},  // red
		{debug_level::warn, "\033[1;35m"},  // magenta
		{debug_level::info, "\033[1;36m"},  // cyan
		{debug_level::dump, "\033[1;32m"}  // green
	};
	return level_colors.at(level);
}



/// Get debug level color stop sequence
const char* get_color_stop([[maybe_unused]] flag level)
{
	return "\033[0m";
}


}







/// @}
