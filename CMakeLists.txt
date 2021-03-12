###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# 3.12 for CMAKE_PROJECT_VERSION
cmake_minimum_required(VERSION 3.12)

include(${CMAKE_SOURCE_DIR}/version.txt)

project(gsmartcontrol
	VERSION ${CMAKE_PROJECT_VERSION}
	DESCRIPTION "Hard Disk Drive and SSD Health Inspection Tool"
	HOMEPAGE_URL "https://gsmartcontrol.shaduri.dev"
	LANGUAGES CXX
)


# Provide DATADIR, etc...
include(GNUInstallDirs)

message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

include(src/build_config/compiler_options.cmake)


# Disable RPATH manipulation, we don't have shared libraries
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_SKIP_INSTALL_RPATH TRUE)

# Write the binaries to project root (avoids conflicts with manifests, also it's more convenient this way)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")


# User-controlled build options
option(APP_BUILD_EXAMPLES "Build examples" OFF)
option(APP_BUILD_TESTS "Build tests" OFF)


# Install documentation
set(DOC_FILES
	LICENSE.LGPL3.txt
	LICENSE.txt
	NEWS.txt
	README.md
)
if (WIN32)
	install(FILES ${DOC_FILES} DESTINATION doc/)
	install(DIRECTORY docs DESTINATION doc/
		PATTERN "*.yml" EXCLUDE
		PATTERN "CNAME" EXCLUDE
	)
else()
	install(FILES ${DOC_FILES} DESTINATION "${CMAKE_INSTALL_DOCDIR}/")
	install(DIRECTORY docs DESTINATION "${CMAKE_INSTALL_DOCDIR}/"
		PATTERN "*.yml" EXCLUDE
		PATTERN "CNAME" EXCLUDE
	)
endif()


add_subdirectory(contrib)
add_subdirectory(data)
add_subdirectory(dependencies)
add_subdirectory(packaging)
add_subdirectory(src)

include(packaging/cpack_options.cmake)

