
AC_INIT([GSmartControl],[2.0.0],[],[gsmartcontrol])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 -Wall])


# Clear *FLAGS, LIBS, LDFLAGS if requested through configure option.
APP_AUTO_CLEAR_FLAGS()

# C compiler, C preprocessor
#AC_PROG_CC

# C++ compiler
AC_PROG_CXX

#AM_PROG_LIBTOOL
AM_PROG_AR

# C++ support in checks
AC_LANG([C++])

# Needed for generating static libs
AC_CHECK_TOOL(RANLIB, ranlib)

# Win32 resource file compilation
AC_CHECK_TOOL(WINDRES, windres)

# Win32 external dll stripping (already detected by autoconf)
# AC_CHECK_TOOL(STRIP, strip)

# Defines LN_S, needed for man page installation.
AC_PROG_LN_S


# -------------------------------------------------------------------------------------


# ------------- Detect compiler, OS, environment. Enable system features.


# Enable C++17 support.
# This has to be AFTER gtkmm, since gtkmm has its own C++ flags.
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])


# detect compiler vendor (sets $ax_cv_cxx_compiler_vendor)
AX_COMPILER_VENDOR

# Detect target OS kernel and userspace and export to config.h
# (also define automake conditionals).
APP_DETECT_OS_KERNEL([target], [CONFIG_KERNEL_])
APP_DETECT_OS_ENV([target], [CONFIG_OS_ENV_])
APP_DETECT_OS_KERNEL([build], [CONFIG_HOST_KERNEL_])

# Detect common (warning, debug, optimize) compiler flags and
# export them to C(XX)FLAGS, LDFLAGS.
# We don't use prefix because this way the flags will be used for examples,
# which is important for some compatibility flags (e.g. __func__ visibility, etc...).
APP_COMPILER_OPTIONS([])

# Thread support for known configurations.
# Sun compiler (at least on linux) has some runtime issues with stringstream
# unless we use the thread flags. Also, Linux-based gdb fails to debug
# a gcc-compiled binary for some reason.
# Do this for all systems except mingw.
if test "$app_cv_target_os_env" != "mingw32" && test "$app_cv_target_os_env" != "mingw64"; then
	APP_GET_MT_FLAGS([])
fi

# Enable system extensions. Somewhat follows AC_USE_SYSTEM_EXTENSIONS.
case "$app_cv_target_os_env" in
	gnu)
		# glibc all-extensions macro: _GNU_SOURCE.
		# Defined automatically when used with glibc, but not others.
		CXXFLAGS="$CXXFLAGS -D_GNU_SOURCE"
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
		CXXFLAGS="$CXXFLAGS -D__EXTENSIONS__ -D_POSIX_PTHREAD_SEMANTICS"
		;;
	mingw*)
		# This enables standards-compliant stdio behaviour (regarding printf and
		# friends), as opposed to msvc-compatible one. This is usually enabled
		# by default if one of the usual macros are encountered (_XOPEN_SOURCE,
		# _GNU_SOURCE, etc...).
		# See _mingw.h for details.
		CXXFLAGS="$CXXFLAGS -D__USE_MINGW_ANSI_STDIO=1 -DWINVER=0x0600"  # Since Vista
		;;
	darwin*)
		# Enable large file support.
		CXXFLAGS="$CXXFLAGS -D_DARWIN_USE_64_BIT_INODE=1"
esac

# In many environments this enables large file support
CXXFLAGS="$CXXFLAGS -D_FILE_OFFSET_BITS=64"


# Arrange large file support (define _FILE_OFFSET_BITS 64, etc...).
# Adds the special flags to CC.
AC_SYS_LARGEFILE


# -------------------------------------------------------------------------------------


# Non-standard feature checks

# getrawpartition() for netbsd and openbsd in -lutil.
AC_CHECK_LIB(util, getrawpartition, [ LIBS="$LIBS -lutil" ])


# -------------------------------------------------------------------------------------


# enable / disable compilation of examples
AC_ARG_ENABLE([examples], AS_HELP_STRING([--enable-examples],
		[enable compilation of example programs (default: no)]), [with_examples=${enableval}], [with_examples=no])
# export to automake.am-s
AM_CONDITIONAL([ENABLE_EXAMPLES], [test "$with_examples" = "yes"])


