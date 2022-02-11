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


# Use Object libraries to allow runtime test discovery
add_library(hz_tests OBJECT)
target_sources(hz_tests PRIVATE
	test_format_unit.cpp
	test_string_num.cpp
)
target_link_libraries(hz_tests PRIVATE
	hz
	Catch2
)

