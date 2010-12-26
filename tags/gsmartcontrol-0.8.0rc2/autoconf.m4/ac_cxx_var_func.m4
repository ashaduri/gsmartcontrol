


AC_DEFUN([AC_CXX_VAR_FUNC],
[AC_CACHE_CHECK(whether $CXX recognizes __func__,
ac_cv_cxx_var_func,
[AC_LANG_PUSH([C++])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[const char* s = __func__;]])],[ac_cv_cxx_var_func=yes],[ac_cv_cxx_var_func=no])
 AC_LANG_POP([])
])
if test "$ac_cv_cxx_var_func" = yes; then
  AC_DEFINE(HAVE_CXX__FUNC,,[Define if the C++ complier supports __func__])
fi
])


