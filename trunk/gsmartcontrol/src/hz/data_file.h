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

#include "fs_path.h"
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
inline std::string data_file_find(const std::string& filename, bool allow_to_be_directory = false)
{
	if (filename.empty())
		return std::string();

	hz::FsPath fp(filename);
	// convert to native if it contains separators. this allows us
	// to use subdirectories in filenames while always using slashes.
	fp.to_native();

	if (fp.is_absolute()) {  // shouldn't happen, but still...
		if (!fp.exists()) {
			debug_print_error("app", "%s: Data file \"%s\" not found: File doesn't exist.\n", DBG_FUNC, fp.c_str());
			return std::string();

		} else if (!allow_to_be_directory && fp.is_dir()) {
			debug_print_error("app", "%s: Data file \"%s\" is a directory (which is not allowed).\n", DBG_FUNC, fp.c_str());
			return std::string();
		}

		return fp.str();  // return as is.
	}

	for (const auto& dirpath : DataFileStaticHolder::search_directories) {
		hz::FsPath dp(dirpath + hz::DIR_SEPARATOR_S + fp.str());

		// debug_print_dump("app", "%s: Searching for data \"%s\" at \"%s\".\n", DBG_FUNC, fp.c_str(), dirpath.c_str());

		if (dp.exists()) {
			if (!allow_to_be_directory && dp.is_dir()) {
				debug_print_error("app", "%s: Data file \"%s\" file found at \"%s\", but it is a directory.\n",
						DBG_FUNC, fp.c_str(), dirpath.c_str());
				return std::string();
			}

			debug_print_info("app", "%s: Data file \"%s\" found at \"%s\".\n", DBG_FUNC, fp.c_str(), dirpath.c_str());
			return dp.str();
		}
	}

	debug_print_error("app", "%s: Data file \"%s\" not found.\n", DBG_FUNC, fp.c_str());
	return std::string();
}





} // ns



#endif

/// @}
