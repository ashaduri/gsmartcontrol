
# Windows 32-bit production version with mingw/gcc.
# Use with:
# cmake -DCMAKE_TOOLCHAIN_FILE=...

set(CMAKE_SYSTEM_NAME Windows)

# Specify the compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# The target environment
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32/sys-root/mingw)

# Adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Common options for gcc
include(${CMAKE_CURRENT_LIST_DIR}/common-gcc-clang.cmake)

set(APP_NSIS_PATH /usr/bin/makensis)

