
AC_INIT([GSmartControl], [0.8.0rc1], [], [gsmartcontrol])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_TARGET

# disable portability warnings because of pattern rules in src/res.
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 -Wall,no-portability])

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

# to be able to generate static libs
AC_CHECK_TOOL(RANLIB, ranlib)


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Various compiler features

# detect compiler vendor (sets $ax_cv_cxx_compiler_vendor)
AX_COMPILER_VENDOR

# detect RTTI (defines HAVE_RTTI)
AC_CXX_RTTI

# detect exception support (defines HAVE_EXCEPTIONS)
AC_CXX_EXCEPTIONS

# detect abi::__cxa_demangle (defines HAVE_GCC_ABI_DEMANGLE)
AX_CXX_GCC_ABI_DEMANGLE

# detect __gnu_cxx::__verbose_terminate_handler (defines HAVE_VERBOSE_TERMINATE_HANDLER.)
AC_CXX_VERBOSE_TERMINATE_HANDLER

# detect __func__ support (defines HAVE_CXX__FUNC)
AC_CXX_VAR_FUNC


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Portability stuff

# arrange large file support (define _FILE_OFFSET_BITS 64, etc...)
AC_SYS_LARGEFILE

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

# This may be needed for pch support
AC_CHECK_HEADERS([stdc++.h])

# pcrecpp needs this (break on first function found)
AC_CHECK_FUNCS([strtoll _strtoi64], [break])

# Non-C99 systems don't have strtof and strtold. some have strtodf for strtof,
# but let's ignore it. Note: They may be supported by system libc,
# but unsupported by headers (e.g. sun studio on linux). In that case,
# defining them still leads to undefined symbols at link time, so we don't do it.
# header / link check:
AC_CHECK_DECLS([strtof, strtold], [], [], [[#include <stdlib.h>]])
# link check only:
# AC_CHECK_FUNCS([strtof strtold])


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# "build" - the type of system on which the package is being configured and compiled.
ac_windows_build="no"
ac_linux_build="no"
case "${host}" in
	*mingw*)
		ac_windows_build="yes"
		;;
	*linux*)
		ac_linux_build="yes"
		;;
esac

# export to Makefile.am-s
AM_CONDITIONAL([WINDOWS_BUILD], [test "$ac_windows_build" = "yes"])
AM_CONDITIONAL([LINUX_BUILD], [test "$ac_linux_build" = "yes"])


# "host" - the type of system on which the package runs. Specifying it enables the
# cross-compilation mode. "target" defaults to this too.
ac_windows_target="no"
ac_linux_target="no"
case "${target}" in
	*mingw*)
		ac_windows_target="yes"
		;;
	*linux*)
		ac_linux_target="yes"
		;;
esac

# export to Makefile.am-s
AM_CONDITIONAL([WINDOWS_TARGET], [test "$ac_windows_target" = "yes"])
AM_CONDITIONAL([LINUX_TARGET], [test "$ac_linux_target" = "yes"])


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


COMPILER_CXXFLAGS=""
COMPILER_LDFLAGS=""
# reset original user-supplied *FLAGS
RESET_ORIGINAL="no"


AC_ARG_ENABLE(default-gcc-options,
	AS_HELP_STRING([--enable-default-gcc-options],[enable default compilation options (warnings, mostly) (GCC only) (default: disabled)]),
	[default_gcc_options=${enableval}], [default_gcc_options=no])

if test "$default_gcc_options" = "yes"; then
	if test "$ac_windows_target" = "yes"; then
		COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS -mms-bitfields -mwindows"
		COMPILER_LDFLAGS="$COMPILER_CXXFLAGS -mms-bitfields"
	fi
	COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS \
-Wall -Wcast-align -Wcast-qual -Wconversion \
-Wctor-dtor-privacy -Wfloat-equal -Wnon-virtual-dtor -Woverloaded-virtual \
-Wpointer-arith -Wshadow -Wsign-compare -Wsign-promo -Wundef -Wwrite-strings"
	RESET_ORIGINAL="yes";
fi


