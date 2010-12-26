
############################################################################
# Copyright:
#      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# APP_AUTO_CLEAR_FLAGS()
# Clear user-specified CFLAGS / CXXFLAGS / LIBS / LDFLAGS if requested.

# You should call this macro before any of AC_PROG_C(XX) checks to
# avoid checking with user-specified options.

AC_DEFUN([APP_AUTO_CLEAR_FLAGS], [

	AC_ARG_ENABLE(user-flags, AS_HELP_STRING([--disable-user-flags],
			[ignore user-supplied compiler flags (default: user flags are enabled)]),
		[app_cv_compiler_user_flags=${enableval}], [app_cv_compiler_user_flags=yes])

	AC_MSG_NOTICE([Enable user-specified compiler flags: $app_cv_compiler_user_flags])

	# Reset user-supplied flags if requested.
	if test "x$app_cv_compiler_user_flags" = "xno"; then
		CFLAGS="";
		CXXFLAGS="";
		LIBS="";
		LDFLAGS="";
	fi

])



