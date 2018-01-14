/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup rconfig
/// \weakgroup rconfig
/// @{

#ifndef RCONFIG_LOADSAVE_H
#define RCONFIG_LOADSAVE_H

#include <string>

#include "config.h"
#include "hz/debug.h"
#include "hz/fs_file.h"



namespace rconfig {



/// Load the config branch from file.
inline bool load_from_file(const std::string& file)
{
	// Don't use std::fstream, it's not safe with localized filenames on win32.
	hz::File f(file);

	std::string json_str;
	if (!f.get_contents(json_str)) {  // this warns
		debug_print_error("rconfig", "load_from_file(): Unable to read from file \"%s\".\n", file.c_str());
		return false;
	}

	try {
		get_config_branch() = json::parse(json_str);
	}
	catch (json::parse_error& e) {
		debug_out_warn("rconfig", "Cannot load config file \"" << file << "\": " << e.what() << "\n");
		return false;
	}
	return true;
}



/// Save the "/config" branch to a file.
inline bool save_to_file(const std::string& file)
{
	std::string json_str = get_config_branch().dump(4);

	hz::File f(file);
	if (!f.put_contents(json_str)) {
		debug_print_error("rconfig", "save_to_file(): Unable to write to file \"%s\".\n", file.c_str());
		return false;
	}

	return true;
}





}  // ns



#endif

/// @}
