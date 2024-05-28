###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# CPack options for this project

# Generators supported:
# Linux: TBZ2 (binary)
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

set(APP_PACKAGE_NAME_DISPLAY "GSmartControl")
#if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
#	set(APP_PACKAGE_NAME_DISPLAY "${APP_PACKAGE_NAME_DISPLAY} (64-Bit)")
#endif()

#set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
if (WIN32)
	# Default value is "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}".
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")  # appended to e.g. CPACK_NSIS_INSTALL_ROOT
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
endif()

set(CPACK_PACKAGE_VENDOR "Alexander Shaduri")

#set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
#set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
#set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})

# Used by wix only?
#set(APP_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/data/${APP_BRANDING_INTERNAL_APP_NAME}/package_description.txt")
#if (EXISTS "${APP_PACKAGE_DESCRIPTION_FILE}")
#	set(CPACK_PACKAGE_DESCRIPTION_FILE "${APP_PACKAGE_DESCRIPTION_FILE}")
#endif()

#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CMAKE_PROJECT_DESCRIPTION}")

# Set to win32 or win64 by default, or CMAKE_SYSTEM_NAME for others.
#set(CPACK_SYSTEM_NAME "${APP_TARGET_SYSTEM_NAME}")

#if (WIN32)
#	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
#endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_STRIP_FILES FALSE)  # update-smart-drivedb doesn't like this. TODO re-enable now that it's a script
set(CPACK_SOURCE_STRIP_FILES FALSE)

# Icon in Add/Remove Programs
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\data\\\\gsmartcontrol.ico")

# Start menu shortcut
set(CPACK_PACKAGE_EXECUTABLES "gsmartcontrol;${APP_PACKAGE_NAME_DISPLAY}")

# Force monolithic installers (ignore per-component stuff, exclude optional components).
set(CPACK_MONOLITHIC_INSTALL true)

SET(CPACK_SOURCE_IGNORE_FILES
#	"/0.*"  # build dirs (broken if full path contains 0)
	"/\\\\.idea/"
	"/doxygen_doc/"
	"/cmake-build-.*"  # build dirs
	"/\\\\.git/"  # git
	".*~$"  # editor temporary files
	"/CMakeLists\\\\.txt\\\\.user$"
	"/TODO$"
)



# --- NSIS options

set(CPACK_NSIS_DISPLAY_NAME "${APP_PACKAGE_NAME_DISPLAY}")  # used in add/remove programs as a display text
set(CPACK_NSIS_PACKAGE_NAME "${APP_PACKAGE_NAME_DISPLAY}")  # used in installer window title, start menu folder
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
else()
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
endif()

set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\packaging\\\\nsis\\\\nsi_install.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}\\\\packaging\\\\nsis\\\\nsi_uninstall.ico")

set(CPACK_NSIS_HELP_LINK "${PROJECT_HOMEPAGE_URL}")
set(CPACK_NSIS_URL_INFO_ABOUT "${PROJECT_HOMEPAGE_URL}")
set(CPACK_NSIS_CONTACT "${PROJECT_HOMEPAGE_URL}")
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_INSTALLED_ICON_NAME "gsmartcontrol.exe")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")  # Relative to installation directory, start menu uses this.
set(CPACK_NSIS_MENU_LINKS "gsmartcontrol" "${APP_PACKAGE_NAME_DISPLAY}")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")  # Ask to uninstall before installing.

# set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${APP_BRANDING_INTERNAL_APP_NAME}.exe")  # may conflict with UAC, better not use it.

# update-smart-drivedb may leave drivedb.h.*
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
	Delete \\\"$INSTDIR\\\\drivedb.h.*\\\"
	Delete \\\"$INSTDIR\\\\*stderr.txt\\\"
	Delete \\\"$INSTDIR\\\\*stderr.old.txt\\\"
	Delete \\\"$INSTDIR\\\\*stdout.txt\\\"
	Delete \\\"$INSTDIR\\\\*stdout.old.txt\\\"
")



# ---------------------------------------------- Windows Dependencies

# Install GTK+ and other dependencies in Windows.
# Requires installed smartctl-nc.exe, smartctl.exe, update-smart-drivedb.ps1 in bin subdirectory of sysroot.
# The following packages when cross-compiling from opensuse:
# mingw64-cross-gcc-c++ mingw64-gtkmm3-devel adwaita-icon-theme
if (WIN32)
	message(STATUS "CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")

	set(APP_WINDOWS_SYSROOT "" CACHE "Location of system root for Windows binaries, for packing")
	if (NOT APP_WINDOWS_SYSROOT)
