/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
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

	std::string json;
	if (!f.get_contents(json)) {  // this warns
		debug_print_error("rconfig", "load_from_file(): Unable to read from file \"%s\".\n", file.c_str());
		return false;
	}

	std::string error = picojson::parse(get_config_branch(), json);
	if (!error.empty()) {
		debug_out_warn("rconfig", "Cannot load config file \"" << file << "\": " + error + "\n");
		return false;
	}
	return true;
}



/// Save the "/config" branch to a file.
inline bool save_to_file(const std::string& file)
{
	std::string json = get_config_branch().serialize();

	hz::File f(file);
	if (!f.put_contents(json)) {
		debug_print_error("rconfig", "save_to_file(): Unable to write to file \"%s\".\n", file.c_str());
		return false;
	}

	return true;
}





}  // ns



#endif

/// @}
