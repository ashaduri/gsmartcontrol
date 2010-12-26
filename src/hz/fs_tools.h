/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_FS_TOOLS_H
#define HZ_FS_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstdlib>  // std::getenv(), std::size_t

#ifdef _WIN32
	#include <io.h>  // getcwd, chdir
#else
	#include <unistd.h>  // getcwd, chdir
#endif

#include "win32_tools.h"  // registry stuff


// Filesystem utilities


namespace hz {



// Get the current user's home directory (in native fs encoding).
// Return an empty string if not found.
inline std::string get_home_dir()
{
	// Do NOT use g_get_home_dir, it doesn't work consistently in win32
	// between glib versions.

#ifndef _WIN32
	const char* e = std::getenv("HOME");  // works well enough
	std::string dir = (e ? e : "");

#else  // win32
	std::string dir;
	win32_get_registry_value_string(dir, HKEY_CURRENT_USER,
				"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Personal");

	if (dir.empty()) {
		win32_get_registry_value_string(dir, HKEY_CURRENT_USER,
				"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "User");
	}
#endif

	return dir;
}



// Get current working directory
inline std::string get_current_dir()
{
	std::size_t buf_size = 1024;
	char* buf = new char[buf_size];

	while (getcwd(buf, buf_size) == NULL) {
		delete[] buf;
		buf = new char[buf_size *= 2];
	}

	std::string dir = (buf ? buf : "");
	delete[] buf;

	return dir;
}



// Change current directory.
inline bool set_current_dir(const std::string& dir)
{
	return (chdir(dir.c_str()) == 0);
}



// Get temporary directory of a system (can be user-specific)
inline std::string get_tmp_dir()
{
	const char* e = std::getenv("TMPDIR");
	if (!e)
		e = std::getenv("TMP");
	if (!e)
		e = std::getenv("TEMP");

#ifndef _WIN32
	if (!e)
		e = "/tmp";
#endif

	return std::string(e ? e : "");
}






}  // ns hz



#endif
