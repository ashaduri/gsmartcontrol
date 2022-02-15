###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT APP_BUILD_TESTS)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()

# Make sure include() works with our modules.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/dependencies/catch2/Catch2/contrib")
include(ParseAndAddCatchTests)

add_executable(test_all)
target_sources(test_all PRIVATE
	test_all.cpp
)
target_link_libraries(test_all PRIVATE
	libdebug
	Catch2
	# Object libraries containing tests
	applib_tests
	hz_tests
)

if (NOT CMAKE_CROSSCOMPILING)
	catch_discover_tests(test_all)
endif()
