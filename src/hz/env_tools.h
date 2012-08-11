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

#ifndef HZ_ENV_TOOLS_H
#define HZ_ENV_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>

#if defined _WIN32
	#include <windows.h>  // winapi stuff
	#include "scoped_array.h"
	#include "win32_tools.h"  // win32_*
#elif defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>
#else
	#include <cstdlib>  // for stdlib.h, std::getenv
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

	hz::scoped_array<wchar_t> wname(hz::win32_utf8_to_utf16(name.c_str()));
	if (!wname)  // conversion error
		return false;

	wchar_t dummy[2];
	// Determine the size of needed buffer by supplying an undersized one.
	// If it fits, returns number of chars not including 0. If not, returns the number
	// of chars needed for full data, including 0.
	DWORD len = GetEnvironmentVariableW(wname.get(), dummy, 2);

	if (len == 0) {  // failure
		return false;
	}
	if (len == 1) {  // whaddayaknow, it fit
		len = 2;
	}

	hz::scoped_array<wchar_t> wvalue(new wchar_t[len]);

	if (GetEnvironmentVariableW(wname.get(), wvalue.get(), len) != len - 1) {  // failure
		return false;
	}

	if (wcschr(wvalue.get(), L'%') != 0) {  // check for embedded variables

		// This always includes 0 in the returned length. Go figure.
		len = ExpandEnvironmentStringsW(wvalue.get(), dummy, 2);  // get the needed length

		if (len > 0) {  // not failure
			hz::scoped_array<wchar_t> wvalue_exp(new wchar_t[len]);

			if (ExpandEnvironmentStringsW(wvalue.get(), wvalue_exp.get(), len) != len)  // failure
				return false;

			hz::scoped_array<char> val(hz::win32_utf16_to_utf8(wvalue_exp.get()));
			if (val) {
				value = val.get();
			}
			return val;
		}
	}

	hz::scoped_array<char> val(hz::win32_utf16_to_utf8(wvalue.get()));
	if (val) {
		value = val.get();
	}
	return val;


	// glib version may be thread-unsafe on win32, so don't use it there.
#elif defined ENABLE_GLIB && ENABLE_GLIB
	const char* v = g_getenv(name.c_str());
	if (!v)
		return false;
	value = v;
	return true;


#else
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

	hz::scoped_array<wchar_t> wname(hz::win32_utf8_to_utf16(name.c_str()));
	hz::scoped_array<wchar_t> wvalue(hz::win32_utf8_to_utf16(value.c_str()));

	if (!wname || !wvalue)  // conversion error
		return false;

	// Since using main() instead of wmain() causes environment tables to be
	// desynchronized, calling _wputenv() could correct the _wgetenv() table.
	// However, both of them are not thread-safe (their secure variants too),
	// so we avoid them.

	// hz::scoped_array<wchar_t> put_arg(hz::win32_utf8_to_utf16((name + "=" + value).c_str()));
	// if (put_arg)
	// 	_wputenv(put_arg.get());
	if (wname) {
		return (SetEnvironmentVariableW(wname.get(), wvalue.get()) != 0);
	}
	return false;

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
	return putenv((name + "=" + value).c_str()) == 0;
#endif

}



/// Unset an environment variable
inline bool env_unset_value(const std::string& name)
{
	if (name.empty() || name.find('=') != std::string::npos)
		return false;

#if defined _WIN32
	hz::scoped_array<wchar_t> wname(hz::win32_utf8_to_utf16(name.c_str()));

	if (wname) {
		// we could do _wputenv("name=") or _wputenv_s(...) too, but they're not thread-safe.
		return SetEnvironmentVariableW(wname.get(), 0);
	}
	return false;

#elif defined ENABLE_GLIB && ENABLE_GLIB
	g_unsetenv(name.c_str());  // returns void. may not be thread-safe!
	return true;

#elif !(defined HAVE_SETENV && HAVE_SETENV)
	// unsetenv returns -1 on error with errno EINVAL set (invalid char in name).
	return unsetenv(name.c_str()) == 0;

#else
	return putenv((name + "=").c_str()) == 0;  // returns 0 on success
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
