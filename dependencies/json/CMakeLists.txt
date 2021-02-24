
add_library(json INTERFACE)

target_sources(json INTERFACE
	json.hpp
)

target_include_directories(json
	INTERFACE SYSTEM
		"${CMAKE_SOURCE_DIR}/dependencies"
)

