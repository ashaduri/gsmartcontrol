###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2026 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT APP_BUILD_TESTS)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


# Use Object libraries to allow runtime test discovery
add_library(gui_tests OBJECT)
target_sources(gui_tests PRIVATE
	test_fractional_scaling.cpp
)
target_link_libraries(gui_tests PRIVATE
	Catch2
)