#		if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
#			if (CMAKE_SIZEOF_VOID_P EQUAL 8)
#				set(APP_WINDOWS_SYSROOT "/mingw64")
#			else()
#				set(APP_WINDOWS_SYSROOT "/mingw32")
#			endif()
#			file(TO_CMAKE_PATH "${APP_WINDOWS_SYSROOT}" APP_WINDOWS_SYSROOT)
#		else()  # Cross-compiling
			set(APP_WINDOWS_SYSROOT "${CMAKE_FIND_ROOT_PATH}")
#		endif()
	endif()
	if (NOT APP_WINDOWS_SYSROOT)
		message(FATAL_ERROR "APP_WINDOWS_SYSROOT or CMAKE_FIND_ROOT_PATH must be set to environment root directory when compiling for Windows.")
	endif()
	message(STATUS "APP_WINDOWS_SYSROOT: ${APP_WINDOWS_SYSROOT}")

	set(APP_WINDOWS_GTK_ICONS_ROOT "" CACHE "Location of root folder for icons, for packing Windows packages")
	if (NOT APP_WINDOWS_GTK_ICONS_ROOT)
		if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
			set(APP_WINDOWS_GTK_ICONS_ROOT "${APP_WINDOWS_SYSROOT}/share/icons")
		else()  # Cross-compiling
			set(APP_WINDOWS_GTK_ICONS_ROOT "/usr/share/icons")
		endif()
	endif()
	message(STATUS "APP_WINDOWS_GTK_ICONS_ROOT: ${APP_WINDOWS_GTK_ICONS_ROOT}")

	# This is for non-CI builds (CI uses extracted files).
	# FIXME The logic may be wrong here.
	set(APP_WINDOWS_SMARTCTL_ROOT "" CACHE "Location of root folder for smartctl, for packing Windows packages")
	if (NOT APP_WINDOWS_SMARTCTL_ROOT)
		if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
			# PROGRAMFILES matches the bitness of the installer.
			if (DEFINED ENV{PROGRAMFILES})
				set(APP_WINDOWS_SMARTCTL_ROOT "$ENV{PROGRAMFILES}/smartmontools")
			elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
				set(APP_WINDOWS_SMARTCTL_ROOT "C:\\Program Files\\smartmontools")
			else()  # 32-bit
				set(APP_WINDOWS_SMARTCTL_ROOT "C:\\Program Files (x86)\\smartmontools")
			endif ()
			file(TO_CMAKE_PATH "${APP_WINDOWS_SMARTCTL_ROOT}" APP_WINDOWS_SMARTCTL_ROOT)
		else()  # Cross-compiling
			if (CMAKE_SIZEOF_VOID_P EQUAL 8)
				set(APP_WINDOWS_SMARTCTL_ROOT "$ENV{HOME}/.wine/drive_c/Program Files/smartmontools")
			else()
				set(APP_WINDOWS_SMARTCTL_ROOT "$ENV{HOME}/.wine/drive_c/Program Files (x86)/smartmontools")
			endif()
		endif()
	endif()
	message(STATUS "APP_WINDOWS_SMARTCTL_ROOT: ${APP_WINDOWS_SMARTCTL_ROOT}")


	set(WINDOWS_SUFFIX "win32")
	set(SMARTCTL_EXTRACED_BIN_DIR "bin")
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(WINDOWS_SUFFIX "win64")
		set(SMARTCTL_EXTRACED_BIN_DIR "bin64")
	endif()

	# TODO unix2dos doc/*.txt

	# Smartmontools
	if (EXISTS "${CMAKE_BINARY_DIR}/smartmontools")  # CI
		# GitHub extract location. The files are extracted without relative paths in archive (7z e).
		install(FILES
			"${CMAKE_BINARY_DIR}/smartmontools/drivedb.h"
			"${CMAKE_BINARY_DIR}/smartmontools/update-smart-drivedb.ps1"
			"${CMAKE_BINARY_DIR}/smartmontools/smartctl-nc.exe"
			"${CMAKE_BINARY_DIR}/smartmontools/smartctl.exe"
			DESTINATION .
		)
	else()
		# System-installed smartmontools
		install(FILES
			"${APP_WINDOWS_SMARTCTL_ROOT}/bin/drivedb.h"
			"${APP_WINDOWS_SMARTCTL_ROOT}/bin/smartctl-nc.exe"
			"${APP_WINDOWS_SMARTCTL_ROOT}/bin/smartctl.exe"
			"${APP_WINDOWS_SMARTCTL_ROOT}/bin/update-smart-drivedb.ps1"
			DESTINATION .
		)
	endif()

	# GCC Runtime
	file(GLOB MATCHED_FILES LIST_DIRECTORIES false "${APP_WINDOWS_SYSROOT}/bin/libgcc_s_*.dll")
	install(FILES ${MATCHED_FILES} DESTINATION .)
	file(GLOB MATCHED_FILES LIST_DIRECTORIES false "${APP_WINDOWS_SYSROOT}/bin/libstdc++-*.dll")
	install(FILES ${MATCHED_FILES} DESTINATION .)


	#	All of GTK+
	set(GTK_FILES
#		gdk-pixbuf-query-loaders.exe
		gspawn-win32-helper-console.exe
		gspawn-win32-helper.exe
		gspawn-win64-helper-console.exe
		gspawn-win64-helper.exe
		gtk-query-settings.exe
#		gtk-query-immodules-3.0.exe
#		gtk-update-icon-cache-3.0.exe

		edit.dll
		libarchive-*.dll
		libasprintf-*.dll
		libatk-*.dll
		libatkmm-*.dll
		libatomic-*.dll
		libb2-*.dll
		libbrotlicommon.dll
		libbrotlidec.dll
		libbrotlienc.dll
		libbz2-*.dll
		libcairo-*.dll
		libcairo-gobject-*.dll
		libcairo-script-interpreter-*.dll
		libcairomm-*.dll
		libcares-*.dll
		libcharset-*.dll
		libcrypto-*.dll
		libcurl-*.dll
		libdatrie-*.dll
		libdeflate*.dll
		libepoxy-*.dll
		libexpat-*.dll
		libffi-*.dll
		libfontconfig-*.dll
		libformw*.dll
		libfreetype-*.dll
		libfribidi-*.dll
		libgailutil-*.dll
		libgdk-*.dll
		libgdkmm-*.dll
		libgdk_pixbuf-*.dll
		libgettextlib-*.dll
		libgettextpo-*.dll
		libgettextsrc-*.dll
		libgif-*.dll
		libgio-*.dll
		libgiomm-*.dll
		libgirepository-*.dll
		libglib-*.dll
		libglibmm-*.dll
		libglibmm_generate_extra_defs-*.dll
		libgmodule-*.dll
		libgmp-*.dll
		libgmpxx-*.dll
		libgobject-*.dll
		libgomp-*.dll
		libgraphite*.dll
		libgthread-*.dll
		libgtk-*.dll
		libgtkmm-*.dll
		libharfbuzz-*.dll
		libhistory*.dll
		libhogweed-*.dll
		libiconv-*.dll
		libidn*.dll
		libintl-*.dll
		libisl-*.dll
		libjansson-*.dll
		libjemalloc.dll
		libjbig-*.dll
		libjpeg-*.dll
		libjson-glib-*.dll
		libjsoncpp-*.dll
		libLerc-*.dll
		liblz4.dll
		liblzma-*.dll
		liblzo2-*.dll
		libmenuw*.dll
		libmpc-*.dll
		libmetalink-*.dll
		libmpdec++-*.dll
		libmpdec-*.dll
		libmpfr-*.dll
		libncurses*.dll
		libnettle-*.dll
		libnghttp2-*.dll
		libp11-kit-*.dll
		libpanelw*.dll
		libpango-*.dll
		libpangocairo-*.dll
		libpangoft2-*.dll
		libpangomm-*.dll
		libpangowin32-*.dll
		libpcre2-*.dll
		libpixman-*.dll
		libpng*-*.dll
		libpsl-*.dll
#		libpython*.dll
		libquadmath-*.dll
		libreadline*.dll
		librhash.dll
		librsvg-*.dll
		libsharpyuv-*.dll
		libsigc-*.dll
		libsqlite3-*.dll
		libssh2-*.dll
#		libssl-*.dll
		libssp-*.dll
		libsystre-*.dll
		libtasn*.dll
		libtermcap-*.dll
		libthai-*.dll
		libtiff-*.dll
		libtiffxx-*.dll
		libtre-*.dll
		libturbojpeg.dll
		libunistring-*.dll
		libuv-*.dll
#		libwebp-*.dll
#		libwebpdecoder-*.dll
#		libwebpdemux-*.dll
#		libwebpmux-*.dll
		libwinpthread-*.dll
		libxml2-*.dll
		libzstd.dll
#		tcl86.dll
#		tk86.dll
		zlib*.dll
	)
	foreach(pattern ${GTK_FILES})
		file(GLOB MATCHED_FILES
			LIST_DIRECTORIES false
			"${APP_WINDOWS_SYSROOT}/bin/${pattern}"
		)
		message(STATUS "Matched files in ${APP_WINDOWS_SYSROOT}/bin for ${pattern}: ${MATCHED_FILES}")
		if (NOT "${MATCHED_FILES}" STREQUAL "")
			install(FILES ${MATCHED_FILES} DESTINATION .)
		endif()
	endforeach()

	# gdk_pixbuf loaders
	install(DIRECTORY "${APP_WINDOWS_SYSROOT}/lib/gdk-pixbuf-2.0/2.10.0/loaders"
		DESTINATION "lib/gdk-pixbuf-2.0/2.10.0/"
		FILES_MATCHING PATTERN "*.dll")
	install(FILES "${APP_WINDOWS_SYSROOT}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
		DESTINATION "lib/gdk-pixbuf-2.0/2.10.0/")


	# Msys2 in github has problems with this (cannot install: file exists!)
