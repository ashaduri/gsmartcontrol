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


add_executable(test_all)
target_sources(test_all PRIVATE
	test_all.cpp
)
target_link_libraries(test_all PRIVATE
	libdebug
	applib_tests
	Catch2
)

if (NOT CMAKE_CROSSCOMPILING)
	catch_discover_tests(test_all)
endif()