# -------------------------------------------------------------------------------------

pkg_modules="gtkmm-3.0 >= 3.4.0"

# define ENABLE_GLIB and friends if it's found.
gtkmm_found="no"

PKG_CHECK_MODULES([GTKMM], [$pkg_modules],
		[gtkmm_found="yes"], [gtkmm_found="no"])

if test "x$gtkmm_found" = "xno"; then
	AC_MSG_ERROR([$GTKMM_PKG_ERRORS])
fi

CXXFLAGS="$GTKMM_CFLAGS -DENABLE_GLIB=1 -DENABLE_GLIBMM=1 $CXXFLAGS"
LIBS="$GTKMM_LIBS $LIBS"


# -------------------------------------------------------------------------------------


PCRECPP_CFLAGS="";
PCRECPP_LIBS="";

# try pkg-config first
PKG_CHECK_MODULES(PCRECPP, [libpcrecpp], [pcrecpp_found=yes], [pcrecpp_found=no])

# if not found, try pcre-config (solaris had it but not pkg-config).
if test "x$pcrecpp_found" = "xno"; then
	AC_PATH_PROG(pcre_config_binary, [pcre-config])
	if test "x$pcre_config_binary" != "x"; then
		PCRE_CFLAGS="`pcre-config --cflags`"
		PCRE_LIBS="`pcre-config --libs-cpp`"
	else
		# AC_MSG_ERROR([Could not find pcrecpp through pkg-config and pcre-config. Make sure the pcre and pcrecpp libraries are installed])
		AC_MSG_ERROR([$PCRECPP_PKG_ERRORS])
	fi
fi

CXXFLAGS="$CXXFLAGS $PCRECPP_CFLAGS"
LIBS="$LIBS $PCRECPP_LIBS"


# -------------------------------------------------------------------------------------


# gtkmm may have an older -std standard in its CXXFLAGS which screws up
# our -std flag in CXX. Override it.
# Ideally, gcc should use the "latest" standard, but it just uses the latest option.
CXXFLAGS="$CXXFLAGS -std=c++17"

# This is required for experimental filesystem support in gcc7. Not sure about the final filesystem in gcc8.
LIBS="$LIBS -lstdc++fs"

# This is required for experimental filesystem support in clang/llvm's libc++.
# LIBS="$LIBS -lc++experimental"


# -------------------------------------------------------------------------------------


# Win32 distribution support (NSIS, zip)

AC_ARG_WITH([nsis], [AS_HELP_STRING([--with-nsis=<location>|auto],
		[specify makensis.exe location to support making NSIS packages]
		[ (default: "C:\Program Files\NSIS\makensis.exe" for windows, ]
		["~/.wine/drive_c/Program Files/NSIS/Unicode/makensis.exe" for others)])],
		[with_nsis=${withval}], [with_nsis=auto])

AC_ARG_ENABLE([nsis-wine], [AS_HELP_STRING([--enable-nsis-wine=yes|no],
		[use wine to run makensis.exe when making NSIS packages (default: no)])],
		[with_nsis_wine=${enableval}], [with_nsis_wine=no])

AC_ARG_WITH([windows-sysroot], [AS_HELP_STRING([--with-windows-sysroot],
		[specify location of Windows system root directory when making Windows packages (default: /mingw32)])],
		[with_windows_sysroot=${withval}], [with_windows_sysroot=auto])

if test "x$with_nsis" = "xauto" || test "x$with_nsis" = "x"; then
	# if building on windows, use native paths
	if test "x$app_cv_build_os_kernel" = "xwindows32" || test "x$app_cv_build_os_kernel" = "xwindows64"; then
		with_nsis="\"C:\\Program Files\\NSIS\\makensis.exe\"";
	else
		with_nsis="~/\".wine/drive_c/Program Files/NSIS/Unicode/makensis.exe\"";
	fi
fi

NSIS_EXEC="$with_nsis";
if test "x$with_nsis_wine" = "xyes"; then
	NSIS_EXEC="wine $with_nsis";
fi

if test "x$with_windows_sysroot" = "xauto" || test "x$with_windows_sysroot" = "x"; then
	with_windows_sysroot="/mingw32"
fi
WINDOWS_SYSROOT="$with_windows_sysroot";

