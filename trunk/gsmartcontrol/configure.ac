
AC_INIT([GSmartControl], 0.8.4, [], [gsmartcontrol])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 -Wall])


# Clear *FLAGS, LIBS, LDFLAGS if requested through configure option.
APP_AUTO_CLEAR_FLAGS()

# C compiler, C preprocessor
AC_PROG_CC
#AM_PROG_CC_STDC
#AC_PROG_CPP

# C++ compiler
AC_PROG_CXX
#AC_PROG_CXXCPP

#AM_PROG_LIBTOOL

# C++ support in checks
#AC_LANG([C])
AC_LANG([C++])

# Needed for generating static libs
AC_CHECK_TOOL(RANLIB, ranlib)

# Win32 resource file compilation
AC_CHECK_TOOL(WINDRES, windres)

# Win32 external dll stripping (already detected by autoconf)
# AC_CHECK_TOOL(STRIP, strip)


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# ------------- Detect compiler, OS, environment. Enable system features.


# detect compiler vendor (sets $ax_cv_cxx_compiler_vendor)
AX_COMPILER_VENDOR

# Detect target OS kernel and userspace and export to config.h
# (also define automake conditionals).
APP_DETECT_OS_KERNEL([target], [CONFIG_KERNEL_])
APP_DETECT_OS_ENV([target], [CONFIG_OS_ENV_])
APP_DETECT_OS_KERNEL([build], [CONFIG_HOST_KERNEL_])

# Detect common (warning, debug, optimize) compiler flags and
# export them to C(XX)FLAGS, LDFLAGS.
# We don't use prefix because this way the flags will be used for tests,
# which is important for some compatibility flags (e.g. __func__ visibility, etc...).
APP_COMPILER_OPTIONS([])

# Thread support for known configurations.
APP_GET_MT_FLAGS([])

# Enable _GNU_SOURCE, etc...
APP_USE_SYSTEM_EXTENSIONS([])

# Arrange large file support (define _FILE_OFFSET_BITS 64, etc...)
AC_SYS_LARGEFILE



# ------------- Correct incomplete standard libraries

# define int8_t and friends if the standard doesn't have them
AC_TYPE_INT8_T
AC_TYPE_UINT8_T
AC_TYPE_INT16_T
AC_TYPE_UINT16_T
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T
AC_TYPE_INTMAX_T
AC_TYPE_UINTMAX_T

AC_TYPE_LONG_LONG_INT
AC_TYPE_UNSIGNED_LONG_LONG_INT



# ------------- Language / Standard Library checks

# detect RTTI (defines HAVE_RTTI)
AC_CXX_RTTI

# detect exception support (defines HAVE_EXCEPTIONS)
AC_CXX_EXCEPTIONS

# detect abi::__cxa_demangle (defines HAVE_GCC_ABI_DEMANGLE)
AX_CXX_GCC_ABI_DEMANGLE

# detect __gnu_cxx::__verbose_terminate_handler (defines HAVE_VERBOSE_TERMINATE_HANDLER.)
AC_CXX_VERBOSE_TERMINATE_HANDLER

# detect __func__ support (defines HAVE_CXX___func__)
APP_CXX___FUNC__

# detect __FUNCTION__ support (defines HAVE_CXX___FUNCTION__)
APP_CXX___FUNCTION__

# detect if the compiler accepts extern "C" function pointer overloads
# (defines HAVE_CXX_EXTERN_C_OVERLOAD)
APP_CXX_EXTERN_C_OVERLOAD


# This may be needed for pch support
AC_CHECK_HEADERS([stdc++.h])

# pcrecpp needs this (break on first function found)
AC_CHECK_FUNCS([strtoll _strtoi64], [break])

