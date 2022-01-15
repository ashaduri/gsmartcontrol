###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2022 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(whereami STATIC)

target_sources(whereami PRIVATE
	whereami/src/whereami.cpp
	whereami/src/whereami.h
)

target_include_directories(whereami
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/whereami/src"
)

