
# This cmake file is only for IDE integration, it should not be used
# to compile this program.

cmake_minimum_required(VERSION 3.13)
project(gsmartcontrol)

set(CMAKE_CXX_STANDARD 17)


# Clang5 doesn't understand libstdc++'s std::get(variant), so use libc++.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	option(APP_COMPILER_CLANG_USE_LIBCXX "Use LLVM libc++ instead of gcc's libstdc++ (clang)" ON)
	if (APP_COMPILER_CLANG_USE_LIBCXX)
		add_compile_options(-stdlib=libc++)
	endif()
endif()


set(_enable_clang_tidy OFF)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(_enable_clang_tidy ON)
endif()

option(APP_COMPILER_ANALYZE_CLANG_TIDY "Use clang-tidy in Analysis build type" ${_enable_clang_tidy})
set(APP_COMPILER_ANALYZE_CLANG_TIDY_CMDLINE "clang-tidy"  # reads configuration from root-level .clang-tidy file
	CACHE STRING "Command-line for clang-tidy.")
if (APP_COMPILER_ANALYZE_CLANG_TIDY)
	set (CMAKE_CXX_CLANG_TIDY "${APP_COMPILER_ANALYZE_CLANG_TIDY_CMDLINE}")
endif()


add_compile_options(-Wall -Wextra -Wpedantic
	-Wshadow -Wpointer-arith
	-Wundef -Wunused-macros -Wcast-qual -Wcast-align -Wconversion
	-Wmissing-declarations -Wpacked -Wredundant-decls -Wvla -Woverlength-strings
	-Wnon-virtual-dtor -Woverloaded-virtual
	-Wno-missing-field-initializers
	)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options(
		-Wdocumentation
		-Wheader-guard
		-Wloop-analysis
		-Wno-keyword-macro
	)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options(
		-Wnoexcept
#		-Wsuggest-attribute=const
		-Wsuggest-attribute=noreturn
		-Wsuggest-attribute=format
#		-Wsuggest-final-types  # works better with LTO
#		-Wsuggest-final-methods
		-Wsuggest-override
		-Wno-virtual-move-assign
		-Wdate-time
	)
endif()

# Some warnings are only triggered at higher optimization levels
add_compile_options(-O2)