# Non-C99 systems don't have strtof and strtold. some have strtodf for strtof,
# but let's ignore it. Note: They may be supported by system libc,
# but not enabled by default (e.g. by glibc feature macros). In that case,
# defining them still leads to undefined symbols at link time, so we don't do it.
# Header check is the best way to determine availability:
AC_CHECK_DECLS([strtof, strtold], [], [], [[#include <stdlib.h>]])
# link check only:
# AC_CHECK_FUNCS([strtof strtold])


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Non-standard feature checks

# getrawpartition() for netbsd and openbsd in -lutil.
AC_CHECK_LIB(util, getrawpartition, [ 	LIBS="$LIBS -lutil" ])


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# enable / disable compilation of tests
AC_ARG_ENABLE([tests], AS_HELP_STRING([--enable-tests],
		[enable compilation of test programs (default: no)]), [with_tests=${enableval}], [with_tests=no])
# export to automake.am-s
AM_CONDITIONAL([ENABLE_TESTS], [test "$with_tests" = "yes"])


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


AC_ARG_ENABLE(abort-if-no-gtkmm, AS_HELP_STRING([--disable-abort-if-no-gtkmm],
		[disable abort if no gtkmm is found (default: configure aborts when gtkmm is not found)]),
	[app_cv_abort_if_no_gtkmm=${enableval}], [app_cv_abort_if_no_gtkmm=yes])

AC_MSG_NOTICE([Abort if no gtkmm is found: $app_cv_abort_if_no_gtkmm])

# iconview is available since 2.6.0. gthread is not enabled by default.
pkg_modules="gtkmm-2.4 >= 2.6.0 gthread-2.0 >= 2.0"

# define ENABLE_GLIB and friends if it's found.
gtkmm_found="no"

PKG_CHECK_MODULES([GTKMM], [$pkg_modules], [
		AC_DEFINE([ENABLE_GLIB], [1], [Whether GLib is installed])
		AC_DEFINE([ENABLE_GLIBMM], [1], [Whether GLibmm is installed])
		gtkmm_found="yes"], [gtkmm_found="no"])

if test "x$gtkmm_found" = "xno"; then
	if test "x$app_cv_abort_if_no_gtkmm" = "xno"; then
		AC_MSG_WARN([$GTKMM_PKG_ERRORS])
		AC_MSG_WARN([The program will be unable to compile fully.])
	else
		AC_MSG_ERROR([$GTKMM_PKG_ERRORS])
	fi
fi

# GTK flags must be prepended, or glib_config.h conflict between
# gtk 2.x and 1.2 may occur.

#CFLAGS="$GTKMM_CFLAGS $CFLAGS"
CXXFLAGS="$GTKMM_CFLAGS $CXXFLAGS"
LIBS="$GTKMM_LIBS $LIBS"


# Detect libglade / gtkbuilder support

APP_CHOOSE_GLD_READER([C++], [LIBGLADE_])

if test "$app_cv_gld_reader" = "libglade"; then
	CFLAGS="$CFLAGS $LIBGLADE_CFLAGS"
	CXXFLAGS="$CXXFLAGS $LIBGLADE_CXXFLAGS"
	LIBS="$LIBS $LIBGLADE_LIBS"
fi


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


PCRE_CFLAGS="";
PCRE_LIBS="";

# try pkg-config first
PKG_CHECK_MODULES(PCRE, [libpcre], [pcre_found=yes], [pcre_found=no])

# if not found, try pcre-config (solaris had it but not pkg-config).
if test "x$pcre_found" = "xno"; then
	AC_PATH_PROG(pcre_config_binary, [pcre-config])
	if test "x$pcre_config_binary" != "x"; then
		PCRE_CFLAGS="`pcre-config --cflags`"
		PCRE_LIBS="`pcre-config --libs`"
	else
		# AC_MSG_ERROR([Could not find pcre through pkg-config and pcre-config. Make sure the pcre libraries are installed])
		AC_MSG_ERROR([$PCRE_PKG_ERRORS])
	fi
fi

CFLAGS="$CFLAGS $PCRE_CFLAGS"
CXXFLAGS="$CXXFLAGS $PCRE_CFLAGS"
LIBS="$LIBS $PCRE_LIBS"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Test stuff here...
# <nothing>

#SDL_VERSION=1.2.0
#AM_PATH_SDL($SDL_VERSION, :,
#	AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]) )
#CFLAGS="$CFLAGS $SDL_CFLAGS"
#CXXFLAGS="$CXXFLAGS $SDL_CFLAGS "
#LIBS="$LIBS $SDL_LIBS"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Win32 distribution support (NSIS, zip)

AC_ARG_WITH([nsis], [AS_HELP_STRING([--with-nsis=<location>|auto],
		[specify makensis.exe location to support making NSIS packages]
		[ (default: "C:\Program Files\NSIS\makensis.exe" for windows, ]
		["~/.wine/drive_c/Program Files/NSIS/makensis.exe" for others)])],
		[with_nsis=${withval}], [with_nsis=auto])

AC_ARG_ENABLE([nsis-wine], [AS_HELP_STRING([--enable-nsis-wine=yes|no|auto],
		[use wine to run makensis.exe when making NSIS packages (default: auto, which is no for windows, yes for others)])],
		[with_nsis_wine=${enableval}], [with_nsis_wine=yes])

AC_ARG_WITH([windows-dlls], [AS_HELP_STRING([--with-windows-dlls],
		[specify location of Windows dlls when making Windows packages (default: /target/bin)])],
		[with_windows_env=${withval}], [win_windows_dlls=auto])

if test "x$with_nsis" = "xauto" || test "x$with_nsis" = "x"; then
	if test "x$app_cv_build_os_kernel" = "xwindows"; then
		with_nsis="~/\".wine/drive_c/Program Files/NSIS/makensis.exe\"";
	else
		with_nsis="\"C:\\Program Files\\NSIS\\makensis.exe\"";
	fi
fi

if test "x$with_nsis_wine" = "xauto"; then
	if test "x$app_cv_build_os_kernel" = "xwindows"; then
		with_nsis_wine="no";
	else
		with_nsis_wine="yes";
	fi
fi

NSIS_EXEC="$with_nsis";
if test "x$with_nsis_wine" != "xno"; then
	NSIS_EXEC="wine $with_nsis";
fi

if test "x$with_windows_dlls" = "xauto" || test "x$with_windows_dlls" = "x"; then
	with_windows_dlls="/target/bin"
fi
WINDOWS_DLLS_LOCATION="$with_windows_dlls";

AC_SUBST(NSIS_EXEC)
AC_SUBST(WINDOWS_DLLS_LOCATION)


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


RES_FILES="src/res/gsc_about_dialog.glade \
	src/res/gsc_executor_log_window.glade \
	src/res/gsc_help_window.glade \
	src/res/gsc_info_window.glade \
	src/res/gsc_main_window.glade \
	src/res/gsc_preferences_window.glade \
	src/res/gsc_text_window.glade \
	AUTHORS.txt  LICENSE_gsmartcontrol.txt README.txt"

# Unfortunately, _SOURCES can't contain substitutions, so we use LIBADD.
RES_LIBADD=""  # .o file to put into .a, libres_a_LIBADD, libres_a_DEPENDENCIES.
RES_DIST=""  # only files in "res" directory, EXTRA_DIST
RES_TARGETS=""  # entry for each file in makefile

RES_GLD_EXT="glade"
if test "$app_cv_gld_reader" = "gtkbuilder"; then
	RES_GLD_EXT="ui"
fi

for RES_FILE in $RES_FILES;
do  # on separate line for portability
	RES_BASE=`echo "$RES_FILE" | sed 's/.*\///'`  # "a/b/c" -> "c", "c" -> "c"
	RES_DIR=`AS_DIRNAME(["$RES_FILE"])`  # "a/b/c" -> "a/b"
	RES_BASE_NOEXT=`expr "$RES_BASE" : '\(.*\)\..*'`  # "a.b.c" -> "a.b"
	RES_BASE_EXT=`expr "$RES_BASE" : '.*\.\(.*\)'`  # "a.b.c" -> "c"

	# Add to EXTRA_DIST only if it's in the same directory. Other files should
	# be added in their own directories.
	RES_FILE_CANON="\$(top_srcdir)/$RES_FILE"
	if test "$RES_DIR" = "src/res"; then
		RES_DIST="$RES_DIST $RES_BASE"
		RES_FILE_CANON="\$(srcdir)/$RES_BASE"  # shorter path, better make behaviour
	fi

	# We use separate source files for gtkbuilder and libglademm, because
	# make dist should be able to bundle both of them separately.

	if test "$RES_BASE_EXT" = "glade"; then

		RES_LIBADD="$RES_LIBADD $RES_BASE_NOEXT.$RES_GLD_EXT.$ac_objext"  # file.ui.o or file.glade.o

		# Output both glade and ui rules. This way make dist will be able
		# to build cpp files of both of them.

		RES_TARGETS="$RES_TARGETS
$RES_BASE_NOEXT.ui.cpp: $RES_FILE_CANON
	gtk-builder-convert \"$RES_FILE_CANON\" \"$RES_BASE_NOEXT\".tmp_ui
	\"\$(top_srcdir)/file2csource.sh\" \"$RES_BASE_NOEXT\".tmp_ui \"$RES_BASE_NOEXT.ui.cpp\" \"$RES_BASE_NOEXT\"_ui
"
		RES_TARGETS="$RES_TARGETS
$RES_BASE_NOEXT.glade.cpp: $RES_FILE_CANON
	\"\$(top_srcdir)/file2csource.sh\" \"$RES_FILE_CANON\" \"$RES_BASE_NOEXT.glade.cpp\" \"$RES_BASE_NOEXT\"_ui
"

	else  # txt, etc...

		SRC_FILE="$RES_BASE.cpp"
		RES_LIBADD="$RES_LIBADD $RES_BASE.$ac_objext"  # file.txt.o

		RES_TARGETS="$RES_TARGETS
$SRC_FILE: $RES_FILE_CANON
	\"\$(top_srcdir)/file2csource.sh\" \"$RES_FILE_CANON\" \"$SRC_FILE\" \"$RES_BASE_NOEXT\"_$RES_BASE_EXT
"
	fi

done


# export to makefile.am
AC_SUBST(RES_LIBADD)
AC_SUBST(RES_DIST)
AC_SUBST(RES_TARGETS)

# Don't put this one into Makefile.in by default - it's multiline and causes havoc.
_AM_SUBST_NOTMAKE([RES_TARGETS])


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Export some configure variables as defines for the source code.
# Note: Autoconf manual says that AC_DEFINE won't work here.

ADDITIONAL_FLAGS="-DPACKAGE_PKGDATA_DIR=\"\\\"\$(pkgdatadir)\\\"\" \
-DPACKAGE_SYSCONF_DIR=\"\\\"\$(sysconfdir)\\\"\" \
-DTOP_SRC_DIR=\"\\\"\$(top_srcdir)\\\"\""

#CFLAGS="$CFLAGS $ADDITIONAL_FLAGS"
CXXFLAGS="$CXXFLAGS $ADDITIONAL_FLAGS"


# Top src dir should always be in include path, before other includes.
# Add global configuration file. Note: It must be searched in top_builddir
# first (that's why -I is the first flag), for precompiled headers to work.

# Add pcrecpp too, because its files use #<file> on each other.
ADDITIONAL_INCLUDES="-I\$(top_builddir) -I\$(top_srcdir)/src -I\$(top_srcdir)/src/pcrecpp"

# -include works with gcc, intel and pathscale, but doesn't work with sun and pgi.
# Note: some gcc-frontend-based compilers may have to be added here
# (e.g. gccfss (gcc frontend / sun backend), etc...).
if test "$ax_cv_cxx_compiler_vendor" = "gnu" || test "$ax_cv_cxx_compiler_vendor" = "intel" \
		|| test "$ax_cv_cxx_compiler_vendor" = "pathscale"; then
	# This auto-includes this file when compiling any file. Benefits include pch support.
	ADDITIONAL_INCLUDES="$ADDITIONAL_INCLUDES -include global_macros.h"
else  # sun, pgi, others...
	# This forces the file to be included manually.
	ADDITIONAL_INCLUDES="$ADDITIONAL_INCLUDES -DHZ_NO_COMPILER_AUTOINCLUDE"
fi

# our flags must be before all the others
#CFLAGS="$ADDITIONAL_INCLUDES $CFLAGS"
CXXFLAGS="$ADDITIONAL_INCLUDES $CXXFLAGS"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# final setup

# This macro should be defined by ./configure automatically.
# However, sometimes it's not (e.g. win32, why?), so we define it manually.
CXXFLAGS="$CXXFLAGS -DHAVE_CONFIG_H"

#CFLAGS="$CFLAGS $USER_INCLUDES"
#CXXFLAGS="$CXXFLAGS $USER_INCLUDES"
#LIBS="$LIBS $USER_LDFLAGS"


# make them available in Makefile.am
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# all_includes="$all_includes $ALL_CXXFLAGS"
# all_libraries="$all_libraries $ALL_LIBS"
#all_includes="$all_includes $USER_INCLUDES"
#all_libraries="$all_libraries $LIBS"


# substitude them in makefile.am-s

#AC_SUBST(all_includes)
#AC_SUBST(all_libraries)



# print them out for debug purposes

AC_MSG_NOTICE([
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

# -------------------------------------------------------------------------------------


AC_CONFIG_FILES([ data/gsmartcontrol.desktop \
	data/nsis/distribution.txt data/nsis/gsmartcontrol.nsi \
	debian.dist/changelog \
	src/gsc_winres.rc \
	gsmartcontrol.spec version compilation_flags ])


AC_CONFIG_FILES([data/gsmartcontrol-root], [chmod +x data/gsmartcontrol-root])


# these are all the makefiles to generate

AC_CONFIG_FILES([Makefile src/Makefile src/applib/Makefile src/res/Makefile src/hz/Makefile \
	src/libdebug/Makefile src/pcrecpp/Makefile src/rconfig/Makefile src/rmn/Makefile \
	data/Makefile data/16/Makefile data/22/Makefile data/24/Makefile data/32/Makefile \
	data/48/Makefile data/64/Makefile data/128/Makefile data/256/Makefile data/nsis/Makefile \
	debian.dist/Makefile])


AC_OUTPUT


