###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT APP_BUILD_EXAMPLES)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


add_executable(example_format_unit)
target_sources(example_format_unit PRIVATE
	example_format_unit.cpp
)
target_link_libraries(example_format_unit PRIVATE
	hz
)


add_executable(example_string_algo)
target_sources(example_string_algo PRIVATE
	example_string_algo.cpp
)
target_link_libraries(example_string_algo PRIVATE
	hz
)


add_executable(example_string_num)
target_sources(example_string_num PRIVATE
	example_string_num.cpp
)
target_link_libraries(example_string_num PRIVATE
	hz
)
