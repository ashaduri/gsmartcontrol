###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################


# Enable LTO for Release mode
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# Add compiler options for development
add_compile_options(
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wpointer-arith
	-Wundef
	-Wunused-macros
	-Wcast-qual
	-Wcast-align
	-Wconversion
	-Wmissing-declarations
	-Wpacked
	-Wredundant-decls
	-Wvla
	-Woverlength-strings
	-Wnon-virtual-dtor
	-Woverloaded-virtual
	-Wno-missing-field-initializers
	-Wdate-time
	-Wregister
)

# -fvisibility-inlines-hidden hides inline functions of exported classes.
# This switch declares that the user does not attempt to compare pointers to inline functions
# or methods where the addresses of the two functions are taken in different shared objects.
# Dramatically improves load an link times with DSOs (with templates).
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

# -fvisibility=hidden hides all symbols by default (like MSVC); this reduces symbol tables
# for shared libraries and avoids unneeded exports.
add_compile_options(
	-fvisibility=hidden
)

#target_compile_options(foo PUBLIC "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")
