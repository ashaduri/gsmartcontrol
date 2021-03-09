
# Variables for nsis .in files
set(WINDOWS_SUFFIX "win32")
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(WINDOWS_SUFFIX "win64")
endif()
configure_file("nsis/distribution.in.txt" "nsis/distribution.txt" ESCAPE_QUOTES @ONLY NEWLINE_STYLE DOS)
configure_file("nsis/gsmartcontrol.in.nsi" "nsis/gsmartcontrol.nsi" ESCAPE_QUOTES @ONLY NEWLINE_STYLE DOS)

# NSIS helpers
if (WIN32)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nsis/distribution.txt" DESTINATION doc/)

	# nsis file, only for cross-compilation
	if (NOT ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nsis/gsmartcontrol.nsi" DESTINATION .)
	endif()
endif()


# CPack Debian
configure_file("cpack_debian/debian-postinst.in.sh" "${CMAKE_CURRENT_BINARY_DIR}/cpack_debian/postinst" ESCAPE_QUOTES @ONLY)
configure_file("cpack_debian/debian-postrm.in.sh" "${CMAKE_CURRENT_BINARY_DIR}/cpack_debian/postrm" ESCAPE_QUOTES @ONLY)

# CPack RPM
configure_file("cpack_rpm/rpm-post-install.in.sh" "${CMAKE_CURRENT_BINARY_DIR}/cpack_rpm/rpm-post-install.sh" ESCAPE_QUOTES @ONLY)
configure_file("cpack_rpm/rpm-post-uninstall.in.sh" "${CMAKE_CURRENT_BINARY_DIR}/cpack_rpm/rpm-post-uninstall.sh" ESCAPE_QUOTES @ONLY)

# OBS Debian
configure_file("obs_debian/changelog.in" "obs_debian/changelog" ESCAPE_QUOTES @ONLY)

# OBS RPM
#configure_file(obs_rpm/gsmartcontrol.in.spec obs_rpm/gsmartcontrol.spec)

