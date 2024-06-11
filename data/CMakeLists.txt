###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################


# Generate files
configure_file("gsmartcontrol.appdata.in.xml" "gsmartcontrol.appdata.xml" ESCAPE_QUOTES @ONLY)
configure_file("gsmartcontrol.in.desktop" "gsmartcontrol.desktop" ESCAPE_QUOTES @ONLY)
configure_file("gsmartcontrol-root.in.sh" "gsmartcontrol-root.sh" ESCAPE_QUOTES @ONLY)
configure_file("org.gsmartcontrol.in.policy" "org.gsmartcontrol.policy" ESCAPE_QUOTES @ONLY)

# Install app icons
if (NOT WIN32)
	foreach(dir 16 22 24 32 48 64 128 256)
		install(FILES "${dir}/gsmartcontrol.png"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${dir}x${dir}/apps/")
	endforeach()
	# Don't run update-icon-cache, cpack is mostly for packaging.
#	install(CODE "execute_process (COMMAND gtk-update-icon-cache-3.0 -t -f \"@CMAKE_INSTALL_DATADIR@/icons/hicolor\")")
endif()

if (WIN32)
	# .ico file
	install(FILES gsmartcontrol.ico DESTINATION .)
endif()

# Install internal icons
if (WIN32)
	install(FILES icon_optical.png icon_harddisk.png
		DESTINATION .)
else()
	install(FILES icon_optical.png icon_harddisk.png
		DESTINATION "${CMAKE_INSTALL_DATADIR}/gsmartcontrol/icons")
endif()

# Desktop file
if (NOT WIN32)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.desktop"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/")

	# Appdata file
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.appdata.xml"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo/")

	# PolKit file
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.gsmartcontrol.policy"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/polkit-1/actions/")

	# Man pages
	install(FILES "man1/gsmartcontrol.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
	install(FILES "man1/gsmartcontrol.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1" RENAME "gsmartcontrol-root.1")

	# Scripts (this goes to bin, not sbin, as it doesn't require root privileges before running)
	install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol-root.sh" TYPE BIN RENAME "gsmartcontrol-root")
endif()