AC_ARG_ENABLE(gcc-debug,
	AS_HELP_STRING([--enable-gcc-debug],[enable debug build (GCC only) (default: disabled)]),
	[gcc_debug=${enableval}], [gcc_debug=no])

if test "$gcc_debug" = "yes"; then
	COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS -g3 -O0 -DDEBUG -DDEBUG_BUILD"
	COMPILER_LDFLAGS="$COMPILER_LDFLAGS -g3 -O0"
	RESET_ORIGINAL="yes";
fi


AC_ARG_ENABLE(gcc-optimize,
	AS_HELP_STRING([--enable-gcc-optimize],[enable optimized build (GCC only) (default: disabled)]),
	[gcc_optimize=${enableval}], [gcc_optimize=no])

if test "$gcc_optimize" = "yes"; then
	# Note: Do NOT put CPU flags here - they make the program non-portable.
	if test "$ac_windows_target" = "yes"; then
		COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS -g0 -O3 -s"
		COMPILER_LDFLAGS="$COMPILER_LDFLAGS -s"
	else
		COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS -g0 -O3 -s -mtune=generic"
		COMPILER_LDFLAGS="$COMPILER_LDFLAGS -s -mtune=generic"
	fi
	RESET_ORIGINAL="yes";
fi


AC_ARG_ENABLE(gcc_pch,
	AS_HELP_STRING([--enable-gcc-pch],[enable precompiled header support (pch make target) (GCC only) (default: disabled)
			(use "make gch" to speed up the compilation)]),
	[gcc_pch=${enableval}], [gcc_pch=no])

if test "$gcc_pch" = "yes"; then
	COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS -Winvalid-pch -DENABLE_PCH"
fi


# define LINUX for linux target
if test "$ac_linux_target" = "yes"; then
	COMPILER_CXXFLAGS="$COMPILER_CXXFLAGS -DLINUX"
fi


# Reset user-supplied flags if one of the above options is present.
# We append COMPILER_* counterparts in the end, to override any
# previous options.
if test "$RESET_ORIGINAL" = "yes"; then
	CFLAGS=""
	CXXFLAGS=""
	LIBS=""
	LDFLAGS=""
fi


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Enable compiler thread switches for known platforms.

AC_MSG_NOTICE([checking thread support flags...])

THREAD_CFLAGS=""
THREAD_LIBS=""
THREAD_FOUND="no"

if test "$ac_windows_target" = "yes"; then
	if test "$ax_cv_cxx_compiler_vendor" = "gnu"; then
		THREAD_CFLAGS="-mthreads -D_THREAD_SAFE"
		THREAD_LIBS="-mthreads"
		THREAD_FOUND="yes"
	fi

elif test "$ac_linux_target" = "yes"; then
	if test "$ax_cv_cxx_compiler_vendor" = "gnu" || test "$ax_cv_cxx_compiler_vendor" = "intel"; then
		THREAD_CFLAGS="-pthread -D_MT -D_THREAD_SAFE"
		THREAD_LIBS="-pthread"
		THREAD_FOUND="yes"
	elif test "$ax_cv_cxx_compiler_vendor" = "sun"; then
		THREAD_CFLAGS="-mt"
		THREAD_LIBS="-mt"
		THREAD_FOUND="yes"
	fi
fi

if test "$THREAD_FOUND" = "no"; then
	AC_MSG_NOTICE([Cannot detect compiler thread support. Set CXXFLAGS and LDFLAGS manually.])

else
	AC_MSG_NOTICE([CFLAGS: $THREAD_CFLAGS;  LIBS: $THREAD_LIBS])
	CXXFLAGS="$CXXFLAGS $THREAD_CFLAGS"
	LIBS="$LIBS $THREAD_LIBS"
fi


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# iconview is available since 2.6.0. gthread is not enabled by default.
pkg_modules="gtkmm-2.4 >= 2.6.0 gthread-2.0 >= 2.0"

# define ENABLE_GLIB and friends if it's found.
gtkmm_found="no"

