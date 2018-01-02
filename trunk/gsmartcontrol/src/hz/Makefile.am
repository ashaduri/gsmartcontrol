AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

noinst_HEADERS = bad_cast_exception.h bin2ascii_encoder.h \
	data_file.h debug.h env_tools.h errno_string.h error.h error_holder.h \
	format_unit.h fs_common.h fs_dir.h fs_dir_platform.h fs_error_holder.h \
	fs_file.h fs_path.h fs_path_utils.h fs_tools.h hz_config.h i18n.h \
	instance_manager.h intrusive_ptr.h launch_url.h \
	locale_tools.h process_signal.h \
	res_data.h scoped_ptr.h stream_cast.h \
	string_algo.h string_num.h string_sprintf.h string_wcmatch.h \
	system_specific.h win32_tools.h

bin2ascii_encoder_example_SOURCES = bin2ascii_encoder_example.cpp
errno_string_example_SOURCES = errno_string_example.cpp
format_unit_example_SOURCES = format_unit_example.cpp
fs_dir_example_SOURCES = fs_dir_example.cpp
fs_file_example_SOURCES = fs_file_example.cpp
fs_path_example_SOURCES = fs_path_example.cpp
string_algo_example_SOURCES = string_algo_example.cpp
string_num_example_SOURCES = string_num_example.cpp
string_wcmatch_example_SOURCES = string_wcmatch_example.cpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_EXAMPLES
noinst_PROGRAMS += bin2ascii_encoder_example errno_string_example \
	format_unit_example fs_dir_example fs_file_example fs_path_example string_algo_example string_num_example \
	string_wcmatch_example
endif

examples: $(noinst_PROGRAMS)


