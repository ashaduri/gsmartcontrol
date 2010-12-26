# ===========================================================================
#           http://autoconf-archive.cryp.to/ac_cxx_exceptions.html
# ===========================================================================
#
# SYNOPSIS
#
#   AC_CXX_EXCEPTIONS
#
# DESCRIPTION
#
#   If the C++ compiler supports exceptions handling (try, throw and catch),
#   define HAVE_EXCEPTIONS.
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

AC_DEFUN([AC_CXX_EXCEPTIONS],
[AC_CACHE_CHECK(whether the compiler supports exceptions,
ac_cv_cxx_exceptions,
[AC_LANG_PUSH([C++])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[try { throw  1; } catch (int i) { return i; }]])],[ac_cv_cxx_exceptions=yes],[ac_cv_cxx_exceptions=no])
 AC_LANG_POP([])
])
if test "$ac_cv_cxx_exceptions" = yes; then
  AC_DEFINE(HAVE_EXCEPTIONS,,[define if the compiler supports exceptions])
fi
])
