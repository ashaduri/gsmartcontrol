
############################################################################
# Copyright:
#      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# APP_COMPILER_OPTIONS([flags_prefix])
# This macro enables various compiler flags (common warnings, optimization,
# debug, etc...), controllable through configure options.
# flags_prefixCFLAGS, flags_prefixCXXFLAGS and flags_prefixLDFLAGS
# will be initialized (or appended to) with the results.

# DEBUG and DEBUG_BUILD will be exported to config.h in case of debug builds.

# You must call AX_COMPILER_VENDOR, APP_DETECT_OS_KERNEL([target], ...)
# and APP_DETECT_OS_ENV([target], ...) before using this macro.

AC_DEFUN([APP_COMPILER_OPTIONS], [
	# These requirements check compile-time presence, not that they were called
	AC_REQUIRE([AX_COMPILER_VENDOR])
	AC_REQUIRE([APP_DETECT_OS_ENV])

	app_cv_compiler_options_cflags="";
	app_cv_compiler_options_cxxflags="";
	app_cv_compiler_options_ldflags="";


	# ---- Common compiler options (warnings, etc...)

	AC_ARG_ENABLE(common-options, AS_HELP_STRING([--enable-common-options=<compiler>|auto|none],
			[enable useful compilation options (warnings, mostly). Accepted values are auto, none, gnu, clang, intel, sun. (Default: auto)]),
		[app_cv_compiler_common_options=${enableval}], [app_cv_compiler_common_options=none])

	# the default value is "yes" (if no value given after =), treat it as auto.
	if test "x$app_cv_compiler_common_options" = "xyes" || test "x$app_cv_compiler_common_options" = "xauto"; then
		app_cv_compiler_common_options="$ax_cv_cxx_compiler_vendor";
	fi

	if test "x$app_cv_compiler_common_options" = "xno"; then
		# for pretty message
		app_cv_compiler_common_options="none";
	fi
	AC_MSG_NOTICE([Enable common compiler flags for: $app_cv_compiler_common_options])


	if test "x$app_cv_compiler_common_options" != "xnone"; then

		# gcc (or clang) / gnu environment, mingw, cygwin.
		if test "x$app_cv_compiler_common_options" = "xgnu" ||  test "x$app_cv_compiler_common_options" = "xclang"; then

			if test "x$app_cv_target_os_env" = "xmingw32" || test "x$app_cv_target_os_env" = "xmingw64" \
					|| test "x$app_cv_target_os_env" = "xcygwin"; then
				# mingw gcc options:
				# -mno-cygwin - generate non-cygwin executables in cygwin's mingw.
				# -mms-bitfields - make structures compatible with msvc. recommended default for non-cygwin. Default for mingw.
				app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -mms-bitfields"
				app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -mms-bitfields"
				app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -mms-bitfields"
			fi

			app_cv_compiler_tmp_var="-Wall -Wcast-align -Wcast-qual -Wconversion \
-Wfloat-equal -Wnon-virtual-dtor -Woverloaded-virtual \
-Wpointer-arith -Wshadow -Wsign-compare -Wsign-promo -Wundef -Wwrite-strings";
			if test "x$app_cv_compiler_common_options" = "xclang"; then
				# these are really annoying
				app_cv_compiler_tmp_var="$app_cv_compiler_tmp_var -Wno-sign-conversion -Wno-format-security"
			fi
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags $app_cv_compiler_tmp_var";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags $app_cv_compiler_tmp_var";

		# intel / gnu environment
		elif test "x$app_cv_compiler_common_options" = "xintel" && test "x$app_cv_target_os_env" = "xgnu"; then
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -w1";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags \
-ftrapuv -fstack-security-check -early-template-check -static-libgcc -static-intel \
-Wcomment -Wdeprecated -Wextra-tokens -Wformat -Wmissing-prototypes -Wnon-virtual-dtor \
-Wpointer-arith -Wreturn-type -Wshadow -Wtrigraphs -Wuninitialized -w1";
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -static-libgcc -static-intel";

		# suncc
		elif test "x$app_cv_compiler_common_options" = "xsun"; then
			# Enable useful language extensions (e.g. __func__ in C++). Details at
			# http://docs.sun.com/app/docs/doc/820-7599/bkapy?a=view
			# Additional options: "-staticlib=Crun -staticlib=Cstd" - link statically to
			# C++ runtime and standard libraries.
			# http://docs.sun.com/app/docs/doc/820-7599/bkaws?a=view

			# STLport seems to be more standards-compliant, but incompatible
			# with default Cstd. Also, it's not guaranteed to keep compatibility
			# between versions. Select it with "-library=stlport4".
			# http://docs.sun.com/app/docs/doc/820-7599/bkakg?a=view
			# http://developers.sun.com/solaris/articles/cmp_stlport_libCstd.html
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags \
+w -errtags -erroff=notemsource,notused -features=extensions";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags \
+w -errtags -erroff=notemsource,notused -features=extensions";

		fi

	fi



	# ---- Compiler options for debug builds

	AC_ARG_ENABLE(debug-options, AS_HELP_STRING([--enable-debug-options=<compiler>|auto|none],
			[enable debug build options. Accepted values are auto, none, gnu, clang, intel, sun. (Default: none)]),
		[app_cv_compiler_debug_options=${enableval}], [app_cv_compiler_debug_options=none])

	# the default value is "yes" (if no value given after =), treat it as auto.
	if test "x$app_cv_compiler_debug_options" = "xyes" || test "x$app_cv_compiler_debug_options" = "xauto"; then
		app_cv_compiler_debug_options="$ax_cv_cxx_compiler_vendor";
	fi

	if test "x$app_cv_compiler_debug_options" = "xno"; then
		# for pretty message
		app_cv_compiler_debug_options="none";
	fi
	AC_MSG_NOTICE([Enable debug build flags for: $app_cv_compiler_debug_options])


	if test "x$app_cv_compiler_debug_options" != "xnone"; then

		# Define DEBUG and DEBUG_BUILD for debug builds (through config.h).
		AC_DEFINE([DEBUG], [1], [Defined for debug builds])
		AC_DEFINE([DEBUG_BUILD], [1], [Defined for debug builds])

		# gcc, mingw
		if test "x$app_cv_compiler_debug_options" = "xgnu" || test "x$app_cv_compiler_debug_options" = "xclang"; then
			# We could put libstdc++ debug options here, but it generates binary-incompatible
			# C++ code, which leads to runtime errors with e.g. libsigc++.
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -g3 -O0";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -g3 -O0";
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -g3 -O0";

		# intel / gnu
		elif test "x$app_cv_compiler_debug_options" = "xintel" && test "x$app_cv_target_os_env" = "xgnu"; then
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -g3 -O0";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -g3 -O0";
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -g3 -O0";

		# suncc
		elif test "x$app_cv_compiler_debug_options" = "xsun"; then
			# There's no -O0, it's the default. Debug format is stabs by default, but can be switched
			# to dwarf by -xdebugformat=dwarf.
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -xs -g";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -xs -g";
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -xs -g";

		fi

	fi



	# ---- Compiler options for optimized builds

	AC_ARG_ENABLE(optimize-options, AS_HELP_STRING([--enable-optimize-options=<compiler>|auto|none],
			[enable optimized build options. Accepted values are auto, none, gnu, clang, intel, sun. (Default: none)]),
		[app_cv_compiler_optimize_options=${enableval}], [app_cv_compiler_optimize_options=none])

	# the default value is "yes" (if no value given after =), treat it as auto.
	if test "x$app_cv_compiler_optimize_options" = "xyes" || test "x$app_cv_compiler_optimize_options" = "xauto"; then
		app_cv_compiler_optimize_options="$ax_cv_cxx_compiler_vendor";
	fi

	if test "x$app_cv_compiler_optimize_options" = "xno"; then
		# for pretty message
		app_cv_compiler_optimize_options="none";
	fi
	AC_MSG_NOTICE([Enable optimized build flags for: $app_cv_compiler_optimize_options])


	if test "x$app_cv_compiler_optimize_options" != "xnone"; then

		# gcc, mingw
		if test "x$app_cv_compiler_optimize_options" = "xgnu" || test "x$app_cv_compiler_optimize_options" = "xclang"; then
			# -mtune=generic is since gcc 4.0 iirc
			if test "x$app_cv_target_os_env" = "xmingw32"; then
				app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -g0 -O3 -s -march=i686";
				app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -g0 -O3 -s -march=i686";
				app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -g0 -O3 -s -march=i686";
			else  # mingw64, cygwin (they have new gcc), others.
				app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -g0 -O3 -s";
				app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -g0 -O3 -s";
				app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -g0 -O3 -s";
			fi
			# -mtune=generic is only for x86*
			case "$target" in
				i686-*)
					app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -mtune=generic";
					app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -mtune=generic";
					app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -mtune=generic";
					;;
				x86_64-*)
					app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -mtune=generic";
					app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -mtune=generic";
					app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -mtune=generic";
					;;
			esac

		# intel / gnu
		elif test "x$app_cv_compiler_optimize_options" = "xintel" && test "x$app_cv_target_os_env" = "xgnu"; then
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -g0 -O3 -s";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -g0 -O3 -s";
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -g0 -O3 -s";

		# suncc
		elif test "x$app_cv_compiler_optimize_options" = "xsun"; then
			# There's no -O0, it's the default. Debug format is stabs by default, but can be switched
			# to dwarf by -xdebugformat=dwarf.
			app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -xO5";
			app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -xO5";
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -s";

		fi

	fi



	# ---- Compiler options for GCC pch support

	AC_ARG_ENABLE(gcc-pch,
		AS_HELP_STRING([--enable-gcc-pch],[enable precompiled header support (pch make target) (GCC only) (default: disabled)
				(use "make gch" to speed up the compilation, but don't enable it if you don't intend to use it - it will slow things down)]),
		[app_cv_compiler_gcc_pch=${enableval}], [app_cv_compiler_gcc_pch=no])

	AC_MSG_NOTICE([Enable GCC precompiled header usage: $app_cv_compiler_gcc_pch])

	# Don't check the vendor here - it's disabled by default, so it's on the user.
	if test "x$app_cv_compiler_gcc_pch" = "xyes"; then
		app_cv_compiler_options_cflags="$app_cv_compiler_options_cflags -Winvalid-pch -DENABLE_PCH"
		app_cv_compiler_options_cxxflags="$app_cv_compiler_options_cxxflags -Winvalid-pch -DENABLE_PCH"
	fi



	# ---- Compiler options for Mingw's mwindows/mconsole

	# -mwindows - hide console window. possibly suppresses any std output / error.
	# -mconsole - opposite of -mwindows, default.

	AC_ARG_ENABLE(windows-console, AS_HELP_STRING([--enable-windows-console=yes|no|auto],
			[enable windows console (MinGW and Cygwin only). Accepted values are yes, no, auto. ]
			[Auto means disabled for optimized builds, enabled for all others. (Default: auto)]),
		[app_cv_compiler_windows_console=${enableval}], [app_cv_compiler_windows_console=auto])

	if test "x$app_cv_compiler_windows_console" = "xauto"; then
		# disable console for optimized builds.
		if test "x$app_cv_compiler_optimize_options" != "xnone"; then
			app_cv_compiler_windows_console="no";
		else
			app_cv_compiler_windows_console="yes";
		fi
	fi

	if test "x$app_cv_target_os_env" = "xmingw32" || test "x$app_cv_target_os_env" = "xmingw64" \
			|| test "x$app_cv_target_os_env" = "xcygwin"; then
		if test "x$app_cv_compiler_windows_console" = "xno"; then
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -mwindows";
		else
			# specify -mconsole even if it's the default - it should override the user-supplied setting
			app_cv_compiler_options_ldflags="$app_cv_compiler_options_ldflags -mconsole";
		fi
		AC_MSG_NOTICE([Enable windows console: $app_cv_compiler_windows_console])
	fi



	# ---- Export our variables

	if test "x$$1[]CFLAGS" = "x"; then
		$1[]CFLAGS="$app_cv_compiler_options_cflags";
	else
		$1[]CFLAGS="$$1[]CFLAGS $app_cv_compiler_options_cflags";
	fi
	if test "x$$1[]CXXFLAGS" = "x"; then
		$1[]CXXFLAGS="$app_cv_compiler_options_cxxflags";
	else
		$1[]CXXFLAGS="$$1[]CXXFLAGS $app_cv_compiler_options_cxxflags";
	fi
	if test "x$$1[]LDFLAGS" = "x"; then
		$1[]LDFLAGS="$app_cv_compiler_options_ldflags";
	else
		$1[]LDFLAGS="$$1[]LDFLAGS $app_cv_compiler_options_ldflags";
	fi


	AC_MSG_NOTICE([Compiler-specific build options:])
	AC_MSG_NOTICE([CFLAGS: $app_cv_compiler_options_cflags])
	AC_MSG_NOTICE([CXXFLAGS: $app_cv_compiler_options_cxxflags])
	AC_MSG_NOTICE([LDFLAGS: $app_cv_compiler_options_ldflags])

])





