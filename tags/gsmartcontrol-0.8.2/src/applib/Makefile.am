AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_LIBRARIES = libapplib.a
libapplib_a_SOURCES = app_gtkmm_utils.cpp app_pango_utils.cpp cmdex.cpp \
	cmdex_sync.cpp cmdex_sync_gui.cpp gui_utils.cpp selftest.cpp smartctl_parser.cpp \
	storage_detector.cpp storage_device.cpp storage_property.cpp storage_property_descr.cpp
noinst_HEADERS = app_gtkmm_features.h app_gtkmm_utils.h app_pango_utils.h \
	app_pcrecpp.h app_ui_res_utils.h cmdex.h cmdex_sync.h cmdex_sync_gui.h gui_utils.h \
	selftest.h smartctl_executor.h smartctl_executor_gui.h smartctl_parser.h \
	storage_detector.h storage_device.h storage_property.h storage_property_colors.h \
	storage_property_descr.h storage_settings.h wrapping_label.h

# don't use absolute path for the current dir's .a, because the make
# dependency resolver won't get it (needed for parallel builds)

app_ui_res_utils_test_SOURCES = app_ui_res_utils_test.cpp app_ui_res_utils_test_data.cpp
app_ui_res_utils_test_LDADD = libapplib.a \
	$(top_builddir)/src/libdebug/libdebug.a

smartctl_executor_test_SOURCES = smartctl_executor_test.cpp
smartctl_executor_test_LDADD = libapplib.a \
	$(top_builddir)/src/libdebug/libdebug.a

smartctl_parser_test_SOURCES = smartctl_parser_test.cpp
smartctl_parser_test_LDADD = libapplib.a \
	$(top_builddir)/src/pcrecpp/libpcrecpp.a $(top_builddir)/src/libdebug/libdebug.a

storage_detector_test_SOURCES = storage_detector_test.cpp
storage_detector_test_LDADD = libapplib.a \
	$(top_builddir)/src/pcrecpp/libpcrecpp.a $(top_builddir)/src/libdebug/libdebug.a

spawn_test_SOURCES = spawn_test.cpp

# Build them first
SUBDIRS=../libdebug ../pcrecpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_TESTS
noinst_PROGRAMS += app_ui_res_utils_test smartctl_executor_test smartctl_parser_test \
	storage_detector_test spawn_test
endif

tests: $(noinst_PROGRAMS)


