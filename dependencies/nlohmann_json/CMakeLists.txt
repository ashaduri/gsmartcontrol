###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(nlohmann_json INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(nlohmann_json INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/json/json.hpp
)

target_include_directories(nlohmann_json
	INTERFACE SYSTEM
		"${CMAKE_SOURCE_DIR}/dependencies/nlohmann_json"
)

