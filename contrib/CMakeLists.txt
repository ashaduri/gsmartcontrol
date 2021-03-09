###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

if (NOT WIN32)
	install(DIRECTORY "cron-based_noadmin" DESTINATION "${CMAKE_INSTALL_DOCDIR}/gsmartcontrol/contrib/")
endif()
