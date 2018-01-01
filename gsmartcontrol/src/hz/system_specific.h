/**************************************************************************
 Copyright:
      (C) 2008 - 2013  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_SYSTEM_SPECIFIC_H
#define HZ_SYSTEM_SPECIFIC_H

#include "hz_config.h"  // feature macros

/**
\file
System/compiler-specific stuff goes here...
*/

/// \def HZ_GCC_CHECK_VERSION(major, minor, micro)
/// returns true if gcc version is greater or equal to specified.
#ifndef HZ_GCC_CHECK_VERSION
	#define HZ_GCC_CHECK_VERSION(major, minor, micro) \
			( defined (__GNUC__) && ( \
				( (__GNUC__) > (major) ) \
				|| ( ((__GNUC__) == (major)) && ((__GNUC_MINOR__) > (minor)) ) \
				|| ( ((__GNUC__) == (major)) && ((__GNUC_MINOR__) == (minor)) && ((__GNUC_PATCHLEVEL__) >= (micro)) ) \
			) )
#endif



/// \def HZ_GCC_ATTR(a)
/// Wrap gcc's attributes. Evaluates to nothing in non-gcc-compatible compilers.
#ifndef HZ_GCC_ATTR
	#ifdef __GNUC__
		#define HZ_GCC_ATTR(a) __attribute__((a))
	#else
		#define HZ_GCC_ATTR(a)
	#endif
#endif


/// \def HZ_FUNC_PRINTF_ISO_CHECK(format_idx, check_idx)
/// Easy compile-time printf format checks.
/// See http://gcc.gnu.org/onlinedocs/gcc-4.4.1/gcc/Function-Attributes.html

/// \def HZ_FUNC_PRINTF_MS_CHECK(format_idx, check_idx)
/// Easy compile-time printf format checks (windows version of format specifiers).
/// ms_printf is available since gcc 4.4.
/// Note: When using __USE_MINGW_ANSI_STDIO, mingw uses
/// its own *printf() implementation (rather than msvcrt), which accepts
/// both MS-style and standard format specifiers.
/// ms_printf gives warnings on standard specifiers, but we still keep
/// it so that the code will be portable to other win32 environments.
/// TODO: Check if simply specifying "printf" selects the correct
/// version for mingw.


#ifndef HZ_FUNC_PRINTF_ISO_CHECK
	#define HZ_FUNC_PRINTF_ISO_CHECK(format_idx, check_idx) HZ_GCC_ATTR(format(printf, format_idx, check_idx))
#endif



#include <string>

/**
\fn std::string hz::type_name_demangle(const std::string& name)
Demangle a C/C++ type name, as returned by std::type_info.name().
Similar to c++filt command. Supported under gcc only for now.
*/

#if defined HAVE_GCC_ABI_DEMANGLE && HAVE_GCC_ABI_DEMANGLE

	#include <string>
	#include <cstdlib>  // std::free().
	#include <cxxabi.h>  // __cxa_demangle


namespace hz {

	inline std::string type_name_demangle(const std::string& name)
	{
		int status = 0;
		char* demangled = ::abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
		std::string ret;
		if (demangled) {
			if (status == 0)
				ret = demangled;
			std::free(demangled);
		}
		return ret;
	}

}  // ns hz


#else  // non-gcc-compatible

namespace hz {

	inline std::string type_name_demangle(const std::string& name)
	{
		return name;  // can't do anything here
	}

}  // ns hz


#endif





#endif

/// @}