PKG_CHECK_MODULES([GTKMM], [$pkg_modules], [
		AC_DEFINE([ENABLE_GLIB], [], [Whether GLib is installed])
		AC_DEFINE([ENABLE_GLIBMM], [], [Whether GLibmm is installed])
		gtkmm_found="yes"
	], [
		AC_MSG_WARN([$GTKMM_PKG_ERRORS])
		AC_MSG_WARN([The program will be unable to compile fully.])
	])

# use this instead of the above one to enable abort-on-not-found behaviour.
# PKG_CHECK_MODULES([GTKMM], [$pkg_modules])

# GTK flags must be prepended, or glib_config.h conflict between
# gtk 2.x and 1.2 may occur.

#CFLAGS="$GTKMM_CFLAGS $CFLAGS"
CXXFLAGS="$GTKMM_CFLAGS $CXXFLAGS"
LIBS="$GTKMM_LIBS $LIBS"

# needed for in-code checks (AC_DEFINE() takes care of these above, no need of them here)
# CXXFLAGS="$CXXFLAGS -DENABLE_GLIB -DENABLE_GLIBMM"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


gld_found=no;  # equals to "gtkbuilder" or "libglademm" after tests

#if test "gtkmm_found" = "yes"; then

	# Check whether we should use Gtk::Builder (gtkmm >= 2.12) or libglademm.
	AC_ARG_ENABLE(libglademm, AS_HELP_STRING([--enable-libglademm],
			[use libglademm instead of GtkBuilder (default: auto)]), [with_libglademm=${enableval}], [with_libglademm=auto])


	# try gtkbuilder first
	if test "$with_libglademm" = "no" || test "$with_libglademm" = "auto"; then
		PKG_CHECK_EXISTS([gtkmm-2.4 >= 2.12.0], [gld_found=gtkbuilder])
	fi
	if test "$with_libglademm" = "no" && test "$gld_found" = "no"; then
		AC_MSG_ERROR([GtkBuilder support not found (you need gtkmm version 2.12 or higher). Try using libglademm instead.])
	fi

	# libglade specified, or auto and no gtkbuilder. gtkmm2.4-compatible libglade is >= 2.4.0.
	if test "$gld_found" = "no"; then
		PKG_CHECK_MODULES([LIBGLADEMM], [libglademm-2.4 >= 2.4.0], [gld_found="libglademm"],
				[AC_MSG_WARN([$LIBGLADEMM_PKG_ERRORS])])
	fi
	if test "$with_libglademm" = "yes" && test "$gld_found" = "no"; then
		AC_MSG_ERROR([Libglademm not found. Try building with GtkBuilder support (you need gtkmm version 2.12 or higher).])
	fi

	# still not found.
	if test "$gld_found" = "no"; then
		AC_MSG_WARN([Neither GtkBuilder nor libglademm found. The program will be unable to compile fully.])
	fi


	AC_MSG_NOTICE([Glade file support through: $gld_found])

	# merge libglademm flags
	if test "$gld_found" = "libglademm"; then
		CXXFLAGS="$LIBGLADEMM_CFLAGS $CXXFLAGS"
		LIBS="$LIBGLADEMM_LIBS $LIBS"
	fi

#fi


# export to automake.am-s
AM_CONDITIONAL([ENABLE_LIBGLADEMM], [test "$gld_found" = "libglademm"])
AM_CONDITIONAL([ENABLE_GTKBUILDER], [test "$gld_found" = "gtkbuilder"])

# export to config.h
if test "$gld_found" = "libglademm"; then
	AC_DEFINE(ENABLE_LIBGLADEMM, 1, [Use libglademm instead of GtkBuilder])
fi
if test "$gld_found" = "gtkbuilder"; then
	AC_DEFINE(ENABLE_GTKBUILDER, 1, [Use GtkBuilder instead of libglademm])
fi


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


pkg_modules="libpcre"

PKG_CHECK_MODULES(PCRE, [$pkg_modules])

#CFLAGS="$PCRE_CFLAGS $CFLAGS"
CXXFLAGS="$PCRE_CFLAGS $CXXFLAGS"
LIBS="$PCRE_LIBS $LIBS"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# enable / disable compilation of tests
AC_ARG_ENABLE([tests], AS_HELP_STRING([--enable-tests],
		[enable compilation of test programs (default: no)]), [with_tests=${enableval}], [with_tests=no])
