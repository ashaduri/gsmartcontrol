###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2025 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(fmt STATIC)

# Relative sources are allowed only since cmake 3.13.
target_sources(fmt PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/src/format.cc
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/args.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/base.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/chrono.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/color.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/compile.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/core.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/format.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/format-inl.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/os.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/ostream.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/printf.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/ranges.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/std.h
	${CMAKE_CURRENT_SOURCE_DIR}/fmt/include/fmt/xchar.h
)

target_include_directories(fmt
	SYSTEM PUBLIC
		"fmt/include"
)