add_executable(gsmartcontrol
	src/applib/app_builder_widget.h
	src/applib/app_gtkmm_features.h
	src/applib/app_gtkmm_utils.cpp
	src/applib/app_gtkmm_utils.h
	src/applib/app_pcrecpp.h
	src/applib/cli_executors.h
	src/applib/cmdex.cpp
	src/applib/cmdex.h
	src/applib/cmdex_sync.cpp
	src/applib/cmdex_sync.h
	src/applib/cmdex_sync_gui.cpp
	src/applib/cmdex_sync_gui.h
	src/applib/executor_factory.cpp
	src/applib/executor_factory.h
	src/applib/gui_utils.cpp
	src/applib/gui_utils.h
	src/applib/selftest.cpp
	src/applib/selftest.h
	src/applib/smartctl_executor.cpp
	src/applib/smartctl_executor.h
	src/applib/smartctl_executor_gui.h
#	src/applib/smartctl_executor_example.cpp
	src/applib/smartctl_parser.cpp
	src/applib/smartctl_parser.h
#	src/applib/smartctl_parser_example.cpp
#	src/applib/spawn_example.cpp
	src/applib/storage_detector.cpp
	src/applib/storage_detector.h
	src/applib/storage_detector_helpers.h
	src/applib/storage_detector_linux.cpp
	src/applib/storage_detector_linux.h
	src/applib/storage_detector_other.cpp
	src/applib/storage_detector_other.h
#	src/applib/storage_detector_example.cpp
	src/applib/storage_detector_win32.cpp
	src/applib/storage_detector_win32.h
	src/applib/storage_device.cpp
	src/applib/storage_device.h
	src/applib/storage_property.cpp
	src/applib/storage_property.h
	src/applib/storage_property_colors.h
	src/applib/storage_property_descr.cpp
	src/applib/storage_property_descr.h
	src/applib/storage_settings.h
	src/applib/warning_level.h
	src/hz/bad_cast_exception.h
	src/hz/data_file.h
	src/hz/debug.h
	src/hz/env_tools.h
	src/hz/error.h
	src/hz/error_holder.h
	src/hz/format_unit.h
#	src/hz/format_unit_example.cpp
	src/hz/fs.h
	src/hz/fs_ns.h
	src/hz/instance_manager.h
	src/hz/launch_url.h
	src/hz/locale_tools.h
	src/hz/process_signal.h
	src/hz/scoped_ptr.h
	src/hz/stream_cast.h
	src/hz/string_algo.h
#	src/hz/string_algo_example.cpp
	src/hz/string_num.h
#	src/hz/string_num_example.cpp
	src/hz/string_sprintf.h
	src/hz/system_specific.h
	src/hz/win32_tools.h
	src/libdebug/dchannel.cpp
	src/libdebug/dchannel.h
	src/libdebug/dcmdarg.cpp
	src/libdebug/dcmdarg.h
	src/libdebug/dexcept.h
	src/libdebug/dflags.cpp
	src/libdebug/dflags.h
	src/libdebug/dout.cpp
	src/libdebug/dout.h
	src/libdebug/dstate.cpp
	src/libdebug/dstate.h
	src/libdebug/dstate_pub.h
	src/libdebug/dstream.cpp
	src/libdebug/dstream.h
	src/libdebug/libdebug.h
	src/libdebug/libdebug_mini.h
#	src/libdebug/libdebug_example.cpp
	src/rconfig/autosave.h
	src/rconfig/loadsave.h
	src/rconfig/config.h
#	src/rconfig/rconfig_example.cpp
	src/json/json.hpp
	src/gsc_about_dialog.cpp
	src/gsc_about_dialog.h
	src/gsc_add_device_window.cpp
	src/gsc_add_device_window.h
	src/gsc_executor_error_dialog.cpp
	src/gsc_executor_error_dialog.h
	src/gsc_executor_log_window.cpp
	src/gsc_executor_log_window.h
	src/gsc_info_window.cpp
	src/gsc_info_window.h
	src/gsc_init.cpp
	src/gsc_init.h
	src/gsc_main.cpp
	src/gsc_main_window.cpp
	src/gsc_main_window.h
	src/gsc_main_window_iconview.h
	src/gsc_preferences_window.cpp
	src/gsc_preferences_window.h
	src/gsc_settings.h
	src/gsc_text_window.h)

target_include_directories(gsmartcontrol
	PRIVATE
		src
		${CMAKE_BINARY_DIR}  # for config.h
)

target_compile_definitions(gsmartcontrol
	PRIVATE
		-DENABLE_GLIB=1
		-DENABLE_GLIBMM=1
		-D_GNU_SOURCE
		-DPACKAGE_PKGDATA_DIR=\"/usr/local/share/gsmartcontrol\"
		-DPACKAGE_SYSCONF_DIR=\"/usr/local/etc\"
		-DPACKAGE_LOCALE_DIR=\"/usr/local/share/locale/gsmartcontrol\"
		-DPACKAGE_DOC_DIR=\"/usr/share/doc/packages/gsmartcontrol\"
		-DTOP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
		-DHZ_USE_LIBDEBUG=1
		-DHZ_ENABLE_COMPILED_RES_DATA
		-DDEBUG_BUILD
)

target_link_libraries(gsmartcontrol
	PRIVATE
		${GTKMM_LDFLAGS}
		${PCRECPP_LDFLAGS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_link_libraries(gsmartcontrol PRIVATE c++experimental)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(gsmartcontrol PRIVATE stdc++fs)
endif()