WINDOWS_SUFFIX="";
if test "x$app_cv_target_os_kernel" = "xwindows32"; then
	WINDOWS_SUFFIX="win32"
elif test "x$app_cv_target_os_kernel" = "xwindows64"; then
	WINDOWS_SUFFIX="win64"
fi

AC_SUBST(NSIS_EXEC)
AC_SUBST(WINDOWS_SYSROOT)
AC_SUBST(WINDOWS_SUFFIX)


# -------------------------------------------------------------------------------------


# Win32 security manifest support

WINDOWS_ARCH=""
if test "x$app_cv_target_os_kernel" = "xwindows32"; then
	WINDOWS_ARCH="x86"
elif test "x$app_cv_target_os_kernel" = "xwindows64"; then
	WINDOWS_ARCH="amd64"
fi

AC_SUBST(WINDOWS_ARCH)


# -------------------------------------------------------------------------------------


# Export some configure variables as defines for the source code.
# Note: Autoconf manual says that AC_DEFINE won't work here.

ADDITIONAL_FLAGS="-DPACKAGE_PKGDATA_DIR=\"\\\"\$(pkgdatadir)\\\"\" \
-DPACKAGE_SYSCONF_DIR=\"\\\"\$(sysconfdir)\\\"\" \
-DPACKAGE_DOC_DIR=\"\\\"\$(docdir)\\\"\" \
-DTOP_SOURCE_DIR=\"\\\"\$(top_srcdir)\\\"\" \
-DHZ_USE_LIBDEBUG=1

CXXFLAGS="$CXXFLAGS $ADDITIONAL_FLAGS"


# Top src dir should always be in include path, before other includes.
# Add global configuration file. Note: It must be searched in top_builddir
# first (that's why -I is the first flag), for precompiled headers to work.

ADDITIONAL_INCLUDES="-I\$(top_builddir) -I\$(top_srcdir)/src"

# -include works with gcc, intel, pathscale and sunstudio since 12u1, but doesn't work
# with pgi and older sunstudio (the one in solaris10, for example).
# Note: some gcc-frontend-based compilers may have to be added here
# (e.g. gccfss (gcc frontend / sun backend), etc...).
# We use -include only when using pch.
# if test "x$app_cv_compiler_gcc_pch" = "xyes"; then
# 	if test "$ax_cv_cxx_compiler_vendor" = "gnu" || test "$ax_cv_cxx_compiler_vendor" = "clang" \
# 			|| test "$ax_cv_cxx_compiler_vendor" = "intel" || test "$ax_cv_cxx_compiler_vendor" = "pathscale"; then
# 		# This auto-includes this file when compiling any file. Benefits include pch support.
# 		ADDITIONAL_INCLUDES="$ADDITIONAL_INCLUDES -include global_macros.h"
# 	fi
# fi

# our flags must be before all the others
CXXFLAGS="$ADDITIONAL_INCLUDES $CXXFLAGS"


# -------------------------------------------------------------------------------------


# make them available in Makefile.am
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)


# print them out for debug purposes
AC_MSG_NOTICE([
CXX:
$CXX
CXXFLAGS:
$CXXFLAGS
LDFLAGS:
$LDFLAGS
LIBS:
$LIBS
])


# substitude autotools' predefined dir names in makefile.am-s
AC_SUBST(AUTODIRS)


# -------------------------------------------------------------------------------------


AC_CONFIG_FILES([ data/gsmartcontrol.desktop data/gsmartcontrol.appdata.xml \
	data/nsis/distribution.txt data/nsis/gsmartcontrol.nsi \
	debian.dist/changelog \
	src/gsc_winres.rc src/gsmartcontrol.exe.manifest \
	gsmartcontrol.spec version compilation_flags ])


AC_CONFIG_FILES([data/gsmartcontrol-root], [chmod +x data/gsmartcontrol-root])


# these are all the makefiles to generate

AC_CONFIG_FILES([Makefile src/Makefile src/applib/Makefile src/res/Makefile src/hz/Makefile \
	src/libdebug/Makefile src/rconfig/Makefile src/json/Makefile \
	data/Makefile data/16/Makefile data/22/Makefile data/24/Makefile data/32/Makefile \
	data/48/Makefile data/64/Makefile data/128/Makefile data/256/Makefile data/nsis/Makefile \
	debian.dist/Makefile])


AC_OUTPUT


