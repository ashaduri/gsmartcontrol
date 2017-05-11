
############################################################################
# Copyright:
#      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# APP_DETECT_OS_KERNEL(triplet_name, define_macro_prefix)
# Check target OS kernel.
# triplet_name is target, host or build.
# You must call AX_COMPILER_VENDOR before using this macro.

# If you call
# APP_DETECT_OS_KERNEL([target], [CONFIG_KERNEL])
# you will get CONFIG_KERNEL_LINUX C macro exported to config.h
# (on gcc/linux).
# If no define_macro_prefix is specified, no C macro prefix will be used.
# app_cv_target_os_kernel will be set to kernel name (lowercase).
# app_cv_target_os_kernel_macro will be set to kernel name (all caps).
# TARGET_OS_KERNEL_LINUX automake conditional will be set to true.

# Note: Some operating systems / compilers have built-in defines.
# For gcc, check with
# $ gcc -dM -E - < /dev/null

AC_DEFUN([APP_DETECT_OS_KERNEL], [
	# These requirements check compile-time presence, not that they were called
	AC_REQUIRE([AX_COMPILER_VENDOR])

	AC_MSG_CHECKING([for $1 OS kernel])

	app_cv_[]$1[]_os_kernel="";
	app_cv_[]$1[]_os_kernel_macro="";

	# If vendor is not recognized, it's set to "unknown".
	# NOTE: This works only on target compilers!
	if test "x${ax_cv_cxx_compiler_vendor}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[ax_cv_cxx_compiler_vendor] not set.])
	fi

	# "build" - the type of system on which the package is being configured and compiled.
	# "host" - the type of system on which the package runs. Specifying it enables the
	# cross-compilation mode. "target" defaults to this too.

	if test "x$1" != "xbuild" && test "x$1" != "xhost" && test "x$1" != "xtarget"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[Triplet name must be build, host or target.]])
	fi

	case "$$1" in
		*linux*)
			# gcc defines __linux__ on linux. Other defines include: __linux, __unix__, __gnu_linux__,
			# linux, unix, __i386__, i386.
			app_cv_[]$1[]_os_kernel="linux";
			app_cv_[]$1[]_os_kernel_macro="LINUX";
			;;

		i*86-*mingw* | i*86-*cygwin*)
			# mingw defines _WIN32. msvc defines _WIN32 (built-in), and WIN32 via windows.h.
			# Other mingw defines include: __WINNT, __WINNT__, __WIN32__, __i386, _X86_,
			# i386, __i386__, WIN32, __MINGW32__, WINNT (all equal to 1).
			app_cv_[]$1[]_os_kernel="windows32";
			app_cv_[]$1[]_os_kernel_macro="WINDOWS32";
			;;

		x86_64-*-mingw* | x86_64-*-cygwin*)
			# mingw64 defines the same stuff as 32-bit one, plus _WIN64, __MINGW64__, etc... .
			# Keep in mind that if you're generating a 32-bit application, the kernel will
			# be windows32 even if you run it on 64-bit Windows.
			app_cv_[]$1[]_os_kernel="windows64";
			app_cv_[]$1[]_os_kernel_macro="WINDOWS64";
			;;

		*interix*)
			# Interix is a kernel on top of windows, so we treat it separately.
			app_cv_[]$1[]_os_kernel="interix";
			app_cv_[]$1[]_os_kernel_macro="INTERIX";
			;;

		# This includes debian gnu/kfreebsd. Dragonfly is checked separately.
		*freebsd*)
			# gcc built-in defines (to check for freebsd kernel):
			# defined(__FreeBSD__) || defined(__FreeBSD_kernel__).
			# See http://glibc-bsd.alioth.debian.org/porting/PORTING .
			app_cv_[]$1[]_os_kernel="freebsd";
			app_cv_[]$1[]_os_kernel_macro="FREEBSD";
			;;

		*dragonfly*)
			app_cv_[]$1[]_os_kernel="dragonfly";
			app_cv_[]$1[]_os_kernel_macro="DRAGONFLY";
			;;

		*openbsd*)
			app_cv_[]$1[]_os_kernel="openbsd";
			app_cv_[]$1[]_os_kernel_macro="OPENBSD";
			;;

		# This includes debian gnu/netbsd.
		*netbsd*)
			app_cv_[]$1[]_os_kernel="netbsd";
			app_cv_[]$1[]_os_kernel_macro="NETBSD";
			;;

		*solaris*)
			app_cv_[]$1[]_os_kernel="solaris";
			app_cv_[]$1[]_os_kernel_macro="SOLARIS";
			;;

		*darwin*)
			app_cv_[]$1[]_os_kernel="darwin";
			app_cv_[]$1[]_os_kernel_macro="DARWIN";
			;;

		*qnx*)
			app_cv_[]$1[]_os_kernel="qnx";
			app_cv_[]$1[]_os_kernel_macro="QNX";
			;;

		*)
			app_cv_[]$1[]_os_kernel="unknown";
			app_cv_[]$1[]_os_kernel_macro="UNKNOWN";
			;;
	esac

	# Assign this - we may use this in configure itself.
	if test "x$2" != "x"; then
		app_cv_[]$1[]_os_kernel_macro="$2[]_${app_cv_[]$1[]_os_kernel_macro}";
	fi


	# config.h macros (yes, we need this many. otherwise config.h will be incorrect).

	if test "x${app_cv_[]$1[]_os_kernel}" = "xlinux"; then
		AC_DEFINE($2[]LINUX, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xwindows32"; then
		AC_DEFINE($2[]WINDOWS32, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xwindows64"; then
		AC_DEFINE($2[]WINDOWS64, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xinterix"; then
		AC_DEFINE($2[]INTERIX, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xfreebsd"; then
		AC_DEFINE($2[]FREEBSD, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xdragonfly"; then
		AC_DEFINE($2[]DRAGONFLY, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xopenbsd"; then
		AC_DEFINE($2[]OPENBSD, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xnetbsd"; then
		AC_DEFINE($2[]NETBSD, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xsolaris"; then
		AC_DEFINE($2[]SOLARIS, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xdarwin"; then
		AC_DEFINE($2[]DARWIN, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xqnx"; then
		AC_DEFINE($2[]QNX, 1, [$1 OS kernel])
	fi
	if test "x${app_cv_[]$1[]_os_kernel}" = "xunknown"; then
		AC_DEFINE($2[]UNKNOWN, 1, [$1 OS kernel])
	fi

	# Export to Makefile.am's.
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_LINUX, [test "x${app_cv_[]$1[]_os_kernel}" = "xlinux"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_WINDOWS32, [test "x${app_cv_[]$1[]_os_kernel}" = "xwindows32"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_WINDOWS64, [test "x${app_cv_[]$1[]_os_kernel}" = "xwindows64"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_INTERIX, [test "x${app_cv_[]$1[]_os_kernel}" = "xinterix"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_FREEBSD, [test "x${app_cv_[]$1[]_os_kernel}" = "xfreebsd"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_DRAGONFLY, [test "x${app_cv_[]$1[]_os_kernel}" = "xdragonfly"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_OPENBSD, [test "x${app_cv_[]$1[]_os_kernel}" = "xopenbsd"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_NETBSD, [test "x${app_cv_[]$1[]_os_kernel}" = "xnetbsd"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_SOLARIS, [test "x${app_cv_[]$1[]_os_kernel}" = "xsolaris"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_DARWIN, [test "x${app_cv_[]$1[]_os_kernel}" = "xdarwin"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_QNX, [test "x${app_cv_[]$1[]_os_kernel}" = "xqnx"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_UNKNOWN, [test "x${app_cv_[]$1[]_os_kernel}" = "xunknown"])


	# composite ones, for easy checking

	AM_CONDITIONAL(m4_toupper($1)[]_OS_KERNEL_FAMILY_WINDOWS, \
			[test "x${app_cv_[]$1[]_os_kernel}" = "xwindows32" || test "x${app_cv_[]$1[]_os_kernel}" = "xwindows64" \
				|| test "x${app_cv_[]$1[]_os_kernel}" = "xinterix"])

	if test "x${app_cv_[]$1[]_os_kernel}" = "xwindows32" || test "x${app_cv_[]$1[]_os_kernel}" = "xwindows64" \
			|| test "x${app_cv_[]$1[]_os_kernel}" = "xinterix"; then
		AC_DEFINE($2[]FAMILY_WINDOWS, 1, [$1 OS kernel family])
	fi


	AC_MSG_RESULT([${app_cv_[]$1[]_os_kernel}])
])




# APP_DETECT_OS_ENV(triplet_name, define_macro_prefix)
# Check target OS userland environment (mainly glibc).
# triplet_name is target, host or build.
# You must call AX_COMPILER_VENDOR before using this macro.

# If you call
# APP_DETECT_OS_ENV([target], [COMPILER], [CONFIG_OS_ENV])
# you will get CONFIG_OS_ENV_LINUX C macro exported to config.h
# (on gcc/linux).
# If no define_macro_prefix is specified, no C macro prefix will be used.
# app_cv_target_os_env will be set to env name (lowercase).
# app_cv_target_os_env_macro will be set to env name (all caps).
# TARGET_OS_ENV_LINUX automake conditional will be set to true.

AC_DEFUN([APP_DETECT_OS_ENV], [
	# These requirements check compile-time presence, not that they were called
	AC_REQUIRE([AX_COMPILER_VENDOR])

	AC_MSG_CHECKING([for $1 OS userland environment])

	app_cv_[]$1[]_os_env="";
	app_cv_[]$1[]_os_env_macro="";

	# If vendor is not recognized, it's set to "unknown".
	# NOTE: This works only on target compilers!
	if test "x${ax_cv_cxx_compiler_vendor}" = "x"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[ax_cv_cxx_compiler_vendor] not set.])
	fi

	# "build" - the type of system on which the package is being configured and compiled.
	# "host" - the type of system on which the package runs. Specifying it enables the
	# cross-compilation mode. "target" defaults to this too.

	if test "x$1" != "xbuild" && test "x$1" != "xhost" && test "x$1" != "xtarget"; then
		# msg_result is needed to print the checking... message.
		AC_MSG_RESULT([error])
		AC_MSG_ERROR([[Triplet name must be build, host or target.]])
	fi

	case "$$1" in
		*cygwin*)
			app_cv_[]$1[]_os_env="cygwin";
			app_cv_[]$1[]_os_env_macro="CYGWIN";
			;;
		i*86-*mingw*)
			app_cv_[]$1[]_os_env="mingw32";
			app_cv_[]$1[]_os_env_macro="MINGW32";
			;;
		x86_64-*-mingw*)
			app_cv_[]$1[]_os_env="mingw64";
			app_cv_[]$1[]_os_env_macro="MINGW64";
			;;
		*interix*)
			app_cv_[]$1[]_os_env="interix";
			app_cv_[]$1[]_os_env_macro="INTERIX";
			;;
		# This includes gnu/linux, debian gnu/kfreebsd, debian gnu/netbsd...
		*-gnu | gnu*)
			app_cv_[]$1[]_os_env="gnu";
			app_cv_[]$1[]_os_env_macro="GNU";
			;;
		# Freebsd libc
		*freebsd*)
			app_cv_[]$1[]_os_env="freebsd";
			app_cv_[]$1[]_os_env_macro="FREEBSD";
			;;
		*dragonfly*)
			app_cv_[]$1[]_os_env="dragonfly";
			app_cv_[]$1[]_os_env_macro="DRAGONFLY";
			;;
		*openbsd*)
			app_cv_[]$1[]_os_env="openbsd";
			app_cv_[]$1[]_os_env_macro="OPENBSD";
			;;
		*netbsd*)
			app_cv_[]$1[]_os_env="netbsd";
			app_cv_[]$1[]_os_env_macro="NETBSD";
			;;
		*solaris*)
			app_cv_[]$1[]_os_env="solaris";
			app_cv_[]$1[]_os_env_macro="SOLARIS";
			;;
		*darwin*)
			app_cv_[]$1[]_os_env="darwin";
			app_cv_[]$1[]_os_env_macro="DARWIN";
			;;
		*qnx*)
			app_cv_[]$1[]_os_env="qnx";
			app_cv_[]$1[]_os_env_macro="QNX";
			;;
		*)
			app_cv_[]$1[]_os_env="unknown";
			app_cv_[]$1[]_os_env_macro="UNKNOWN";
			;;
	esac

	# Assign this even if not using flags - we may use this in configure itself.
	if test "x$2" != "x"; then
		app_cv_[]$1[]_os_env_macro="$2[]_${app_cv_[]$1[]_os_env_macro}";
	fi


	# config.h macros (yes, we need this many. otherwise config.h will be incorrect).

	if test "x${app_cv_[]$1[]_os_env}" = "xcygwin"; then
		AC_DEFINE($2[]CYGWIN, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xmingw32"; then
		AC_DEFINE($2[]MINGW32, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xmingw64"; then
		AC_DEFINE($2[]MINGW64, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xinterix"; then
		AC_DEFINE($2[]INTERIX, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xgnu"; then
		AC_DEFINE($2[]GNU, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xfreebsd"; then
		AC_DEFINE($2[]FREEBSD, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xdragonfly"; then
		AC_DEFINE($2[]DRAGONFLY, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xopenbsd"; then
		AC_DEFINE($2[]OPENBSD, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xnetbsd"; then
		AC_DEFINE($2[]NETBSD, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xsolaris"; then
		AC_DEFINE($2[]SOLARIS, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xdarwin"; then
		AC_DEFINE($2[]DARWIN, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xqnx"; then
		AC_DEFINE($2[]QNX, 1, [$1 OS userspace environment])
	fi
	if test "x${app_cv_[]$1[]_os_env}" = "xunknown"; then
		AC_DEFINE($2[]UNKNOWN, 1, [$1 OS userspace environment])
	fi

	# Export to Makefile.am's.
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_CYGWIN, [test "x${app_cv_[]$1[]_os_env}" = "xcygwin"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_MINGW32, [test "x${app_cv_[]$1[]_os_env}" = "xmingw32"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_MINGW64, [test "x${app_cv_[]$1[]_os_env}" = "xmingw64"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_INTERIX, [test "x${app_cv_[]$1[]_os_env}" = "xinterix"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_GNU, [test "x${app_cv_[]$1[]_os_env}" = "xgnu"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_FREEBSD, [test "x${app_cv_[]$1[]_os_env}" = "xfreebsd"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_DRAGONFLY, [test "x${app_cv_[]$1[]_os_env}" = "xdragonfly"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_OPENBSD, [test "x${app_cv_[]$1[]_os_env}" = "xopenbsd"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_NETBSD, [test "x${app_cv_[]$1[]_os_env}" = "xnetbsd"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_SOLARIS, [test "x${app_cv_[]$1[]_os_env}" = "xsolaris"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_DARWIN, [test "x${app_cv_[]$1[]_os_env}" = "xdarwin"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_QNX, [test "x${app_cv_[]$1[]_os_env}" = "xqnx"])
	AM_CONDITIONAL(m4_toupper($1)[]_OS_ENV_UNKNOWN, [test "x${app_cv_[]$1[]_os_env}" = "xunknown"])


	AC_MSG_RESULT([${app_cv_[]$1[]_os_env}])
])




