AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = resource_base.h resource_data_any.h \
	resource_data_types.h resource_exception.h resource_node.h \
	resource_node_dump.h resource_serialization.h rmn.h

rmn_example_SOURCES = rmn_example.cpp

# Not working yet
# EXTRA_DIST = resource_data_multi.h.notworing

# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_EXAMPLES
noinst_PROGRAMS += rmn_example
endif

examples: $(noinst_PROGRAMS)