# export to automake.am-s
AM_CONDITIONAL([ENABLE_TESTS], [test "$with_tests" = "yes"])


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
if test "$gld_found" = "gtkbuilder"; then
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


# Export some configure variables as defines for the source code

ADDITIONAL_FLAGS="-DPACKAGE_PKGDATA_DIR=\"\\\"\$(pkgdatadir)\\\"\" \
-DPACKAGE_SYSCONF_DIR=\"\\\"\$(sysconfdir)\\\"\" \
-DTOP_SRC_DIR=\"\\\"\$(top_srcdir)\\\"\""

#CFLAGS="$CFLAGS $ADDITIONAL_FLAGS"
CXXFLAGS="$CXXFLAGS $ADDITIONAL_FLAGS"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------

# Top src dir should always be in include path, before other includes.
# Add global configuration file. Note: It must be searched in top_builddir
# first (that's why -I is the first flag), for precompiled headers to work.

# Add pcrecpp too, because its files use #<file> on each other.
ADDITIONAL_INCLUDES="-I\$(top_builddir) -I\$(top_srcdir)/src -I\$(top_srcdir)/src/pcrecpp"

# -include doesn't work with sun and pgi, but works with intel and gcc.
# we don't whitelist intel/gcc, because there are some gcc-frontend-based
# compilers we don't want to break (pathscale, gccfss (gcc frontend / sun backend), etc...).
if test "$ax_cv_cxx_compiler_vendor" != "sun" && test "$ax_cv_cxx_compiler_vendor" != "portland"; then
	ADDITIONAL_INCLUDES="$ADDITIONAL_INCLUDES -include global_macros.h"
fi

#CFLAGS="$ADDITIONAL_INCLUDES $CFLAGS"
# our flags must be before all the others
CXXFLAGS="$ADDITIONAL_INCLUDES $CXXFLAGS"


# -------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------


# Append compiler flags to CXXFLAGS, so that they override
# whatever the user has set.
CXXFLAGS="$CXXFLAGS $COMPILER_CXXFLAGS"
LIBS="$LIBS $COMPILER_LDFLAGS"

# This macro should be defined by ./configure automatically.
# However, sometimes it's not (e.g. win32, why?), so we define it manually.
CXXFLAGS="$CXXFLAGS -DHAVE_CONFIG_H"


# -------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------

# final setup

#CFLAGS="$CFLAGS $USER_INCLUDES"
#CXXFLAGS="$CXXFLAGS $USER_INCLUDES"
#LIBS="$LIBS $USER_LDFLAGS"


# make them available in Makefile.am
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)


# -------------------------------------------------------------------------------------


# all_includes="$all_includes $ALL_CXXFLAGS"
# all_libraries="$all_libraries $ALL_LIBS"
#all_includes="$all_includes $USER_INCLUDES"
#all_libraries="$all_libraries $LIBS"


# substitude them in makefile.am-s

#AC_SUBST(all_includes)
#AC_SUBST(all_libraries)



# print'em out for debug purposes

AC_MSG_NOTICE([
CXXFLAGS:
$CXXFLAGS
LIBS:
$LIBS
])


# substitude autotools' predefined dir names in makefile.am-s

AC_SUBST(AUTODIRS)


# ----------------------------------------------------

AC_CONFIG_FILES([ data/gsmartcontrol_gnome.desktop data/gsmartcontrol_kde.desktop \
	data/gsmartcontrol_other.desktop \
	gsmartcontrol.spec version compilation_flags ])


# these are all the makefiles to generate

AC_CONFIG_FILES([Makefile src/Makefile src/applib/Makefile src/res/Makefile src/hz/Makefile \
	src/libdebug/Makefile src/pcrecpp/Makefile src/rconfig/Makefile src/rmn/Makefile \
	data/Makefile data/16/Makefile data/22/Makefile data/24/Makefile data/32/Makefile \
	data/48/Makefile data/64/Makefile data/128/Makefile])
AC_OUTPUT