#	install(FILES "${APP_WINDOWS_SYSROOT}/etc/fonts/fonts.conf" DESTINATION "etc/fonts/")

	# <prefix>/etc/gtk-3.0/ should contain settings.ini with a win32 theme.
	# Use custom settings.ini to enable Windows theme
#	install(FILES "${APP_WINDOWS_SYSROOT}/etc/gtk-3.0/settings.ini" DESTINATION "etc/gtk-3.0/")
	install(FILES "${CMAKE_SOURCE_DIR}/packaging/gtk/etc/gtk-3.0/settings.ini" DESTINATION "etc/gtk-3.0/")
	#	install(FILES "${APP_WINDOWS_SYSROOT}/etc/gtk-3.0/im-multipress.conf" DESTINATION "etc/gtk-3.0/")

	# Not present in msys2
#	install(DIRECTORY "${APP_WINDOWS_SYSROOT}/share/themes" DESTINATION "share/")

	# needed for file chooser
	# Not present in msys2 (maybe use plain glib, not mingw64 variant?)
#	install(DIRECTORY "${APP_WINDOWS_SYSROOT}/share/glib-2.0/schemas" DESTINATION "share/glib-2.0/")


	set(APP_WINDOWS_INSTALL_GTK_ICONS true)

	if (APP_WINDOWS_INSTALL_GTK_ICONS)
		# needed for window titlebar (if using client-side decorations),
		# tree sorting indicators, GUI icons.
