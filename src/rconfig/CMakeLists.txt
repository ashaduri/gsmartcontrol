###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(rconfig INTERFACE)

# Relative sources are allowed only since cmake 3.13.
target_sources(rconfig INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/autosave.h
	${CMAKE_CURRENT_SOURCE_DIR}/loadsave.h
	${CMAKE_CURRENT_SOURCE_DIR}/rconfig.h
)

target_include_directories(rconfig INTERFACE "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(rconfig
	INTERFACE
		hz
		nlohmann_json
		app_gtkmm_interface
)


add_subdirectory(examples)

