/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FS_PATH_UTILS_H
#define HZ_FS_PATH_UTILS_H

#include <string>

#if !defined _WIN32
	#include <libgen.h>  // dirname, basename
	#include <cstring>  // strncpy
	#include <cstddef>  // std::size_t
#endif

#include "fs_common.h"  // separator


/**
\file
Filesystem path string manipulation.
For windows, always supply utf-8 or current locale-encoded strings.
Paths like \\.\ and \\?\ are not supported for windows (yet).
*/

namespace hz {



/// Convert path from unknown format to native (e.g. unix paths to win32).
/// Same as FsPath(path).to_native().str()
inline std::string path_to_native(const std::string& path);

/// Remove trailing slashes in path (unless they are part of root component).
inline std::string path_trim_trailing_separators(const std::string& path);

/// Check if the path is absolute (only for native paths). Return 0 if it's not.
/// The returned value is a position past the root component (e.g. 3 for C:\\temp).
inline std::string::size_type path_is_absolute(const std::string& path);


/// Get the path truncated by 1 level, e.g. /usr/local/ -> /usr.
inline std::string path_get_dirname(const std::string& path);

/// Get the basename of path, e.g. /usr/local/ -> local; /a/b -> b.
inline std::string path_get_basename(const std::string& path);

/// Get root path of current path. e.g. '/' or 'D:\'.
/// May not work with relative paths under win32.
inline std::string path_get_root(const std::string& path);


/// Change the supplied filename so that it's safe to create it
/// (remove any potentially harmful characters from it).
inline std::string filename_make_safe(const std::string& filename);

/// Change the supplied path so that it's safe to create it
/// (remove any potentially harmful characters from it).
inline std::string path_make_safe(const std::string& path);





// ------------------------------------------- Implementation



inline std::string path_to_native(const std::string& path)
{
	std::string s(path);
	std::string::size_type pos = 0;

	char from = '\\';
	if (DIR_SEPARATOR == '\\')
		from = '/';

	while ((pos = s.find(from, pos)) != std::string::npos) {
		s[pos] = DIR_SEPARATOR;
		++pos;
	}

	return s;
}



inline std::string path_trim_trailing_separators(const std::string& path)
{
	std::string::size_type apos = path_is_absolute(path);  // first position of non-abs portion
	if (apos >= path.size())  // / or similar
		return path;

	std::string::size_type pos = path.find_last_not_of(DIR_SEPARATOR);  // remove trailing slashes
	if (pos == std::string::npos)  // / ?
		return path.substr(0, apos);

	return path.substr(0, pos + 1);
}



inline std::string::size_type path_is_absolute(const std::string& path)
{
#ifndef _WIN32
	if (!path.empty() && path[0] == '/')
		return 1;

#else  // win32
	if (path.size() >= 3 && path.substr(1, 2) == ":\\")  // 'D:\'
		return 3;

	if (path.size() >= 4 && path.substr(0, 2) == "\\\\") {  // '\\host\'
		std::string::size_type pos = path.rfind('\\');
		if (pos >= 3 && pos != std::string::npos)
			return pos + 1;
	}
#endif

	return 0;
}



inline std::string path_get_dirname(const std::string& path)
{
// GLib (as of 2.14.1) has a bug in g_path_get_dirname() implementation:
// "/usr/local/" returns "/usr/local", not "/usr". Don't use it.

#if !defined _WIN32
	std::size_t buf_size = path.size() + 1;
	char* buf = new char[buf_size];
	std::strncpy(buf, path.c_str(), buf_size);
	std::string ret = dirname(buf);  // dirname may modify buf, that's why we needed a copy of it.
	delete[] buf;
	return ret;

#else
	if (path.empty())
		return ".";

	std::string::size_type apos = path_is_absolute(path);  // first position of non-abs portion
	if (apos >= path.size())  // / or similar
		return path;

	std::string::size_type pos2 = path.find_last_not_of(DIR_SEPARATOR);  // remove trailing slashes
	if (pos2 == std::string::npos)  // / ?
		return path.substr(0, apos);

	std::string::size_type pos1 = path.find_last_of(DIR_SEPARATOR, pos2);  // next slash from the end
	if (pos1 == std::string::npos) {
		return ".";  // one-component relative dir
	}
	if (apos && pos1 == apos - 1) {  // it's a root subdir
		return path.substr(0, apos);
	}

	pos1 = path.find_last_not_of(DIR_SEPARATOR, pos1);  // skip duplicate slashes
	if (pos1 == std::string::npos && apos)  // it's root subdir
		return path.substr(0, apos);

	std::string dir = path.substr(0, pos1+1);
	if (dir.empty())
		return ".";

	return dir;
#endif
}



inline std::string path_get_basename(const std::string& path)
{
#if !defined _WIN32
	std::size_t buf_size = path.size() + 1;
	char* buf = new char[buf_size];
	std::strncpy(buf, path.c_str(), buf_size);
	std::string ret = basename(buf);  // basename may modify buf, that's why we needed a copy of it.
	delete[] buf;
	return ret;

#else
	if (path.empty())
		return ".";

	std::string::size_type apos = path_is_absolute(path);  // first position of non-abs portion
	if (apos >= path.size())  // / or similar
		return path;  // / -> /, as per basename manpage

	std::string::size_type pos2 = path.find_last_not_of(DIR_SEPARATOR);  // remove trailing slashes
	std::string::size_type pos1 = path.find_last_of(DIR_SEPARATOR, pos2);
	pos1 = (pos1 == std::string::npos ? 0 : (pos1 + 1));
	pos2 = (pos2 == std::string::npos ? path.size() : (pos2 + 1));

	return path.substr(pos1, pos2 - pos1);
#endif
}



inline std::string path_get_root([[maybe_unused]] const std::string& path)
{
#if !defined _WIN32
	return "/";  // easy

#else  // hard
	// don't use path_is_absolute(), we have slightly more error-checking.
	if (path.size() >= 3 && path.substr(1, 2) == ":\\")  // 'D:\'
		return path.substr(0, 3);

	if (path.size() >= 4 && path.substr(0, 2) == "\\\\") {  // '\\host\', '\\.\', '\\?\'
		std::string::size_type pos = path.rfind('\\');
		if (pos >= 3 && pos != std::string::npos)
			return path.substr(0, pos+1);
	}
	return std::string();  // cannot detect
#endif
}



inline std::string path_compress(const std::string& path)
{
	std::string::size_type rel_pos = path_is_absolute(path);
	std::string rel = path.substr(rel_pos);  // retrieve relative component only

	std::string::size_type curr = 0, last = 0;
	std::string::size_type end = rel.size();
	std::string result, component;

	while (true) {
		if (last >= end)  // last is past the end
			break;

		curr = rel.find(DIR_SEPARATOR, last);
		if (curr != last) {
			component = rel.substr(last, (curr == std::string::npos ? curr : (curr - last)));

			if (component == ".") {
				if (result == "" && rel_pos == 0)  // don't un-relativise
					result += (std::string(".") + DIR_SEPARATOR_S);
				// else, nothing

			} else if (component == "..") {
				// retain ".." when previous component is ".." or ".".
				if (result == "" || result == (std::string(".") + DIR_SEPARATOR_S)
						|| (result.size() >= 3 && result.substr(result.size() - 3) == (std::string("..") + DIR_SEPARATOR_S))) {
					result += (std::string("..") + DIR_SEPARATOR_S);

				} else {
					// std::cerr << "Getting dirname on \"" << result << "\"\n";
					std::string up = path_get_dirname(result);  // go up
					if (up == ".") {
						result = "";
					} else {
						result = (up + DIR_SEPARATOR_S);
					}
				}

			} else {
				result += (component + DIR_SEPARATOR_S);
			}
		}

		if (curr == std::string::npos)
			break;
		last = curr + 1;
	}

	return path_trim_trailing_separators(path.substr(0, rel_pos) + result);
}



inline std::string filename_make_safe(const std::string& filename)
{
	std::string s(filename);
	std::string::size_type pos = 0;
	while ((pos = s.find_first_not_of(
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890._-",
			pos)) != std::string::npos) {
		s[pos] = '_';
		++pos;
	}
	// win32 kernel (heh) has trouble with space and dot-ending files
	if (!s.empty() && (s[s.size() - 1] == '.' || s[s.size() - 1] == ' ')) {
		s[s.size() - 1] = '_';
	}
	return s;
}



inline std::string path_make_safe(const std::string& path)
{
	std::string s(path);
	std::string::size_type pos = 0;
	while ((pos = s.find_first_not_of(
			"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890._-",
			pos)) != std::string::npos) {
		if (s[pos] != DIR_SEPARATOR)
			s[pos] = '_';
		++pos;
	}
	// win32 kernel (heh) has trouble with space and dot-ending files
	if (!s.empty() && (s[s.size() - 1] == '.' || s[s.size() - 1] == ' ')) {
		s[s.size() - 1] = '_';
	}
	return s;
}





}  // ns hz



#endif

/// @}
