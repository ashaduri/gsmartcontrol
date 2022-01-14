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

#ifndef HZ_ENV_TOOLS_H
#define HZ_ENV_TOOLS_H

#include <string>
#include <memory>
#include <utility>


/// \def HAVE_SETENV
/// Defined to 0 or 1. If 1, the compiler has setenv() and unsetenv().
#ifndef HAVE_SETENV
// setenv(), unsetenv() glibc feature test macros:
// _BSD_SOURCE || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600.
// However, since they are available almost anywhere except windows,
// we just test for that.
	#if defined _WIN32
		#define HAVE_SETENV 0
	#else
		#define HAVE_SETENV 1
	#endif
#endif


#if defined _WIN32
	#include <windows.h>  // winapi stuff
	#include "win32_tools.h"  // win32_*
#elif defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>
#else
	#include <cstdlib>  // for stdlib.h, std::getenv
	#include <cstring>  // std::malloc
	#include <stdlib.h>  // setenv, unsetenv, putenv
#endif


/**
\file
Environment manipulation functions.
On windows, these always work with utf-8 strings.
*/


namespace hz {


/// Get environment variable value.
/// \return false if error or no such variable.
inline bool env_get_value(const std::string& name, std::string& value)
{
	if (name.empty())
		return false;

#if defined _WIN32

	// getenv and _wgetenv are not thread-safe under windows, so use winapi.

	std::wstring wname = hz::win32_utf8_to_utf16(name);
	if (wname.empty())  // conversion error
		return false;

	wchar_t dummy[2];
	// Determine the size of needed buffer by supplying an undersized one.
	// If it fits, returns number of chars not including 0. If not, returns the number
	// of chars needed for full data, including 0.
	DWORD len = GetEnvironmentVariableW(wname.c_str(), dummy, 2);

	if (len == 0) {  // failure
		return false;
	}
	if (len == 1) {  // whaddayaknow, it fit
		len = 2;
	}

	auto wvalue = std::make_unique<wchar_t[]>(len);
	if (GetEnvironmentVariableW(wname.c_str(), wvalue.get(), len) != len - 1) {  // failure
		return false;
	}

	if (wcschr(wvalue.get(), L'%') != 0) {  // check for embedded variables

		// This always includes 0 in the returned length. Go figure.
		len = ExpandEnvironmentStringsW(wvalue.get(), dummy, 2);  // get the needed length

		if (len > 0) {  // not failure
			auto wvalue_exp = std::make_unique<wchar_t[]>(len);
			if (ExpandEnvironmentStringsW(wvalue.get(), wvalue_exp.get(), len) != len)  // failure
				return false;

			value = hz::win32_utf16_to_utf8(wvalue_exp.get());
			return true;
		}
	}

	value = hz::win32_utf16_to_utf8(wvalue.get());
	return true;


#elif defined ENABLE_GLIB && ENABLE_GLIB
	// glib version may be thread-unsafe on win32, so don't use it there.
	const char* v = g_getenv(name.c_str());
	if (!v)
		return false;
	value = v;
	return true;

#else
	// Since C++11, getenv has to be thread-safe (unless some other thread is
	// modifying the environment).
	const char* v = std::getenv(name.c_str());
	if (!v)
		return false;
	value = v;
	return true;

#endif
}



/// Set environment variable. If \c overwrite is false, the value won't
/// be overwritten if it already exists.
/// NOTE: This function is not thread-safe in GLibc and should only be invoked at the start of the program.
/// \return true if the value was written successfully.
inline bool env_set_value(const std::string& name, const std::string& value, bool overwrite = true)
{
	if (name.empty() || name.find('=') != std::string::npos)
		return false;

#if defined _WIN32
	std::string oldvalue;
	if (!overwrite && env_get_value(name, oldvalue))
		return true;

	std::wstring wname = hz::win32_utf8_to_utf16(name);
	std::wstring wvalue = hz::win32_utf8_to_utf16(value);

	if (wname.empty() || wvalue.empty())  // conversion error
		return false;

	// Since using main() instead of wmain() causes environment tables to be
	// desynchronized, calling _wputenv() could correct the _wgetenv() table.
	// However, both of them are not thread-safe (their secure variants too),
	// so we avoid them.

	return (SetEnvironmentVariableW(wname.c_str(), wvalue.c_str()) != 0);

	// glib version may be thread-unsafe on win32, so don't use it there.
#elif defined ENABLE_GLIB && ENABLE_GLIB
	return g_setenv(name.c_str(), value.c_str(), static_cast<gboolean>(overwrite)) != 0;  // may be thread-unsafe


#elif defined HAVE_SETENV && HAVE_SETENV
	// setenv returns -1 if there's insufficient space in environment
	// or it's an invalid name (EINVAL errno is set).
	return setenv(name.c_str(), value.c_str(), overwrite) == 0;

#else
	std::string oldvalue;
	if (!overwrite && env_get_value(name, oldvalue)) {
		return true;
	}
	std::string putenv_arg = name + "=" + value;
	char* buf = (char*)std::malloc(putenv_arg.size() + 1);
	std::memcpy(buf, putenv_arg.c_str(), putenv_arg.size() + 1);
	return putenv(buf) == 0;  // returns 0 on success. Potentially leaks, but it is the way putenv() behaves.
#endif

}



/// Unset an environment variable.
/// NOTE: This function is not thread-safe in GLibc and should only be invoked at the start of the program.
inline bool env_unset_value(const std::string& name)
{
	if (name.empty() || name.find('=') != std::string::npos)
		return false;

#if defined _WIN32
	std::wstring wname = hz::win32_utf8_to_utf16(name);

	if (!wname.empty()) {
		// we could do _wputenv("name=") or _wputenv_s(...) too, but they're not thread-safe.
		return SetEnvironmentVariableW(wname.c_str(), 0);
	}
	return false;

#elif defined ENABLE_GLIB && ENABLE_GLIB
	g_unsetenv(name.c_str());  // returns void. may not be thread-safe!
	return true;

#elif defined HAVE_SETENV && HAVE_SETENV
	// unsetenv returns -1 on error with errno EINVAL set (invalid char in name).
	return unsetenv(name.c_str()) == 0;

#else
	std::string putenv_arg = name + "=";
	char* buf = (char*)std::malloc(putenv_arg.size() + 1);
	std::memcpy(buf, putenv_arg.c_str(), putenv_arg.size() + 1);
	return putenv(buf) == 0;  // returns 0 on success. Potentially leaks, but it is the way putenv() behaves.
#endif

}





}  // ns




#endif

/// @}
