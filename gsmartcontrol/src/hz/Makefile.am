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

bin2ascii_encoder_test_SOURCES = bin2ascii_encoder_test.cpp
errno_string_test_SOURCES = errno_string_test.cpp
format_unit_test_SOURCES = format_unit_test.cpp
fs_dir_test_SOURCES = fs_dir_test.cpp
fs_file_test_SOURCES = fs_file_test.cpp
fs_path_test_SOURCES = fs_path_test.cpp
string_algo_test_SOURCES = string_algo_test.cpp
string_num_test_SOURCES = string_num_test.cpp
string_wcmatch_test_SOURCES = string_wcmatch_test.cpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_TESTS
noinst_PROGRAMS += bin2ascii_encoder_test errno_string_test \
	format_unit_test fs_dir_test fs_file_test fs_path_test string_algo_test string_num_test \
	string_wcmatch_test
endif

tests: $(noinst_PROGRAMS)


