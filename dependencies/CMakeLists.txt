###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

find_package(PkgConfig REQUIRED)


# Gtkmm.
# Don't make it REQUIRED, we may want to build only the parsers
pkg_check_modules(Gtkmm IMPORTED_TARGET GLOBAL "gtkmm-3.0>=3.0")
add_library(app_gtkmm_interface INTERFACE)
target_link_libraries(app_gtkmm_interface
	INTERFACE
		PkgConfig::Gtkmm
)
target_compile_definitions(app_gtkmm_interface
	INTERFACE
		ENABLE_GLIB=1
		ENABLE_GLIBMM=1
		# For porting to GTK4
		GDK_DISABLE_DEPRECATED=1
		GTK_DISABLE_DEPRECATED=1
)

# Support pre-C++17 glibmm with throw(...) exception specifications
pkg_check_modules(Glibmm "glibmm-2.4")
if ("${Glibmm_VERSION}" VERSION_LESS "2.50.1")
	target_compile_definitions(app_gtkmm_interface INTERFACE "APP_GLIBMM_USES_THROW")
	message(STATUS "Enabling old glibmm throw(...) workaround")
endif()


# pcrecpp from pcre1
pkg_check_modules(Pcrecpp REQUIRED IMPORTED_TARGET GLOBAL libpcrecpp)
add_library(app_pcrecpp_interface INTERFACE)
target_link_libraries(app_pcrecpp_interface
	INTERFACE
		PkgConfig::Pcrecpp
)


# Gettext libintl
find_package(Intl REQUIRED)
add_library(app_gettext_interface INTERFACE)
if (Intl_INCLUDE_DIRS)
	target_include_directories(app_gettext_interface
		INTERFACE
			${Intl_INCLUDE_DIRS}
	)
endif()
if (Intl_LIBRARIES)
	target_link_libraries(app_gettext_interface
		INTERFACE
			${Intl_LIBRARIES}
	)
endif()

# Gettext binaries and macros.
# See https://cmake.org/cmake/help/v3.15/module/FindGettext.html
find_package(Gettext REQUIRED)


add_subdirectory(catch2)
add_subdirectory(nlohmann_json)
