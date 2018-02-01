AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

noinst_HEADERS = bad_cast_exception.h bin2ascii_encoder.h \
	data_file.h debug.h env_tools.h error.h error_holder.h \
	format_unit.h fs.h fs_ns.h \
	instance_manager.h launch_url.h \
	locale_tools.h process_signal.h \
	scoped_ptr.h stream_cast.h \
	string_algo.h string_num.h string_sprintf.h \
	system_specific.h win32_tools.h

bin2ascii_encoder_example_SOURCES = bin2ascii_encoder_example.cpp
format_unit_example_SOURCES = format_unit_example.cpp
string_algo_example_SOURCES = string_algo_example.cpp
string_num_example_SOURCES = string_num_example.cpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_EXAMPLES
noinst_PROGRAMS += bin2ascii_encoder_example \
	format_unit_example string_algo_example string_num_example
endif

examples: $(noinst_PROGRAMS)


