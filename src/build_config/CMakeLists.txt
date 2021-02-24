
# Host system
message(STATUS "CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")

# Target system
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")


# TODO Add these flags

# Note: Some operating systems / compilers have built-in defines.
# For gcc, check with
# $ gcc -dM -E - < /dev/null

# -mms-bitfields (target os env = mingw32, mingw64, cygwin; compiler = gnu, clang)

# gcc warnings: -Wall -Wcast-align -Wcast-qual -Wconversion
#-Wfloat-equal -Wnon-virtual-dtor -Woverloaded-virtual
#-Wpointer-arith -Wshadow -Wsign-compare -Wsign-promo -Wundef -Wwrite-strings

# clang warnings: gcc warnings plus:
# -Wno-sign-conversion -Wno-format-security

# debug builds:
# -DDEBUG=1 -DDEBUG_BUILD=1 -O0 -g3

# optimized builds (target mingw32):
# -g0 -O3 -s -march=i686

# optimized builds (target mingw64, cygwin):
# -g0 -O3 -s

# optimized builds (i686, x86_64 only; others don't have "generic")
# -mtune=generic



# host, target OS kernels (pattern, os name, C macro):
# *linux*, linux, LINUX
# i*86-*mingw* | i*86-*cygwin*, windows32, WINDOWS32
# x86_64-*-mingw* | x86_64-*-cygwin*, windows64, WINDOWS64
# *freebsd*, freebsd, FREEBSD
# *dragonfly*, dragonfly, DRAGONFLY
# *openbsd*, openbsd, OPENBSD
# *netbsd*, netbsd, NETBSD
# *solaris*, solaris, SOLARIS
# *darwin*, darwin, DARWIN
# *qnx*, qnx, QNX
# *, unknown, UNKNOWN

# Notes:
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


# OS environments:
# *-gnu | gnu* (This includes gnu/linux, debian gnu/kfreebsd, debian gnu/netbsd...)
# cygwin
# mingw32
# mingw64
# freebsd (FreeBSD libc)
# dragonfly
# openbsd
# netbsd
# solaris
# darwin
# qnx
# unknown


# MT flags
# -pthread -D_MT -D_THREAD_SAFE (linux/gcc, linux/clang, freebsd, dragonfly)
# -mthreads -D_THREAD_SAFE (win/gcc, win/clang)
# -pthread -D_REENTRANT (openbsd, netbsd)
# -pthreads -D_MT -D_THREAD_SAFE -D_REENTRANT (solaris)
# darwin, qnx - MT enabled by default.


include(CheckSymbolExists)

# getrawpartition() for netbsd and openbsd in -lutil.
check_symbol_exists(getrawpartition "util.h" HAVE_GETRAWPARTITION)



# System-specific definitions
add_library(build_config INTERFACE)
configure_file("config.in.h" "config.h" ESCAPE_QUOTES @ONLY)
target_sources(build_config INTERFACE
	config.in.h
)
target_include_directories(build_config INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

