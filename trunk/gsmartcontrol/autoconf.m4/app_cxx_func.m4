
############################################################################
# Copyright:
#      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# Check whether C++ compiler supports C99 __func__.

AC_DEFUN([APP_CXX___func__], [
	AC_CACHE_CHECK(whether $CXX recognizes __func__, app_cv_cxx___func__, [
		AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]],
			[[const char* s = __func__;]])],
		[app_cv_cxx___func__=yes], [app_cv_cxx___func__=no])
		AC_LANG_POP([])
	])
	if test "x$app_cv_cxx___func__" = "xyes"; then
		AC_DEFINE(HAVE_CXX___func__, 1,
			[Defined to 1 if the C++ complier supports __func__, 0 otherwise])
	else
		AC_DEFINE(HAVE_CXX___func__, 0,
			[Defined to 1 if the C++ complier supports __func__, 0 otherwise])
	fi
])



# Check whether C++ compiler supports __FUNCTION__.

AC_DEFUN([APP_CXX___FUNCTION__], [
	AC_CACHE_CHECK(whether $CXX recognizes __FUNCTION__, app_cv_cxx___FUNCTION__, [
		AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]],
			[[const char* s = __FUNCTION__;]])],
		[app_cv_cxx___FUNCTION__=yes], [app_cv_cxx___FUNCTION__=no])
		AC_LANG_POP([])
	])
	if test "x$app_cv_cxx___FUNCTION__" = "xyes"; then
		AC_DEFINE(HAVE_CXX___FUNCTION__, 1,
			[Defined to 1 if the C++ complier supports __FUNCTION__, 0 otherwise])
	else
		AC_DEFINE(HAVE_CXX___FUNCTION__, 0,
			[Defined to 1 if the C++ complier supports __FUNCTION__, 0 otherwise])
	fi
])



