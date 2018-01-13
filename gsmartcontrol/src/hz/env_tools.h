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

#ifndef HZ_ENV_TOOLS_H
#define HZ_ENV_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>

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
	return g_setenv(name.c_str(), value.c_str(), overwrite) != 0;  // may be thread-unsafe


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



/// Unset an environment variable
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



/// Temporarily change a value of an environment variable (for as long
/// as an object of this class exists).
class ScopedEnv {

	public:

		/// Constructor.
		/// \param name variable name
		/// \param value variable value to set
		/// \param do_change if false, no operation will be performed. This is useful if you need
		/// 	to conditionally set a variable (you can't practically declare a scoped variable inside
		/// 	a conditional block to be used outside it).
		/// \param overwrite if false and the variable already exists, don't change it.
		ScopedEnv(const std::string& name, const std::string& value, bool do_change = true, bool overwrite = true)
				: name_(name), do_change_(do_change), old_set_(false), error_(false)
		{
			if (do_change_) {
				old_set_ = env_get_value(name_, old_value_);
				if (old_set_ && !overwrite) {
					do_change_ = false;
				} else {
					error_ = !env_set_value(name_, value, true);
				}
			}
		}


		/// Destructor, changes back the variable to the old value
		~ScopedEnv()
		{
			if (do_change_) {
				if (old_set_) {
					env_set_value(name_, old_value_, true);
				} else {
					env_unset_value(name_);
				}
			}
		}


		/// If true, there was an error setting the value.
		bool bad() const
		{
			return error_;
		}


		/// Check if there was a value before we set it
		bool get_old_set() const
		{
			return old_set_;
		}


		/// Get the old variable value
		std::string get_old_value() const
		{
			return old_value_;
		}


	private:

		std::string name_;  ///< Variable name
		std::string old_value_;  ///< Old value
		bool do_change_;  ///< If false, don't do anything
		bool old_set_;  ///< If false, there was no variable before we set it
		bool error_;  ///< If true, there was an error setting the value

};





}  // ns




#endif

/// @}
