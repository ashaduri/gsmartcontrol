AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = rcautosave.h rcdump.h rcloadsave.h rcmain.h rconfig.h \
	rconfig_mini.h

rconfig_test_SOURCES = rconfig_test.cpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_TESTS
noinst_PROGRAMS += rconfig_test
endif

tests: $(noinst_PROGRAMS)


