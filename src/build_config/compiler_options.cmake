###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# This file is included from root CMakeLists.txt

# Set the default C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# We need OS/compiler extensions, if available.

# glibc: all-extensions macro: _GNU_SOURCE.
# Defined automatically when used with glibc, but not others.

# Solaris: The problem with solaris is that it has too many incompatible
# feature test macros, and there is no enable-all macro. Autoconf
# assumes that __EXTENSIONS__ is the one, but it should be defined
# _in addition_ to feature test macros. Note that on its own, __EXTENSIONS__
# still enables some useful stuff like _LARGEFILE64_SOURCE.
# To support some posix macros, additional link objects are also required.
# Some *_SOURCE feature test macros may be incompatible with C++.
# http://docs.sun.com/app/docs/doc/816-5175/standards-5?a=view

# Mingw: This enables standards-compliant stdio behaviour (regarding printf and
# friends), as opposed to msvc-compatible one. This is usually enabled
# by default if one of the usual macros are encountered (_XOPEN_SOURCE,
# _GNU_SOURCE, etc...).
# See _mingw.h for details.
# TODO
# -D__USE_MINGW_ANSI_STDIO=1 -DWINVER=0x0600

# Darwin: Enable large file support
# -D_DARWIN_USE_64_BIT_INODE=1

# In many environments this enables large file support
# -D_FILE_OFFSET_BITS=64

set(CMAKE_CXX_EXTENSIONS ON)

#set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# Add various OS/compiler-specific options that should always be there
# regardless of the machine.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_definitions(_GNU_SOURCE)
endif()


# Enable warnings
add_library(compiler_warnings INTERFACE)
target_compile_options(compiler_warnings INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)
