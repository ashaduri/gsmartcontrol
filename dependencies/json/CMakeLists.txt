###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(json INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(json INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/json.hpp
)

target_include_directories(json
	INTERFACE SYSTEM
		"${CMAKE_SOURCE_DIR}/dependencies"
)

