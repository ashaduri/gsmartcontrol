/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_ENV_TOOLS_H
#define HZ_ENV_TOOLS_H

#include "hz_config.h"  // feature macros

#include <string>


// Note: This file needs glib to work on Windows.

// Don't check it explicitly - maybe they are provided by a third-party
// library.
// #if defined _WIN32 && !defined ENABLE_GLIB
// 	#error "env_tools.h needs Glib on Windows!"
// #endif

#if defined ENABLE_GLIB
	#include <glib.h>
#else
	#include <cstdlib>  // std::getenv
	#include <stdlib.h>  // setenv, unsetenv
	// setenv(), unsetenv(): _BSD_SOURCE || _POSIX_C_SOURCE >= 200112L ||
	// _XOPEN_SOURCE >= 600
#endif



namespace hz {



bool env_get_value(const std::string& name, std::string& value)
{
	const char* v = 0;
#ifdef ENABLE_GLIB
	v = g_getenv(name.c_str());
#else
	v = std::getenv(name.c_str());
#endif
	if (!v)
		return false;

	value = v;
	return true;
}



bool env_set_value(const std::string& name, const std::string& value, bool overwrite = true)
{
#ifdef ENABLE_GLIB
	return g_setenv(name.c_str(), value.c_str(), overwrite) != 0;
#else
	// setenv returns -1 if there's insufficient space in environment
	// or it's an invalid name (EINVAL errno is set).
	return setenv(name.c_str(), value.c_str(), overwrite) == 0;
#endif

}



bool env_unset_value(const std::string& name)
{
#ifdef ENABLE_GLIB
	g_unsetenv(name.c_str());  // returns void. may not be thread-safe!
	return true;
#else
	// unsetenv returns -1 on error with errno EINVAL set (invalid char in name).
	return unsetenv(name.c_str()) == 0;
#endif

}



// Temporarily change a value of an environment variable.

class ScopedEnv {

	public:

		// change environment variable value.
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

		// change back
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


		bool bad() const
		{
			return error_;
		}


		// check if the old value was set
		bool get_old_set() const
		{
			return old_set_;
		}

		// get old value
		std::string get_old_value() const
		{
			return old_value_;
		}


	private:
		std::string name_;
		std::string old_value_;
		bool do_change_;
		bool old_set_;
		bool error_;

};





}  // ns




#endif
