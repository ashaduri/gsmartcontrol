###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2022 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT APP_BUILD_TESTS)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


add_library(test_helpers INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(test_helpers INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/test_helpers.h
)

target_include_directories(test_helpers INTERFACE "${CMAKE_SOURCE_DIR}/src")