# windows-close
# window-maximize
# window-minimize
# window-restore
# pan-down
# pan-up
# dialog-information
# dialog-error
# dialog-warning

		install(DIRECTORY "${APP_WINDOWS_GTK_ICONS_ROOT}/hicolor"
			DESTINATION "share/icons/")

		install(DIRECTORY "${APP_WINDOWS_GTK_ICONS_ROOT}/Adwaita"
			DESTINATION "share/icons/")
	endif()  # icons
endif()



# All CPack variables must be set before this
include(CPack)


if (WIN32)

	# TODO Support NSIS from Linux
	#	cd nsis-dist && @NSIS_EXEC@ gsmartcontrol.nsi

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

#if (UNIX)
#	add_custom_target(package_tbz2
#			COMMAND "${CMAKE_CPACK_COMMAND}" -G TBZ2 -C Release
#			COMMENT "Creating .tar.bz2 package..."
#			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
#	set_target_properties(package_tbz2 PROPERTIES
#			EXCLUDE_FROM_ALL true
#			EXCLUDE_FROM_DEFAULT_BUILD true)
#
#	add_custom_target(package_rpm
#			COMMAND "${CMAKE_CPACK_COMMAND}" -G RPM -C Release
#			COMMENT "Creating RPM package..."
#			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
#	set_target_properties(package_rpm PROPERTIES
#			EXCLUDE_FROM_ALL true
#			EXCLUDE_FROM_DEFAULT_BUILD true)
#
#	add_custom_target(package_deb
#			COMMAND "${CMAKE_CPACK_COMMAND}" -G DEB -C Release
#			COMMENT "Creating DEB package..."
#			WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
#	set_target_properties(package_deb PROPERTIES
#			EXCLUDE_FROM_ALL true
#			EXCLUDE_FROM_DEFAULT_BUILD true)
#
#	add_custom_target(package_all
#			DEPENDS package_tbz2 package_rpm
#			COMMENT "Creating packages...")
#	set_target_properties(package_all PROPERTIES
#			EXCLUDE_FROM_ALL true
#			EXCLUDE_FROM_DEFAULT_BUILD true)
#endif()


