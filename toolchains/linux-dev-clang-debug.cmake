###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# Linux development version with clang, debug mode.
# Use with:
# cmake -DCMAKE_TOOLCHAIN_FILE=...

set(CMAKE_SYSTEM_NAME Linux)

# Specify the compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# Common options for gcc/clang
include(${CMAKE_CURRENT_LIST_DIR}/common-gcc-clang.cmake)

# Common options for development
include(${CMAKE_CURRENT_LIST_DIR}/common-dev.cmake)

# Clang-specific options
add_compile_options(
	-Wdocumentation
	-Wheader-guard
	-Wloop-analysis
	-Wno-keyword-macro
)


# Enable clang-tidy by default, reading from <project_root>/.clang-tidy file
set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
