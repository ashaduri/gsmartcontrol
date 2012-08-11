/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_FS_TOOLS_H
#define HZ_FS_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>
#include <cstddef>  // std::size_t

#ifdef _WIN32
	#include <direct.h>  // _wgetcwd, _wchdir
#else
	#include <unistd.h>  // getcwd, chdir
#endif

#include "env_tools.h"  // hz::env_get_value

#ifdef _WIN32
	#include "win32_tools.h"  // win32_* stuff
	#include "scoped_array.h"  // hz::scoped_array
#endif


/**
\file
Filesystem utilities
*/


namespace hz {



/// Get the current user's configuration file directory (in native fs encoding
/// for UNIX, utf-8 for windows). E.g. "$HOME/.config" in UNIX.
inline std::string get_user_config_dir();

/// Get the current user's home directory (in native fs encoding for UNIX,
/// utf-8 for windows). This function always returns something, but
/// note that the directory may not actually exist at all.
inline std::string get_home_dir();

/// Get current working directory
inline std::string get_current_dir();

/// Change current directory.
inline bool set_current_dir(const std::string& dir);

/// Get temporary directory of a system (can be user-specific).
/// For windows it seems to be in UTF-8, for others - in fs encoding.
inline std::string get_tmp_dir();




// ------------------------- Implementation



inline std::string get_user_config_dir()
{
	std::string dir;

#ifdef _WIN32
	// that's "C:\documents and settings\username\application data".
	dir = win32_get_special_folder(CSIDL_APPDATA);
	if (dir.empty()) {
		dir = get_home_dir();  // fallback, always non-empty.
	}
#else
	if (!hz::env_get_value("XDG_CONFIG_HOME", dir)) {
		// default to $HOME/.config
		dir = get_home_dir() + DIR_SEPARATOR + ".config";
	}
#endif
	return dir;
}



inline std::string get_home_dir()
{
	// Do NOT use g_get_home_dir, it doesn't work consistently in win32
	// between glib versions.

	std::string dir;

#ifdef _WIN32
	// For windows we usually get "C:\documents and settings\username".
	// Try $USERPROFILE, then CSIDL_PROFILE, then Windows directory
	// (glib uses it, not sure why though).

	hz::env_get_value("USERPROFILE", dir);  // in utf-8

	if (dir.empty()) {
		dir = win32_get_special_folder(CSIDL_PROFILE);
	}
	if (dir.empty()) {
		dir = win32_get_windows_directory();  // always returns something.
	}

#else  // linux, etc...
	// We use $HOME to allow the user to override it.
	// Other solutions involve getpwuid_r() to read from passwd.
	if (!hz::env_get_value("HOME", dir)) {  // works well enough
		// HOME may be empty in some situations (limited shells
		// and rescue logins).
		// We could use /tmp/<username>, but obtaining the username
		// is too complicated and unportable.
		dir = get_tmp_dir();
	}

#endif

	return dir;
}



inline std::string get_current_dir()
{
	std::string dir;

#ifdef _WIN32
	std::size_t buf_size = MAX_PATH;
	wchar_t* buf = new wchar_t[buf_size];  // not sure if it's really max, so increase it on demand

	while (_wgetcwd(buf, static_cast<int>(buf_size)) == 0) {
		delete[] buf;
		buf = new wchar_t[buf_size *= 2];
	}

	dir = hz::win32_utf16_to_utf8_string(buf);

#else
	std::size_t buf_size = 1024;
	char* buf = new char[buf_size];

	while (getcwd(buf, buf_size) == 0) {
		delete[] buf;
		buf = new char[buf_size *= 2];
	}

	dir = buf;
#endif

	delete[] buf;

	return dir;
}



inline bool set_current_dir(const std::string& dir)
{
#ifdef _WIN32
	// Dont check the parameter, let the function do it.
	return (_wchdir(hz::scoped_array<wchar_t>(hz::win32_utf8_to_utf16(dir.c_str())).get()) == 0);
#else
	return (chdir(dir.c_str()) == 0);
#endif
}



inline std::string get_tmp_dir()
{
	std::string dir;

	hz::env_get_value("TMPDIR", dir);

	if (dir.empty()) {
		 hz::env_get_value("TMP", dir);
	}
	if (dir.empty()) {
		 hz::env_get_value("TEMP", dir);
	}

#ifdef _WIN32
	if (dir.empty()) {
		// not sure about this, but that's what glib does...
		dir = win32_get_windows_directory();  // always returns something
	}
#else
	if (dir.empty()) {
		dir = "/tmp";
	}
#endif

	return dir;
}






}  // ns hz



#endif

/// @}
