/**************************************************************************
 Copyright:
      (C) 2004 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_DATA_FILE_H
#define HZ_DATA_FILE_H

#include <string>
#include <vector>

#include "fs_ns.h"
#include "debug.h"



namespace hz {


/// Static variable holder
struct DataFileStaticHolder {
	static inline std::vector<std::string> search_directories;  ///< Search directories for data files
};



//------------------------------------------------------------------------


/// Add a directory to a search path
inline void data_file_add_search_directory(const std::string& path)
{
	if (!path.empty())
		DataFileStaticHolder::search_directories.push_back(path);
}



/// Get currently registered search directories (a copy is returned)
inline std::vector<std::string> data_file_get_search_directories()
{
	return DataFileStaticHolder::search_directories;
}



/// Set a directory list for a search path
inline void data_file_set_search_directories(const std::vector<std::string>& dirs)
{
	DataFileStaticHolder::search_directories = dirs;
}



/// Find a data file (using a file name) in a search directory list.
/// \return A full path to filename, or empty string on "not found".
inline fs::path data_file_find(const std::string& filename, bool allow_to_be_directory = false)
{
	if (filename.empty())
		return fs::path();

	auto file_path = fs::u8path(filename);
	std::error_code ec;

	if (file_path.is_absolute()) {  // shouldn't happen, but still...
		if (!fs::exists(file_path, ec)) {
			debug_print_error("app", "%s: Data file \"%s\" not found: %s\n",
					DBG_FUNC, file_path.u8string().c_str(), ec.message().c_str());
			return fs::path();

		} else if (!allow_to_be_directory && fs::is_directory(file_path, ec)) {
			debug_print_error("app", "%s: Data file \"%s\" is a directory (which is not allowed).\n",
					DBG_FUNC, file_path.u8string().c_str());
			return fs::path();
		}
		return file_path;  // return as is.
	}

	for (const auto& dir : DataFileStaticHolder::search_directories) {
		auto dir_path = fs::u8path(dir) / file_path;

		// debug_print_dump("app", "%s: Searching for data \"%s\" at \"%s\".\n", DBG_FUNC, fp.c_str(), dirpath.c_str());
		if (fs::exists(dir_path, ec)) {
			if (!allow_to_be_directory && fs::is_directory(dir_path, ec)) {
				debug_print_error("app", "%s: Data file \"%s\" file found at \"%s\", but it is a directory.\n",
						DBG_FUNC, file_path.u8string().c_str(), dir.c_str());
				return fs::path();
			}
			debug_print_info("app", "%s: Data file \"%s\" found at \"%s\".\n", DBG_FUNC, file_path.u8string().c_str(), dir.c_str());
			return dir_path;
		}
	}

	debug_print_error("app", "%s: Data file \"%s\" not found.\n", DBG_FUNC, file_path.u8string().c_str());
	return fs::path();
}





} // ns



#endif

/// @}
