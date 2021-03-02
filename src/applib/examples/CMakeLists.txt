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


add_executable(example_smartctl_executor)
target_sources(example_smartctl_executor PRIVATE
	example_smartctl_executor.cpp
)
target_link_libraries(example_smartctl_executor PRIVATE
	applib
)


add_executable(example_smartctl_parser)
target_sources(example_smartctl_parser PRIVATE
	example_smartctl_parser.cpp
)
target_link_libraries(example_smartctl_parser PRIVATE
	applib
)


add_executable(example_spawn)
target_sources(example_spawn PRIVATE
	example_spawn.cpp
)
target_link_libraries(example_spawn PRIVATE
	applib
)


add_executable(example_storage_detector)
target_sources(example_storage_detector PRIVATE
	example_storage_detector.cpp
)
target_link_libraries(example_storage_detector PRIVATE
	applib
)
