###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT WIN32)
	install(DIRECTORY "${CMAKE_SOURCE_DIR}/contrib" TYPE DOC
		PATTERN "CMakeLists.txt" EXCLUDE)
endif()
