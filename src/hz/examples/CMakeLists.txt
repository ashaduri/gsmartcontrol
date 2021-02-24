
if (NOT APP_BUILD_EXAMPLES)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


add_executable(format_unit_example)
target_sources(format_unit_example PRIVATE
	format_unit_example.cpp
)
target_link_libraries(format_unit_example PRIVATE
	hz
	compiler_warnings
)


add_executable(string_algo_example)
target_sources(string_algo_example PRIVATE
	string_algo_example.cpp
)
target_link_libraries(string_algo_example PRIVATE
	hz
	compiler_warnings
)


add_executable(string_num_example)
target_sources(string_num_example PRIVATE
	string_num_example.cpp
)
target_link_libraries(string_num_example PRIVATE
	hz
	compiler_warnings
)
