
############################################################################
# Copyright:
#      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# Check whether C++ compiler supports overloading between
# extern "C" function pointer and C++ function pointer. GCC and
# Intel treat them equally, so no overloads are needed (or accepted)
# there.

# SunCC accepts C++-to-C function pointer assignment in function
# parameters, but not in constructor parameters. To make such
# code work an additional C-function-pointer constructor must be
# specified. Then the constructor itself can make the conversion
# (with a compiler warning though).

# SunCC also disallows C-to-C++ function pointer specification
# for template parameters (I don't remember the exact details).


# If such support is found, HAVE_CXX_EXTERN_C_OVERLOAD is defined.


AC_DEFUN([APP_CXX_EXTERN_C_OVERLOAD], [
	AC_CACHE_CHECK(whether $CXX accepts extern C function pointer overload, ac_cv_cxx_extern_c_overload, [
		AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			extern "C" { typedef void (*c_func_t)(void); }
			typedef void (*cpp_func_t)(void);
			void f1(c_func_t f) { }
			void f1(cpp_func_t f) { }
		]], [[]])],
		[ac_cv_cxx_extern_c_overload=yes], [ac_cv_cxx_extern_c_overload=no])
		AC_LANG_POP([])
	])
	if test "x$ac_cv_cxx_extern_c_overload" = "xyes"; then
		AC_DEFINE(HAVE_CXX_EXTERN_C_OVERLOAD, ,
			[Define if the C++ complier accepts extern C function pointer overload])
	fi
])



