# ===========================================================================
#           http://autoconf-archive.cryp.to/ac_cxx_namespaces.html
# ===========================================================================
#
# SYNOPSIS
#
#   AC_CXX_NAMESPACES
#
# DESCRIPTION
#
#   If the compiler can prevent names clashes using namespaces, define
#   HAVE_NAMESPACES.
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

AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK(whether the compiler implements namespaces,
	ac_cv_cxx_namespaces,
	[AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
			[[namespace Outer { namespace Inner { int i = 0; }}]],
			[[using namespace Outer::Inner; return i;]])],
			[ac_cv_cxx_namespaces=yes], [ac_cv_cxx_namespaces=no])
		AC_LANG_POP([])
	])
	if test "$ac_cv_cxx_namespaces" = yes; then
		AC_DEFINE(HAVE_NAMESPACES, 1,
			[defined to 1 if the compiler implements namespaces, 0 otherwise])
	else
		AC_DEFINE(HAVE_NAMESPACES, 0,
			[defined to 1 if the compiler implements namespaces, 0 otherwise])
	fi
])



