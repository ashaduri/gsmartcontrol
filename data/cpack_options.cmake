###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# CPack options for this project

# Generators supported:
# Linux: TBZ2 RPM DEB (possibly other archive-based ones as well)
# Windows: NSIS ZIP.

# Per-generator settings
#set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/data/cmake/cpack_project_config.cmake")

if (UNIX)
	# Make the "package" target build .tar.bz2
	set(CPACK_GENERATOR "TBZ2")
endif()

if (WIN32)
	# Make the "package" target build all supported packages (nsis and zip)
	set(CPACK_GENERATOR "")
endif()

#set(APP_PACKAGE_NAME_DISPLAY "${APP_BRANDING_APP_DISPLAY_NAME}")
#if (WIN32 AND APP_TARGET_64BIT)
#	set(APP_PACKAGE_NAME_DISPLAY "${APP_PACKAGE_NAME_DISPLAY} (64-Bit)")
#endif()

#set(CPACK_PACKAGE_NAME "${APP_BRANDING_INTERNAL_APP_NAME}")
#if (WIN32)
	# We set it to display name so that we get a different name for 32-bit version.
	# Default value is "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}".
#	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${APP_PACKAGE_NAME_DISPLAY}")  # appended to e.g. CPACK_NSIS_INSTALL_ROOT
#	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
#endif()

#set(CPACK_PACKAGE_VENDOR "${APP_BRANDING_COPYRIGHT_NAME}")

#set(CPACK_PACKAGE_VERSION_MAJOR ${APP_VERSION_MAJOR})
#set(CPACK_PACKAGE_VERSION_MINOR ${APP_VERSION_MINOR})
#set(CPACK_PACKAGE_VERSION_PATCH ${APP_VERSION_PATCHLEVEL})

# Used by wix only?
#set(APP_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/data/${APP_BRANDING_INTERNAL_APP_NAME}/package_description.txt")
#if (EXISTS "${APP_PACKAGE_DESCRIPTION_FILE}")
#	set(CPACK_PACKAGE_DESCRIPTION_FILE "${APP_PACKAGE_DESCRIPTION_FILE}")
#endif()

#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_BRANDING_DESCRIPTION}")

#set(CPACK_SYSTEM_NAME "${APP_TARGET_SYSTEM_NAME}")


#if (WIN32)
	# Don't use CPACK_PACKAGE_NAME
#	set(CPACK_PACKAGE_FILE_NAME "${APP_BRANDING_INTERNAL_APP_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_SYSTEM_NAME}")
#endif()

#set(APP_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/data/brandings/${APP_BRANDING_INTERNAL_APP_NAME}/eula.rtf")
#if (EXISTS "${APP_RESOURCE_FILE_LICENSE}")
#	set(CPACK_RESOURCE_FILE_LICENSE "${APP_RESOURCE_FILE_LICENSE}")
#endif()
#set(CPACK_STRIP_FILES 0)
#set(CPACK_SOURCE_STRIP_FILES 0)

# Icon in Add/Remove Programs
#if (EXISTS "${CMAKE_SOURCE_DIR}/data/brandings/${APP_BRANDING_INTERNAL_APP_NAME}/app_icon.ico")
#	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\data\\\\brandings\\\\${APP_BRANDING_INTERNAL_APP_NAME}\\\\app_icon.ico")
#endif()

# Start menu shortcut
#set(CPACK_PACKAGE_EXECUTABLES "${APP_BRANDING_INTERNAL_APP_NAME};${APP_PACKAGE_NAME_DISPLAY}")

# Force monolithic installers (ignore per-component stuff).
# We don't want this because we want to avoid packing system_dlls component
# in NSIS-based installer.
# set(CPACK_MONOLITHIC_INSTALL true)

#SET(CPACK_SOURCE_IGNORE_FILES
#	"0*"  # build dirs
#	".*/\\\\.git/.*"  # svn caches
#	".*~$"  # editor temporary files
#)



# --- NSIS options

#set(CPACK_NSIS_DISPLAY_NAME "${APP_PACKAGE_NAME_DISPLAY}")  # used in add/remove programs as a display text
#set(CPACK_NSIS_PACKAGE_NAME "${APP_PACKAGE_NAME_DISPLAY}")  # used in installer window title, start menu folder
#if(APP_TARGET_64BIT)
#	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
#else()
#	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
#endif()
#
#set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\data\\\\package_data\\\\nsis_install.ico")
#set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}\\\\data\\\\package_data\\\\nsis_uninstall.ico")
#
#set(CPACK_NSIS_HELP_LINK "mailto:${APP_BRANDING_CONTACT_EMAIL}")
#set(CPACK_NSIS_URL_INFO_ABOUT "${APP_BRANDING_INFO_URL}")
#set(CPACK_NSIS_CONTACT "${APP_BRANDING_CONTACT_EMAIL}")
#set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
#set(CPACK_NSIS_INSTALLED_ICON_NAME "${APP_BRANDING_INTERNAL_APP_NAME}.exe")
#set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")  # Relative to installation directory, start menu uses this.
#set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")  # Ask to uninstall before installing.
## set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${APP_BRANDING_INTERNAL_APP_NAME}.exe")  # may conflict with UAC, better not use it.
#set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
#	Delete \\\"$INSTDIR\\\\log.txt\\\"
#	Delete \\\"$INSTDIR\\\\log.old.txt\\\"
#	Delete \\\"$INSTDIR\\\\stderr.txt\\\"
#	Delete \\\"$INSTDIR\\\\stderr.old.txt\\\"
#	Delete \\\"$INSTDIR\\\\stdout.txt\\\"
#	Delete \\\"$INSTDIR\\\\stdout.old.txt\\\"
#")

