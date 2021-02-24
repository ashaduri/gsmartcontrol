
# Generate files
configure_file("nsis/distribution.in.txt" "nsis/distribution.txt" ESCAPE_QUOTES @ONLY)
configure_file("nsis/gsmartcontrol.in.nsi" "nsis/gsmartcontrol.nsi" ESCAPE_QUOTES @ONLY)

configure_file("gsmartcontrol.appdata.in.xml" "gsmartcontrol.appdata.xml" ESCAPE_QUOTES @ONLY)
configure_file("gsmartcontrol.in.desktop" "gsmartcontrol.desktop" ESCAPE_QUOTES @ONLY)
configure_file("gsmartcontrol-root.in.sh" "gsmartcontrol-root.sh" ESCAPE_QUOTES @ONLY)

configure_file("debian-postinst.in.sh" "${CMAKE_BINARY_DIR}/postinst" ESCAPE_QUOTES @ONLY)
configure_file("debian-postrm.in.sh" "${CMAKE_BINARY_DIR}/postrm" ESCAPE_QUOTES @ONLY)

configure_file("rpm-post-install.in.sh" "${CMAKE_BINARY_DIR}/rpm-post-install.sh" ESCAPE_QUOTES @ONLY)
configure_file("rpm-post-uninstall.in.sh" "${CMAKE_BINARY_DIR}/rpm-post-uninstall.sh" ESCAPE_QUOTES @ONLY)

# Install app icons
foreach(dir 16 22 24 32 48 64 128 256)
	install(FILES "${dir}/gsmartcontrol.png"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${dir}x${dir}/apps/")
endforeach()
install(CODE "execute_process (COMMAND gtk-update-icon-cache-3.0 -t -f \"@CMAKE_INSTALL_DATADIR@/icons/hicolor\")")

# Application pixmap (fallback icon for desktop files).
# Same as 48/gsmartcontrol.png.
# Also, xpm for debian menu, 32x32.
install(FILES 48/gsmartcontrol.png gsmartcontrol.xpm
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps/")

# We don't install .ico (it's only for Windows)

# Install internal icons
install(FILES icon_cddvd.png icon_hdd.png
	DESTINATION "${CMAKE_INSTALL_DATADIR}/gsmartcontrol/icons")

# Desktop file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.desktop"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/gsmartcontrol/icons/")

# Appdata file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.appdata.xml"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo/")

# PolKit file
install(FILES "org.gsmartcontrol.policy"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/polkit-1/actions/")

# Man pages
install(FILES "gsmartcontrol.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1/")
install(FILES "gsmartcontrol.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1/" RENAME "gsmartcontrol-root.1")

# Scripts (this goes to bin, not sbin, as it doesn't require root privileges before running)
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol-root.sh" TYPE BIN RENAME "gsmartcontrol-root")


