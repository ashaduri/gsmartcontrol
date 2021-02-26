###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(json INTERFACE)

target_sources(json INTERFACE
	json.hpp
)

target_include_directories(json
	INTERFACE SYSTEM
		"${CMAKE_SOURCE_DIR}/dependencies"
)

