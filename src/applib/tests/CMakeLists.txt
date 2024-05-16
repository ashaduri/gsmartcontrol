###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 - 2024 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT APP_BUILD_TESTS)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


# Use Object libraries to allow runtime test discovery
add_library(applib_tests OBJECT)
target_sources(applib_tests PRIVATE
	test_app_pcrecpp.cpp
	test_app_regex.cpp
	test_smartctl_parser.cpp
	test_smartctl_version_parser.cpp
)
target_link_libraries(applib_tests PRIVATE
	applib
	Catch2
)

