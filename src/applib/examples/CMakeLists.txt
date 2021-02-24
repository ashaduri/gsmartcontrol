
if (NOT APP_BUILD_EXAMPLES)
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL true)
else()
    set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL false)
endif()


add_executable(smartctl_executor_example)
target_sources(smartctl_executor_example PRIVATE
	smartctl_executor_example.cpp
)
target_link_libraries(smartctl_executor_example PRIVATE
	applib
	compiler_warnings
)


add_executable(smartctl_parser_example)
target_sources(smartctl_parser_example PRIVATE
	smartctl_parser_example.cpp
)
target_link_libraries(smartctl_parser_example PRIVATE
	applib
	compiler_warnings
)


add_executable(storage_detector_example)
target_sources(storage_detector_example PRIVATE
	storage_detector_example.cpp
)
target_link_libraries(storage_detector_example PRIVATE
	applib
	compiler_warnings
)