#set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")  # so that the Start Menu shortcut is not to "bin/"
#set(CPACK_NSIS_MENU_LINKS "${APP_BRANDING_INTERNAL_APP_NAME}" "${APP_PACKAGE_NAME_DISPLAY}")



# --- RPM options
# FIXME All directories are left empty and not removed after rpm uninstall.
#if (APP_TARGET_64BIT)
#	set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
#else()
#	set(CPACK_RPM_PACKAGE_ARCHITECTURE "i686")
#endif()
#set(CPACK_RPM_PACKAGE_LICENSE "GPL-3")
#set(CPACK_RPM_PACKAGE_GROUP "Hardware/Other")  # not sure
#set(CPACK_RPM_PACKAGE_VENDOR "${APP_BRANDING_COPYRIGHT_NAME}")
#set(CPACK_RPM_PACKAGE_URL "${APP_BRANDING_INFO_URL}")
#set(CPACK_RPM_COMPRESSION_TYPE "bzip2")
## set(CPACK_RPM_PACKAGE_REQUIRES "")  # FIXME We could add libqt5 here, but it will prevent the usage of bundled Qt.
## set(CPACK_RPM_PACKAGE_PROVIDES "${APP_BRANDING_INTERNAL_APP_NAME}")
## set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
#
#configure_file("${CMAKE_SOURCE_DIR}/data/package_data/rpm-post-install.in.sh" "${CMAKE_BINARY_DIR}/rpm-post-install.sh" ESCAPE_QUOTES @ONLY)
#configure_file("${CMAKE_SOURCE_DIR}/data/package_data/rpm-post-uninstall.in.sh" "${CMAKE_BINARY_DIR}/rpm-post-uninstall
#.sh" ESCAPE_QUOTES @ONLY)
#
#set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/rpm-post-install.sh")
#set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/rpm-post-uninstall.sh")



# --- DEB options
#if (APP_TARGET_64BIT)
#	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
#else()
#	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
#endif()
#set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${APP_BRANDING_COPYRIGHT_NAME}")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${APP_BRANDING_INFO_URL}")
#set(CPACK_DEBIAN_PACKAGE_SECTION "science")
## set(CPACK_DEBIAN_PACKAGE_DEPENDS "")  # FIXME We could add libqt5 here, but it will prevent the usage of bundled Qt.
## set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
#
#configure_file("${CMAKE_SOURCE_DIR}/data/package_data/debian-postinst.in.sh"
#	"${CMAKE_BINARY_DIR}/postinst" ESCAPE_QUOTES @ONLY)
#configure_file("${CMAKE_SOURCE_DIR}/data/package_data/debian-postrm.in.sh"
#	"${CMAKE_BINARY_DIR}/postrm" ESCAPE_QUOTES @ONLY)
#
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/postrm")


# All CPack variables must be set before this
include(CPack)


if (WIN32)
	add_custom_target(package_nsis
			COMMAND "${CMAKE_CPACK_COMMAND}" -G NSIS -C Release
			COMMENT "Creating NSIS installer..."
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_target_properties(package_nsis PROPERTIES
			EXCLUDE_FROM_ALL true
			EXCLUDE_FROM_DEFAULT_BUILD true
			PROJECT_LABEL "PACKAGE_NSIS_INSTALLER")  # VS likes uppercase names for better visibility of special targets

	add_custom_target(package_zip
			COMMAND "${CMAKE_CPACK_COMMAND}" -G ZIP -C Release
			COMMENT "Creating ZIP package..."
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_target_properties(package_zip PROPERTIES
			EXCLUDE_FROM_ALL true
			EXCLUDE_FROM_DEFAULT_BUILD true
			PROJECT_LABEL "PACKAGE_ZIP")  # VS likes uppercase names for better visibility of special targets
endif()

if (UNIX)
	add_custom_target(package_tbz2
			COMMAND "${CMAKE_CPACK_COMMAND}" -G TBZ2 -C Release
			COMMENT "Creating .tar.bz2 package..."
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_target_properties(package_tbz2 PROPERTIES
			EXCLUDE_FROM_ALL true
			EXCLUDE_FROM_DEFAULT_BUILD true)

	add_custom_target(package_rpm
			COMMAND "${CMAKE_CPACK_COMMAND}" -G RPM -C Release
			COMMENT "Creating RPM package..."
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_target_properties(package_rpm PROPERTIES
			EXCLUDE_FROM_ALL true
			EXCLUDE_FROM_DEFAULT_BUILD true)

	add_custom_target(package_deb
			COMMAND "${CMAKE_CPACK_COMMAND}" -G DEB -C Release
			COMMENT "Creating DEB package..."
			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_target_properties(package_deb PROPERTIES
			EXCLUDE_FROM_ALL true
			EXCLUDE_FROM_DEFAULT_BUILD true)

	add_custom_target(package_all
			DEPENDS package_tbz2 package_rpm
			COMMENT "Creating packages...")
	set_target_properties(package_all PROPERTIES
			EXCLUDE_FROM_ALL true
			EXCLUDE_FROM_DEFAULT_BUILD true)
endif()



