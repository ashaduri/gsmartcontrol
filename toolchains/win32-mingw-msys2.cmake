###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2024 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# Windows 32-bit production version with mingw/msys2.
# Use with:
# cmake -DCMAKE_TOOLCHAIN_FILE=...

# Target OS
set(CMAKE_SYSTEM_NAME Windows)

# Specify the compiler
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
#set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

# The target environment. This is a path which is valid for msys2.
set(CMAKE_FIND_ROOT_PATH "$ENV{MINGW_PREFIX}")

# Enable LTO for Release build (does not work with mingw)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# Increase optimizations of Release builds
add_compile_options("$<$<CONFIG:RELEASE>:-g0 -O3 -s>")

# Enable common CPU optimizations
add_compile_options(-march=i686 -mtune=generic)


# Developer options
set(APP_COMPILER_ENABLE_WARNINGS ON)
set(APP_BUILD_EXAMPLES ON)
set(APP_BUILD_TESTS ON)


