/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup rconfig
/// \weakgroup rconfig
/// @{

#ifndef RCONFIG_LOADSAVE_H
#define RCONFIG_LOADSAVE_H

#include <string>

#include "hz/debug.h"
#include "hz/fs.h"

#include "rconfig.h"



namespace rconfig {



/// Load the config branch from file.
inline bool load_from_file(const hz::fs::path& file)
{
	std::string json_str;
	auto ec = hz::fs_file_get_contents(file, json_str, 10*1024*1024);  // 10M
	if (ec) {
		debug_out_error("rconfig", "load_from_file(): Unable to read from file \""
				<< file << "\": " << ec.message() << std::endl);
		return false;
	}

	try {
		get_config_branch() = json::parse(json_str);
	}
	catch (json::parse_error& e) {
		debug_out_warn("rconfig", "Cannot load config file \""
				<< file << "\": " << e.what() << std::endl);
		return false;
	}
	return true;
}



/// Save the config branch to a file.
inline bool save_to_file(const hz::fs::path& file)
{
	const std::string json_str = get_config_branch().dump(4);

	auto ec = hz::fs_file_put_contents(file, json_str);
	if (ec) {
		debug_out_error("rconfig", DBG_FUNC_MSG
				<< "Unable to write to file \"" << file << "\": " << ec.message() << "." << std::endl);
		return false;
	}
	return true;
}





}  // ns



#endif

/// @}
