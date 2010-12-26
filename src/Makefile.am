AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO


bin_PROGRAMS = gsmartcontrol

SUBDIRS = libdebug hz rmn rconfig pcrecpp applib res
gsmartcontrol_LDADD = $(top_builddir)/src/applib/libapplib.a \
	$(top_builddir)/src/pcrecpp/libpcrecpp.a $(top_builddir)/src/libdebug/libdebug.a \
	$(top_builddir)/src/res/libres.a

noinst_HEADERS = global_macros.h gsc_about_dialog.h gsc_executor_error_dialog.h \
	gsc_executor_log_window.h gsc_help_window.h gsc_info_window.h gsc_init.h gsc_main_window.h \
	gsc_main_window_iconview.h gsc_preferences_window.h gsc_settings.h gsc_text_window.h
gsmartcontrol_SOURCES = gsc_about_dialog.cpp gsc_executor_error_dialog.cpp \
	gsc_executor_log_window.cpp gsc_help_window.cpp gsc_info_window.cpp gsc_init.cpp gsc_main.cpp \
	gsc_main_window.cpp gsc_preferences_window.cpp
