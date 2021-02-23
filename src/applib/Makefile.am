AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_LIBRARIES = libapplib.a
libapplib_a_SOURCES = app_gtkmm_utils.cpp cmdex.cpp \
	cmdex_sync.cpp cmdex_sync_gui.cpp executor_factory.cpp gui_utils.cpp selftest.cpp smartctl_executor.cpp \
	smartctl_parser.cpp storage_detector.cpp storage_detector_linux.cpp storage_detector_other.cpp \
	storage_detector_win32.cpp storage_device.cpp storage_property.cpp \
	storage_property_descr.cpp
noinst_HEADERS = app_builder_widget.h app_gtkmm_features.h app_gtkmm_utils.h \
	app_pcrecpp.h cli_executors.h cmdex.h cmdex_sync.h cmdex_sync_gui.h executor_factory.h gui_utils.h \
	selftest.h smartctl_executor.h smartctl_executor_gui.h smartctl_parser.h \
	storage_detector.h storage_detector_helpers.h storage_detector_linux.h storage_detector_other.h \
	storage_detector_win32.h storage_device.h storage_property.h storage_property_colors.h \
	storage_property_descr.h storage_settings.h

# don't use absolute path for the current dir's .a, because the make
# dependency resolver won't get it (needed for parallel builds)

smartctl_executor_example_SOURCES = smartctl_executor_example.cpp
smartctl_executor_example_LDADD = libapplib.a \
	$(top_builddir)/src/libdebug/libdebug.a

smartctl_parser_example_SOURCES = smartctl_parser_example.cpp
smartctl_parser_example_LDADD = libapplib.a \
	$(top_builddir)/src/libdebug/libdebug.a

storage_detector_example_SOURCES = storage_detector_example.cpp
storage_detector_example_LDADD = libapplib.a \
	$(top_builddir)/src/libdebug/libdebug.a

spawn_example_SOURCES = spawn_example.cpp

# Build them first
SUBDIRS=../libdebug


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_EXAMPLES
noinst_PROGRAMS += smartctl_executor_example smartctl_parser_example \
	storage_detector_example spawn_example
endif

examples: $(noinst_PROGRAMS)


