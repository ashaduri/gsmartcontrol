###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# We print these here (as opposed to compiler_options.cmake) because it's the final state of these variables.

# Host system
message(STATUS "Host system:")
message(STATUS "CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "CMAKE_HOST_SYSTEM_VERSION: ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

# Target system
message(STATUS "Target system:")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SIZEOF_VOID_P: ${CMAKE_SIZEOF_VOID_P}")

# Compiler
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN: ${CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN}")
message(STATUS "CMAKE_CXX_COMPILER_TARGET: ${CMAKE_CXX_COMPILER_TARGET}")
message(STATUS "CMAKE_CXX_COMPILE_FEATURES: ${CMAKE_CXX_COMPILE_FEATURES}")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

message(STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CMAKE_STATIC_LINKER_FLAGS}")
message(STATUS "CMAKE_STATIC_LINKER_FLAGS_DEBUG: ${CMAKE_STATIC_LINKER_FLAGS_DEBUG}")
message(STATUS "CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL: ${CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_STATIC_LINKER_FLAGS_RELEASE: ${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")
message(STATUS "CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO: ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO}")

message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS_DEBUG: ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL: ${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS_RELEASE: ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO: ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO}")

message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS_DEBUG: ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS_MINSIZEREL: ${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS_RELEASE: ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO: ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")

# Suppress warning about manually specified variables not being used (this is a very common variable)
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")


# These variables are used in build_config.in.h
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows"
		OR "${CMAKE_SYSTEM_NAME}" MATCHES "CYGWIN.*")
	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CONFIG_KERNEL_WINDOWS32 TRUE)
	else()
		set(CONFIG_KERNEL_WINDOWS64 TRUE)
	endif()
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(CONFIG_KERNEL_LINUX TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD"
		OR "${CMAKE_SYSTEM_NAME}" MATCHES "GNU/kFreeBSD")
	set(CONFIG_KERNEL_FREEBSD TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "DragonFly")
	set(CONFIG_KERNEL_DRAGONFLY TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "NetBSD")
	set(CONFIG_KERNEL_NETBSD TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "OpenBSD")
	set(CONFIG_KERNEL_OPENBSD TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS" OR "${CMAKE_SYSTEM_NAME}" MATCHES "Solaris")
	set(CONFIG_KERNEL_SOLARIS TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
	set(CONFIG_KERNEL_DARWIN TRUE)
elseif ("${CMAKE_SYSTEM_NAME}" MATCHES "QNX")
	set(CONFIG_KERNEL_QNX TRUE)
endif()


# Additionally, these macros are defined by OS / compilers:

# Linux: gcc defines __linux__ on linux. Other defines include: __linux, __unix__, __gnu_linux__,
# linux, unix, __i386__, i386.

# Windows 32: mingw defines _WIN32. msvc defines _WIN32 (built-in), and WIN32 via windows.h.
# Other mingw defines include: __WINNT, __WINNT__, __WIN32__, __i386, _X86_,
# i386, __i386__, WIN32, __MINGW32__, WINNT (all equal to 1).

# Windows 64: mingw64 defines the same stuff as 32-bit one, plus _WIN64, __MINGW64__, etc... .
# Keep in mind that if you're generating a 32-bit application, the kernel will
# be windows32 even if you run it on 64-bit Windows.

# FreeBSD: gcc built-in defines (to check for freebsd kernel):
# defined(__FreeBSD__) || defined(__FreeBSD_kernel__).
# See http://glibc-bsd.alioth.debian.org/porting/PORTING .



#include(CheckSymbolExists)

# getrawpartition() for netbsd and openbsd in -lutil.
# This is just a check to see whether we have it. The code uses getrawpartition() without any checks on *BSD.
#check_symbol_exists(getrawpartition "util.h" HAVE_GETRAWPARTITION)
#message(STATUS "getrawpartition() detected: ${HAVE_GETRAWPARTITION}")


# System-specific definitions
add_library(build_config INTERFACE)
configure_file("build_config.in.h" "build_config.h" ESCAPE_QUOTES @ONLY)
target_sources(build_config INTERFACE
	${CMAKE_CURRENT_BINARY_DIR}/build_config.h
)
target_include_directories(build_config INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

