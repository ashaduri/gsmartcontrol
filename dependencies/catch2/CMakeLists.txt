###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

#add_subdirectory(Catch2)


add_library(Catch2 INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(Catch2 INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/Catch2/single_include/catch2/catch.hpp
)

target_include_directories(Catch2
	INTERFACE SYSTEM
		"${CMAKE_SOURCE_DIR}/dependencies/catch2/Catch2/single_include"
)


