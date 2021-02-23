
# Linux development version with gcc, debug mode.
# Use with:
# cmake -DCMAKE_TOOLCHAIN_FILE=...

set(CMAKE_SYSTEM_NAME Linux)

# Specify the compiler
set(CMAKE_C_COMPILER gcc-9)
set(CMAKE_CXX_COMPILER g++-9)

# Common options for gcc/clang
include(${CMAKE_CURRENT_LIST_DIR}/common-gcc-clang.cmake)

# Common options for development
include(${CMAKE_CURRENT_LIST_DIR}/common-dev.cmake)

# gcc-specific options
add_compile_options(
	-Wnoexcept
	-Wsuggest-attribute=const
	-Wsuggest-attribute=noreturn
	-Wsuggest-attribute=format
	-Wsuggest-final-methods
	-Wsuggest-override
	-Wno-virtual-move-assign
	-Wdate-time
	-Wtrampolines
	-Wlogical-op
	-Wduplicated-cond
	-Wnormalized
	-Wextra-semi
	-Wcatch-value
	-Wno-virtual-move-assign
)

