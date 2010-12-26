# ===========================================================================
#        http://autoconf-archive.cryp.to/ax_cxx_gcc_abi_demangle.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CXX_GCC_ABI_DEMANGLE
#
# DESCRIPTION
#
#   If the compiler supports GCC C++ ABI name demangling (has header
#   cxxabi.h and abi::__cxa_demangle() function), define
#   HAVE_GCC_ABI_DEMANGLE
#
#   Adapted from AC_CXX_RTTI by Luc Maisonobe
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Neil Ferguson <nferguso@eso.org>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AX_CXX_GCC_ABI_DEMANGLE],
[AC_CACHE_CHECK(whether the compiler supports GCC C++ ABI name demangling,
	ac_cv_cxx_gcc_abi_demangle,
	[AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			#include <typeinfo>
			#include <cxxabi.h>
			#include <string>
			#include <stdlib.h>  /* not cstdlib */

			template<typename TYPE>
			class A {};
		]], [[
			A<int> instance;
			int status = 0;
			char* c_name = 0;

			c_name = abi::__cxa_demangle(typeid(instance).name(), 0, 0, &status);

			std::string name(c_name);
			free(c_name);  /* not std::free() */

			return name == "A<int>";
		]])],
		[ac_cv_cxx_gcc_abi_demangle=yes], [ac_cv_cxx_gcc_abi_demangle=no])
		AC_LANG_POP([])
	])
	if test "$ac_cv_cxx_gcc_abi_demangle" = yes; then
		AC_DEFINE(HAVE_GCC_ABI_DEMANGLE, 1,
			[defined to 1 if the compiler supports GCC C++ ABI name demangling, 0 otherwise])
	else
		AC_DEFINE(HAVE_GCC_ABI_DEMANGLE, 0,
			[defined to 1 if the compiler supports GCC C++ ABI name demangling, 0 otherwise])
	fi
])




