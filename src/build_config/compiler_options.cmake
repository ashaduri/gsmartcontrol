###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# This file is included from root CMakeLists.txt

# --- Set the default C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Enable PIC, required on many systems
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# --- OS / compiler extensions

# These extensions are useful to enable OS-specific functionality like 64-bit file offsets.

# Enable compiler extensions (like -std=gnu++17 instead of -std=c++17)
set(CMAKE_CXX_EXTENSIONS ON)

# glibc: all-extensions macro: _GNU_SOURCE.
# Defined automatically when used with glibc, but not others.
# We define it for all gcc systems.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_definitions(_GNU_SOURCE)
endif()

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
if (WIN32)
	# No effect in MSVC, doesn't hurt.
	add_compile_definitions(__USE_MINGW_ANSI_STDIO=1)

	# Enable Vista winapi
	add_compile_definitions(WINVER=0x0600)
endif()

# Darwin: Enable large file support
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
	add_compile_definitions(_DARWIN_USE_64_BIT_INODE=1)
endif()

# In many environments this enables large file support. For others, it's harmless.
add_compile_definitions(_FILE_OFFSET_BITS=64)



# --- Compiler flags

# Note: Some operating systems / compilers have built-in defines.
# For gcc, check with
# $ gcc -dM -E - < /dev/null


# MT flags
# -pthread -D_MT -D_THREAD_SAFE (linux/gcc, linux/clang, freebsd, dragonfly)
# -mthreads -D_THREAD_SAFE (win/gcc, win/clang)
# -pthread -D_REENTRANT (openbsd, netbsd)
# -pthreads -D_MT -D_THREAD_SAFE -D_REENTRANT (solaris)
# darwin, qnx - MT enabled by default.


# Enable compiler warnings
if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang
		OR ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU
		OR ${CMAKE_CXX_COMPILER_ID} STREQUAL AppleClang)
	add_compile_options(-Wall -Wextra
		-Wcast-qual -Wconversion -Wfloat-equal -Wnon-virtual-dtor -Woverloaded-virtual
		-Wpointer-arith -Wshadow -Wsign-compare -Wsign-promo -Wundef -Wwrite-strings
	)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	add_compile_options(/W4)
endif()


# GTK uses MS bitfields, so we need this in mingw
if (MINGW)
	add_compile_options(-mms-bitfields)
endif()

# Define macros to check the debug build
add_compile_definitions("$<$<CONFIG:DEBUG>:DEBUG>")
add_compile_definitions("$<$<CONFIG:DEBUG>:DEBUG_BUILD>")

