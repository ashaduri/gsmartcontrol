
# Disable implicit suffix rules for these extensions.
# The problem is that make sees the .glade.cpp or .ui.cpp files
# with their .glade prerequisites, and decides to rebuild the
# (already existing) .glade files by using .glade.cpp files.
# Implicit rules are to blame, so disable them for .glade.
SUFFIXES = .glade .txt

AM_CPPFLAGS =
METASOURCES = AUTO

noinst_LIBRARIES = libres.a

libres_a_SOURCES =

# List _all_ sources that may be compiled. This is basically the same list as
# in configure.ac, but with .cpp extensions and without paths.
EXTRA_libres_a_SOURCES = gsc_about_dialog.glade.cpp gsc_about_dialog.ui.cpp \
	gsc_executor_log_window.glade.cpp gsc_executor_log_window.ui.cpp \
	gsc_executor_log_window.glade.cpp gsc_executor_log_window.ui.cpp \
	gsc_help_window.glade.cpp gsc_help_window.ui.cpp \
	gsc_info_window.glade.cpp gsc_info_window.ui.cpp \
	gsc_main_window.glade.cpp gsc_main_window.ui.cpp \
	gsc_preferences_window.glade.cpp gsc_preferences_window.ui.cpp \
	gsc_text_window.glade.cpp gsc_text_window.ui.cpp \
	AUTHORS.txt.cpp LICENSE_gsmartcontrol.txt.cpp README.txt.cpp

# list all intermediate files here (.tmp_ui files are cleaned automatically by make, but it won't hurt)
mostlyclean-local:
	rm -f *.glade.cpp *.ui.cpp *.tmp_ui *.txt.cpp


# This lists all objects that are actually built.
libres_a_LIBADD = $(RES_LIBADD)
libres_a_DEPENDENCIES = $(RES_LIBADD)

# This lists all resources in this directory (otherwise they don't get bundled,
# noinst_DATA doesn't put them into distribution (why?)).
EXTRA_DIST = $(RES_DIST)


# This lists the actual makefile rules for each target.
@RES_TARGETS@



# # List all local resource files here. Note: noinst_DATA doesn't put them into distribution (why?)
# EXTRA_DIST = gsc_about_dialog.glade gsc_executor_log_window.glade \
# 	gsc_help_window.glade gsc_info_window.glade gsc_main_window.glade \
# 	gsc_preferences_window.glade gsc_text_window.glade
#
# Note: All .cpp files will be generated during "make dist" and bundled with
# the distribution.
#
# libres_a_SOURCES = AUTHORS.txt.cpp LICENSE_gsmartcontrol.txt.cpp README.txt.cpp
#
#
# if ENABLE_GTKBUILDER
# # list all .glade.cpp files here
# libres_a_SOURCES += gsc_about_dialog.ui.cpp gsc_executor_log_window.ui.cpp \
# 	gsc_help_window.ui.cpp gsc_info_window.ui.cpp gsc_main_window.ui.cpp \
# 	gsc_preferences_window.ui.cpp gsc_text_window.ui.cpp
# endif
#
# if ENABLE_LIBGLADE
# # list all .ui.cpp files here
# libres_a_SOURCES += gsc_about_dialog.glade.cpp gsc_executor_log_window.glade.cpp \
# 	gsc_help_window.glade.cpp gsc_info_window.glade.cpp gsc_main_window.glade.cpp \
# 	gsc_preferences_window.glade.cpp gsc_text_window.glade.cpp
# endif
#
# # Note: GNU patterns here. Can't convert them to suffix rules
# # because paths in targets don't seem to be supported by them.
#
# # Note: We use abs_top_srcdir (instead of top_srcdir) because without
# # it we get "file name too long" errors when compiling in this directory.
# # (automake bug?)
#
# # GtkBuilder support
# %.tmp_ui: $(abs_top_srcdir)/src/res/%.glade
# 	gtk-builder-convert $< $@
#
# %.ui.cpp: %.tmp_ui
# 	$(top_srcdir)/file2csource.sh $< $@ $*_ui
#
#
# # Libglade support
# %.glade.cpp: $(abs_top_srcdir)/src/res/%.glade
# 	$(top_srcdir)/file2csource.sh $< $@ $*_ui
#
# # Common compiled-in files (resources)
#
# # This covers the files in top srcdir only.
# %.txt.cpp: $(abs_top_srcdir)/%.txt
# 	$(top_srcdir)/file2csource.sh $< $@ $*_txt
#

