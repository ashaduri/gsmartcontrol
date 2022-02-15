###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2022 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(boost_leaf INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(boost_leaf INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/standalone/leaf.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/convenience/leaf_ns.h
)

target_include_directories(boost_leaf
	SYSTEM INTERFACE
		"${CMAKE_SOURCE_DIR}/dependencies/leaf/standalone"
		"${CMAKE_SOURCE_DIR}/dependencies/leaf/convenience"
)

