###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# Windows 32-bit production version with mingw/gcc.
# Use with:
# cmake -DCMAKE_TOOLCHAIN_FILE=...

# Target OS
set(CMAKE_SYSTEM_NAME Windows)

# Specify the compiler
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
#set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

# The target environment
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32/sys-root/mingw)

# Adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

IF (CMAKE_CROSSCOMPILING)
	set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_FIND_ROOT_PATH}/lib/pkgconfig")
endif()

#set(APP_NSIS_PATH /usr/bin/makensis)


# Enable LTO for Release build
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# Increase optimizations of Release builds
add_compile_options("$<$<CONFIG:RELEASE>:-g0 -O3 -s>")

# Enable common CPU optimizations
add_compile_options(-march=i686 -mtune=generic)


# Developer options
set(APP_COMPILER_ENABLE_WARNINGS ON)
set(APP_BUILD_EXAMPLES ON)
set(APP_BUILD_TESTS ON)


