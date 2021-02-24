
if (NOT APP_BUILD_EXAMPLES)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


add_executable(rconfig_example)
target_sources(rconfig_example PRIVATE
	rconfig_example.cpp
)
target_link_libraries(rconfig_example PRIVATE
	rconfig
	compiler_warnings
)

