# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "/home/runner/work/gsmartcontrol/gsmartcontrol;/home/runner/work/gsmartcontrol/gsmartcontrol/_codeql_build_dir")
set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/usr/local/share/cmake-3.31/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "gsmartcontrol built using CMake")
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE "ON")
set(CPACK_GENERATOR "TBZ2")
set(CPACK_INNOSETUP_ARCHITECTURE "x64")
set(CPACK_INSTALL_CMAKE_PROJECTS "/home/runner/work/gsmartcontrol/gsmartcontrol/_codeql_build_dir;gsmartcontrol;ALL;/")
set(CPACK_INSTALL_PREFIX "/usr/local")
set(CPACK_MODULE_PATH "/home/runner/work/gsmartcontrol/gsmartcontrol/data/cmake/")
set(CPACK_MONOLITHIC_INSTALL "true")
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_CONTACT "https://gsmartcontrol.shaduri.dev")
set(CPACK_NSIS_DISPLAY_NAME "GSmartControl")
set(CPACK_NSIS_DISPLAY_NAME_SET "TRUE")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
	Delete \"$INSTDIR\\drivedb.h.*\"
	Delete \"$INSTDIR\\*stderr.txt\"
	Delete \"$INSTDIR\\*stderr.old.txt\"
	Delete \"$INSTDIR\\*stdout.txt\"
	Delete \"$INSTDIR\\*stdout.old.txt\"
")
set(CPACK_NSIS_HELP_LINK "https://gsmartcontrol.shaduri.dev")
set(CPACK_NSIS_INSTALLED_ICON_NAME "gsmartcontrol.exe")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_MENU_LINKS "gsmartcontrol;GSmartControl")
set(CPACK_NSIS_MUI_ICON "/home/runner/work/gsmartcontrol/gsmartcontrol\\packaging\\nsis\\nsi_install.ico")
set(CPACK_NSIS_MUI_UNIICON "/home/runner/work/gsmartcontrol/gsmartcontrol\\packaging\\nsis\\nsi_uninstall.ico")
set(CPACK_NSIS_PACKAGE_NAME "GSmartControl")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_NSIS_URL_INFO_ABOUT "https://gsmartcontrol.shaduri.dev")
set(CPACK_OBJCOPY_EXECUTABLE "/usr/bin/objcopy")
set(CPACK_OBJDUMP_EXECUTABLE "/usr/bin/objdump")
set(CPACK_OUTPUT_CONFIG_FILE "/home/runner/work/gsmartcontrol/gsmartcontrol/_codeql_build_dir/CPackConfig.cmake")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/home/runner/work/gsmartcontrol/gsmartcontrol/_codeql_build_dir/packaging/nsis/distribution.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hard Disk Drive and SSD Health Inspection Tool")
set(CPACK_PACKAGE_EXECUTABLES "gsmartcontrol;GSmartControl")
set(CPACK_PACKAGE_FILE_NAME "gsmartcontrol-2.0.2-Linux")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://gsmartcontrol.shaduri.dev")
set(CPACK_PACKAGE_ICON "/home/runner/work/gsmartcontrol/gsmartcontrol\\data\\gsmartcontrol.ico")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "gsmartcontrol 2.0.2")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "gsmartcontrol 2.0.2")
set(CPACK_PACKAGE_NAME "gsmartcontrol")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Alexander Shaduri")
set(CPACK_PACKAGE_VERSION "2.0.2")
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "2")
set(CPACK_READELF_EXECUTABLE "/usr/bin/readelf")
set(CPACK_RESOURCE_FILE_LICENSE "/home/runner/work/gsmartcontrol/gsmartcontrol/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "/usr/local/share/cmake-3.31/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "/usr/local/share/cmake-3.31/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_GENERATOR "TBZ2;TGZ;TXZ;TZ")
set(CPACK_SOURCE_IGNORE_FILES "/\\.idea/;/doxygen_doc/;/cmake-build-.*;/\\.git/;.*~$;/CMakeLists\\.txt\\.user$;/TODO$")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/runner/work/gsmartcontrol/gsmartcontrol/_codeql_build_dir/CPackSourceConfig.cmake")
set(CPACK_SOURCE_RPM "OFF")
set(CPACK_SOURCE_STRIP_FILES "FALSE")
set(CPACK_SOURCE_TBZ2 "ON")
set(CPACK_SOURCE_TGZ "ON")
set(CPACK_SOURCE_TXZ "ON")
set(CPACK_SOURCE_TZ "ON")
set(CPACK_SOURCE_ZIP "OFF")
set(CPACK_STRIP_FILES "TRUE")
set(CPACK_SYSTEM_NAME "Linux")
set(CPACK_THREADS "1")
set(CPACK_TOPLEVEL_TAG "Linux")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/home/runner/work/gsmartcontrol/gsmartcontrol/_codeql_build_dir/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
