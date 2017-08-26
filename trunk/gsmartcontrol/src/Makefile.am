AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

SUBDIRS = libdebug hz rmn rconfig applib res


if TARGET_OS_KERNEL_FAMILY_WINDOWS
WINRES_OBJ = gsc_winres_win32.o
endif

# use different name for object file to avoid make auto-rules
gsc_winres_win32.o: $(top_srcdir)/data/gsmartcontrol.ico gsc_winres.rc
	cp -f $(top_srcdir)/data/gsmartcontrol.ico .
	$(WINDRES) gsc_winres.rc gsc_winres_win32.o

mostlyclean-local:
	rm -f gsmartcontrol.ico

noinst_DATA = gsc_winres.rc.in gsmartcontrol.exe.manifest.in


# if TARGET_OS_KERNEL_FAMILY_WINDOWS
# WIN_MAIN_SRC = win_main.cpp
# endif


bin_PROGRAMS = gsmartcontrol

gsmartcontrol_LDADD = $(top_builddir)/src/applib/libapplib.a \
	$(top_builddir)/src/libdebug/libdebug.a \
	$(top_builddir)/src/res/libres.a $(WINRES_OBJ)

noinst_HEADERS = global_macros.h gsc_about_dialog.h gsc_add_device_window.h gsc_executor_error_dialog.h \
	gsc_executor_log_window.h gsc_help_window.h gsc_info_window.h gsc_init.h gsc_main_window.h \
	gsc_main_window_iconview.h gsc_preferences_window.h gsc_settings.h gsc_text_window.h
gsmartcontrol_SOURCES = $(WIN_MAIN_SRC) gsc_about_dialog.cpp gsc_add_device_window.cpp \
	gsc_executor_error_dialog.cpp gsc_executor_log_window.cpp gsc_help_window.cpp gsc_info_window.cpp \
	gsc_init.cpp gsc_main.cpp gsc_main_window.cpp gsc_preferences_window.cpp

