AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = autosave.h loadsave.h config.h

rconfig_example_SOURCES = rconfig_example.cpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_EXAMPLES
noinst_PROGRAMS += rconfig_example
endif

examples: $(noinst_PROGRAMS)


