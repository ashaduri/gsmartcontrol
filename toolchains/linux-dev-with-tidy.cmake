###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################


# Developer options
set(APP_COMPILER_ENABLE_WARNINGS ON)
set(APP_BUILD_EXAMPLES ON)
set(APP_BUILD_TESTS ON)


# Enable clang-tidy by default, reading from <project_root>/.clang-tidy file
set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

