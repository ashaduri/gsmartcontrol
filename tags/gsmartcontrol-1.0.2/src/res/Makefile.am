
# Disable implicit suffix rules for these extensions.
# The problem is that make sees the .glade.cpp or .ui.cpp files
# with their .glade prerequisites, and decides to rebuild the
# (already existing) .glade files by using .glade.cpp files.
# Implicit rules are to blame, so disable them for .glade.
SUFFIXES = .ui .txt

AM_CPPFLAGS =
METASOURCES = AUTO

noinst_LIBRARIES = libres.a

libres_a_SOURCES =

# List _all_ sources that may be compiled. This is basically the same list as
# in configure.ac, but with .cpp extensions and without paths.
EXTRA_libres_a_SOURCES = gsc_about_dialog.ui.cpp \
	gsc_add_device_window.ui.cpp \
	gsc_executor_log_window.ui.cpp \
	gsc_help_window.ui.cpp \
	gsc_info_window.ui.cpp \
	gsc_main_window.ui.cpp \
	gsc_preferences_window.ui.cpp \
	gsc_text_window.ui.cpp \
	AUTHORS.txt.cpp LICENSE_gsmartcontrol.txt.cpp README.txt.cpp

# list all intermediate files here (.tmp_ui files are cleaned automatically by make, but it won't hurt)
mostlyclean-local:
	rm -f *.ui.cpp *.tmp_ui *.txt.cpp


# This lists all objects that are actually built.
libres_a_LIBADD = $(RES_LIBADD)
libres_a_DEPENDENCIES = $(RES_LIBADD)

# This lists all resources in this directory (otherwise they don't get bundled,
# noinst_DATA doesn't put them into distribution (why?)).
EXTRA_DIST = $(RES_DIST)


# This lists the actual makefile rules for each target.
@RES_TARGETS@


