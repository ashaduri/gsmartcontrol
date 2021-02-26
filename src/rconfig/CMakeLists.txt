###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(rconfig INTERFACE)

target_sources(rconfig INTERFACE
	autosave.h
	loadsave.h
	rconfig.h
)

target_link_libraries(rconfig
	INTERFACE
		hz
		json
)


add_subdirectory(examples)

