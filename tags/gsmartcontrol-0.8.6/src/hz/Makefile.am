AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

noinst_HEADERS = any_convert.h any_type.h any_type_holder.h ascii.h \
	bad_cast_exception.h bin2ascii_encoder.h common_types.h cstdint.h cstdint_impl_msvc.h \
	data_file.h debug.h down_cast.h env_tools.h errno_string.h error.h error_holder.h \
	exceptions.h format_unit.h fs_common.h fs_dir.h fs_dir_platform.h fs_error_holder.h \
	fs_file.h fs_fstream.h fs_path.h fs_path_utils.h fs_tools.h function.h function_impl.h hz_config.h i18n.h \
	instance_manager.h intrusive_ptr.h intrusive_scoped_lock.h launch_url.h local_algo.h \
	locale_tools.h noncopyable.h optional_value.h portable_snprintf.h process_signal.h \
	ptr_container.h res_data.h scoped_array.h scoped_ptr.h shared_ptr.h static_assert.h stream_cast.h \
	string_algo.h string_format.h string_num.h string_sprintf.h string_sprintf_macros.h string_wcmatch.h sync.h \
	sync_lock_ptr.h sync_multilock.h sync_part_get_policy.h sync_policy_boost.h \
	sync_policy_glib.h sync_policy_glibmm.h sync_policy_poco.h sync_policy_pthread.h \
	sync_policy_win32.h system_specific.h tls.h tls_policy_boost.h tls_policy_glib.h \
	tls_policy_pthread.h tls_policy_win32.h type_categories.h type_properties.h win32_tools.h

any_type_test_SOURCES = any_type_test.cpp
bin2ascii_encoder_test_SOURCES = bin2ascii_encoder_test.cpp
down_cast_test_SOURCES = down_cast_test.cpp
errno_string_test_SOURCES = errno_string_test.cpp
format_unit_test_SOURCES = format_unit_test.cpp
fs_dir_test_SOURCES = fs_dir_test.cpp
fs_file_test_SOURCES = fs_file_test.cpp
fs_path_test_SOURCES = fs_path_test.cpp
string_algo_test_SOURCES = string_algo_test.cpp
string_format_test_SOURCES = string_format_test.cpp
string_num_test_SOURCES = string_num_test.cpp
string_wcmatch_test_SOURCES = string_wcmatch_test.cpp
sync_test_SOURCES = sync_test.cpp
sync_multilock_test_SOURCES = sync_multilock_test.cpp
tls_test_SOURCES = tls_test.cpp


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_TESTS
noinst_PROGRAMS += any_type_test bin2ascii_encoder_test down_cast_test errno_string_test \
	format_unit_test fs_dir_test fs_file_test fs_path_test string_algo_test string_num_test string_format_test \
	string_wcmatch_test sync_test sync_multilock_test tls_test
endif

tests: $(noinst_PROGRAMS)


