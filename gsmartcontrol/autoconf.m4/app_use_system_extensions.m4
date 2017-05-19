
############################################################################
# Copyright:
#      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# APP_USE_SYSTEM_EXTENSIONS([flags_prefix])
# This macro detects compiler flags which are needed to enable
# system-specific library extensions. For example, on GNU platforms
# it's -D_GNU_SOURCE, on solaris it's -D__EXTENSIONS__, etc...
# Basically, it's the same as AC_USE_SYSTEM_EXTENSIONS,
# but uses compiler flag approach instead of config.h.

# The extensions used bring the system closer to the GNU,
# X/Open (a superset of POSIX) and POSIX systems.

# You must call AX_COMPILER_VENDOR
# and APP_DETECT_OS_ENV([target], ...) before using this macro.

# More info:
# http://www.gnu.org/software/autoconf/manual/autoconf.html#Posix-Variants
# http://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
# http://gcc.gnu.org/onlinedocs/libstdc++/faq.html#faq.predefined
# http://docs.sun.com/app/docs/doc/817-3946/6mjgmt4o8?l=en&a=view

AC_DEFUN([APP_USE_SYSTEM_EXTENSIONS], [
	# These requirements check compile-time presence, not that they were called
	AC_REQUIRE([AX_COMPILER_VENDOR])
	AC_REQUIRE([APP_DETECT_OS_ENV])

	AC_MSG_CHECKING([for target system extension flags])

	app_cv_target_extension_cflags="";
	app_cv_target_extension_cxxflags="";
	app_cv_target_extension_libs="";


	# If vendor is not recognized, it's set to "unknown".
	# NOTE: This works only on target compilers!
	if test "x${ax_cv_cxx_compiler_vendor}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[ax_cv_cxx_compiler_vendor] not set.])
	fi
	if test "x${app_cv_target_os_kernel}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[app_cv_target_os_kernel] not set.])
	fi
	if test "x${app_cv_target_os_env}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[app_cv_target_os_env] not set.])
	fi


	case "$app_cv_target_os_env" in
		gnu)
			# glibc all-extensions macro: _GNU_SOURCE.
			# gcc automatically defines it in later versions.
			# suncc needs the flag explicitly.
			# http://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
			app_cv_target_extension_cflags="-D_GNU_SOURCE"
			app_cv_target_extension_cxxflags="-D_GNU_SOURCE"
			;;

		solaris)
			# The problem with solaris is that it has too many incompatible
			# feature test macros, and there is no enable-all macro. Autoconf
			# assumes that __EXTENSIONS__ is the one, but it should be defined
			# _in addition_ to feature test macros. Note that on its own, __EXTENSIONS__
			# still enables some useful stuff like _LARGEFILE64_SOURCE.
			# To support some posix macros, additional link objects are also required.
			# Some *_SOURCE feature test macros may be incompatible with C++.
			# http://docs.sun.com/app/docs/doc/816-5175/standards-5?a=view
			app_cv_target_extension_cflags="-D__EXTENSIONS__"
			app_cv_target_extension_cxxflags="-D__EXTENSIONS__"
			;;

		netbsd)
			# There is no enable-all macro.
			# (_XOPEN_SOURCE >= 500 || _NETBSD_SOURCE) is recommended.
			# The default is _NETBSD_SOURCE, _XOPEN_SOURCE overrides it.
			# The manual says that rename() will behave posix-like if _POSIX_C_SOURCE
			# is defined. However, stdio.h uses (_XOPEN_SOURCE <= 2) condition,
			# which is really bogus.
			# See sys/featuretest.h
			;;

		mingw*)
			# This enables standards-compliant stdio behaviour (regarding printf and
			# friends), as opposed to msvc-compatible one. This is usually enabled
			# by default if one of the usual macros are encountered (_XOPEN_SOURCE,
			# _GNU_SOURCE, etc...).
			# Also, we make the win2k API available (includes console functions, etc...).
			# Other things to consider: _NO_OLDNAMES (disable "deprecated"
			# non-underscored names like chdir (use _chdir), etc...).
			# See _mingw.h for details.
			app_cv_target_extension_cflags="-D__USE_MINGW_ANSI_STDIO=1 -DWINVER=0x0500"  # Since 2000
			app_cv_target_extension_cxxflags="-D__USE_MINGW_ANSI_STDIO=1 -DWINVER=0x0500"
			;;

		interix)
			# As per AC_USE_SYSTEM_EXTENSIONS.
			app_cv_target_extension_cflags="-D_ALL_SOURCE"
			app_cv_target_extension_cxxflags="-D_ALL_SOURCE"
			;;

	esac


	AC_MSG_RESULT([CFLAGS: $app_cv_target_extension_cflags;  CXXFLAGS: $app_cv_target_extension_cxxflags;  LIBS: $app_cv_target_extension_libs])

	if test "x$$1[]CFLAGS" = "x"; then
		$1[]CFLAGS="$app_cv_target_extension_cflags";
	else
		$1[]CFLAGS="$$1[]CFLAGS $app_cv_target_extension_cflags";
	fi
	if test "x$$1[]CXXFLAGS" = "x"; then
		$1[]CXXFLAGS="$app_cv_target_extension_cxxflags";
	else
		$1[]CXXFLAGS="$$1[]CXXFLAGS $app_cv_target_extension_cxxflags";
	fi
	if test "x$$1[]LIBS" = "x"; then
		$1[]LIBS="$app_cv_target_extension_libs";
	else
		$1[]LIBS="$$1[]LIBS $app_cv_target_extension_libs";
	fi

])





