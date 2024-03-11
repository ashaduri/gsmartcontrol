###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(hz INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(hz INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/bad_cast_exception.h
	${CMAKE_CURRENT_SOURCE_DIR}/data_file.h
	${CMAKE_CURRENT_SOURCE_DIR}/debug.h
	${CMAKE_CURRENT_SOURCE_DIR}/enum_helper.h
	${CMAKE_CURRENT_SOURCE_DIR}/env_tools.h
	${CMAKE_CURRENT_SOURCE_DIR}/error_container.h
	${CMAKE_CURRENT_SOURCE_DIR}/error_holder.h
	${CMAKE_CURRENT_SOURCE_DIR}/format_unit.h
	${CMAKE_CURRENT_SOURCE_DIR}/fs.h
	${CMAKE_CURRENT_SOURCE_DIR}/fs_ns.h
	${CMAKE_CURRENT_SOURCE_DIR}/launch_url.h
	${CMAKE_CURRENT_SOURCE_DIR}/locale_tools.h
	${CMAKE_CURRENT_SOURCE_DIR}/main_tools.h
	${CMAKE_CURRENT_SOURCE_DIR}/process_signal.h
	${CMAKE_CURRENT_SOURCE_DIR}/stream_cast.h
	${CMAKE_CURRENT_SOURCE_DIR}/string_algo.h
	${CMAKE_CURRENT_SOURCE_DIR}/string_num.h
	${CMAKE_CURRENT_SOURCE_DIR}/string_sprintf.h
	${CMAKE_CURRENT_SOURCE_DIR}/system_specific.h
	${CMAKE_CURRENT_SOURCE_DIR}/win32_tools.h
)

target_link_libraries(hz
	INTERFACE
#		libdebug
		app_gtkmm_interface  # ENABLE_* macros
#		app_pcrecpp_interface
		app_gettext_interface  # format_unit.h uses this
		libdebug  # debug.h
		whereami  # whereami.h
		std::filesystem  # fs_ns.h, linking to stdc++fs / c++fs if needed
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


add_subdirectory(tests)

