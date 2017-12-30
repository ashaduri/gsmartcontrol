# ===========================================================================
#              http://autoconf-archive.cryp.to/ac_cxx_rtti.html
# ===========================================================================
#
# SYNOPSIS
#
#   AC_CXX_RTTI
#
# DESCRIPTION
#
#   If the compiler supports Run-Time Type Identification (typeinfo header
#   and typeid keyword), define HAVE_RTTI.
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Todd Veldhuizen
#   Copyright (c) 2008 Luc Maisonobe <luc@spaceroots.org>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AC_CXX_RTTI],
[AC_CACHE_CHECK(whether the compiler supports Run-Time Type Identification,
	ac_cv_cxx_rtti,
	[AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <typeinfo>
			class Base {
				public:
					Base () {}
					virtual int f () { return 0; }
			};
			class Derived : public Base {
				public :
					Derived () {}
					virtual int f () { return 1; }
			};
		]], [[
			Derived d;
			Base* ptr = &d;
			return typeid (*ptr) == typeid (Derived);
		]])],
		[ac_cv_cxx_rtti=yes], [ac_cv_cxx_rtti=no])
		AC_LANG_POP([])
	])
	if test "$ac_cv_cxx_rtti" = yes; then
		AC_DEFINE(HAVE_RTTI, 1,
			[defined to 1 if the compiler supports Run-Time Type Identification, 0 otherwise])
	else
		AC_DEFINE(HAVE_RTTI, 0,
			[defined to 1 if the compiler supports Run-Time Type Identification, 0 otherwise])
	fi
])



