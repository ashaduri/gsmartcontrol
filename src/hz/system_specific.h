/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYSTEM_SPECIFIC_H
#define HZ_SYSTEM_SPECIFIC_H

#include "hz_config.h"  // feature macros


// System/compiler-specific stuff goes here...


// returns true if gcc version is greater or equal to specified.
#ifndef HZ_GCC_CHECK_VERSION
	#define HZ_GCC_CHECK_VERSION(major, minor, micro) \
			( defined (__GNUC__) && ( \
				( (__GNUC__) > (major) ) \
				|| ( ((__GNUC__) == (major)) && ((__GNUC_MINOR__) > (minor)) ) \
				|| ( ((__GNUC__) == (major)) && ((__GNUC_MINOR__) == (minor)) && ((__GNUC_PATCHLEVEL__) >= (micro)) ) \
			) )
#endif



// Wrap gcc's attributes
#ifndef HZ_GCC_ATTR
	#ifdef __GNUC__
		#define HZ_GCC_ATTR(a) __attribute__((a))
	#else
		#define HZ_GCC_ATTR(a)
	#endif
#endif


// for easy printf format checks.
// ms_printf is not available as of 4.3, but present in docs. TODO: check with 4.4 when it's out.
#ifndef HZ_FUNC_PRINTF_CHECK
	#if defined _WIN32 && HZ_GCC_CHECK_VERSION(4, 4, 0)
		#define HZ_FUNC_PRINTF_CHECK(format_idx, check_idx) HZ_GCC_ATTR(format(ms_printf, format_idx, check_idx))
	#else
		#define HZ_FUNC_PRINTF_CHECK(format_idx, check_idx) HZ_GCC_ATTR(format(printf, format_idx, check_idx))
	#endif
#endif




#include <string>


// this also works with intel/linux (__GNUC__ defined by default in it).
// we still leave __GNUC__ for autoconf-less setups.
#if defined HAVE_GCC_ABI_DEMANGLE || defined __GNUC__

	#include <cxxabi.h>  // __cxa_demangle
	#include <stdlib.h>  // free(). don't use the C++ version, this one is allocated internally.


namespace hz {

	// accepts input string as given by std::type_info.name().
	inline std::string type_name_demangle(const std::string& name)
	{
		int status = 0;
		char* demangled = ::abi::__cxa_demangle(name.c_str(), NULL, NULL, &status);
		std::string ret;
		if (demangled) {
			if (status == 0)
				ret = demangled;
			free(demangled);
		}
		return ret;
	}

}  // ns hz


#else // non-gcc-compatible

namespace hz {

	inline std::string type_name_demangle(const std::string& name)
	{
		return name;  // can't do anything here
	}

}  // ns hz


#endif





#endif
