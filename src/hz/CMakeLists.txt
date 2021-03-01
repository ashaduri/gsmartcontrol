###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(hz INTERFACE)

target_sources(hz INTERFACE
	bad_cast_exception.h
	data_file.h
	debug.h
	env_tools.h
	error.h
	error_holder.h
	format_unit.h
	fs.h
	fs_ns.h
	instance_manager.h
	launch_url.h
	locale_tools.h
	process_signal.h
	stream_cast.h
	string_algo.h
	string_num.h
	string_sprintf.h
	system_specific.h
	win32_tools.h
)

target_link_libraries(hz
	INTERFACE
#		libdebug
		app_gtkmm_interface  # ENABLE_* macros
#		app_pcrecpp_interface
		app_gettext_interface  # format_unit.h uses this
		libdebug  # debug.h
)

# Support <experimental/filesystem> if needed
#if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#	target_link_libraries(hz
#		INTERFACE
#			stdc++fs
#	)
#endif()

target_include_directories(hz
	INTERFACE
		"${CMAKE_SOURCE_DIR}/src"
)

target_compile_definitions(hz
	INTERFACE
		HZ_USE_LIBDEBUG=1
)


add_subdirectory(examples)

