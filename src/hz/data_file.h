/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_DATA_FILE_H
#define HZ_DATA_FILE_H

#include <string>
#include <vector>
#include <unordered_map>

#include "fs.h"
#include "debug.h"



namespace hz {


/// Static variable holder
struct DataFileStaticHolder {
	///< Search directories for data files
	static inline std::unordered_map<std::string, std::vector<fs::path>> search_directories;
};



//------------------------------------------------------------------------


/// Add a directory to a search path
inline void data_file_add_search_directory(const std::string& domain, fs::path path)
{
	if (!path.empty())
		DataFileStaticHolder::search_directories[domain].push_back(std::move(path));
}



/// Get currently registered search directories (a copy is returned)
inline std::vector<fs::path> data_file_get_search_directories(const std::string& domain)
{
	if (DataFileStaticHolder::search_directories.count(domain) > 0) {
		return DataFileStaticHolder::search_directories.at(domain);
	}
	return {};
}



/// Set a directory list for a search path
inline void data_file_set_search_directories(const std::string& domain, std::vector<fs::path> dirs)
{
	DataFileStaticHolder::search_directories.insert_or_assign(domain, std::move(dirs));
}



/// Find a data file (using a file name) in a search directory list.
/// \return A full path to filename, or empty string on "not found".
inline fs::path data_file_find(const std::string& domain, const std::string& filename, bool allow_to_be_directory = false)
{
	if (filename.empty())
		return {};

	if (fs::u8path(filename).is_absolute()) {  // shouldn't happen
		debug_out_error("app", DBG_FUNC_MSG
				<< "Data file \"" << filename << "\" must be relative.\n");
		return {};
	}

	auto dirs = data_file_get_search_directories(domain);
	if (dirs.empty()) {  // shouldn't happen
		debug_out_error("app", DBG_FUNC_MSG
				<< "No search directories registered for domain \"" << domain << "\".\n");
		return {};
	}

	for (const auto& dir : dirs) {
		auto file_path = dir / fs::u8path(filename);

		std::error_code ec;
		if (fs::exists(file_path, ec)) {
			if (!allow_to_be_directory && fs::is_directory(file_path, ec)) {
				debug_out_error("app", DBG_FUNC_MSG
						<< "Data file \"[" << domain << ":]" << file_path << "\" file found at \"" << dir << "\", but it is a directory.\n");
				return {};
			}
			debug_out_info("app", DBG_FUNC_MSG
					<< "Data file \"[" << domain << ":]" << file_path << "\" file found at \"" << dir << "\".\n");
			return file_path;
		}
	}

	debug_out_error("app", DBG_FUNC_MSG
			<< "Data file \"[" << domain << ":]" << filename << "\" not found.\n");
	return {};
}



/// Get data file contents.
inline std::string data_file_get_contents(const std::string& domain, const std::string& filename, std::uintmax_t max_size)
{
	auto file = data_file_find(domain, filename);
	if (!file.empty()) {
		std::string contents;
		auto ec = hz::fs_file_get_contents(file, contents, max_size);
		if (!ec) {
			return contents;
		}
		debug_out_error("app", DBG_FUNC_MSG
				<< "Data file \"[" << domain << ":]" << filename << "\" cannot be loaded: " << ec.message() << ".\n");
	}
	return {};
}




} // ns



#endif

/// @}
