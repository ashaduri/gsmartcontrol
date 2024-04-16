###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2024 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################


add_library(tl_expected INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(tl_expected INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/expected/include/tl/expected.hpp
)

target_include_directories(tl_expected
	SYSTEM INTERFACE
		"${CMAKE_SOURCE_DIR}/dependencies/tl_expected/expected/include"
)


