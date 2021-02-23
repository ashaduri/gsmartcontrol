
find_package(PkgConfig REQUIRED)


# Don't make it REQUIRED, we may want to build only the parsers
pkg_check_modules(Gtkmm IMPORTED_TARGET GLOBAL gtkmm-3.0 >= 3.0)

add_library(app_gtkmm_interface INTERFACE)

target_link_libraries(app_gtkmm_interface
	PUBLIC
		PkgConfig::Gtkmm
)

target_compile_definitions(app_gtkmm_interface
	PUBLIC
		ENABLE_GLIB=1
		ENABLE_GLIBMM=1
)



# pcrecpp from pcre1
pkg_check_modules(Pcrecpp REQUIRED IMPORTED_TARGET GLOBAL libpcrecpp)

add_library(app_pcrecpp_interface INTERFACE)

target_link_libraries(app_pcrecpp_interface
	PUBLIC
		PkgConfig::Pcrecpp
)


# Gettext / intl
find_package(Intl REQUIRED)

add_library(app_gettext_interface INTERFACE)

if (Intl_INCLUDE_DIRS)
	target_include_directories(app_gettext_interface
		PUBLIC
			${Intl_INCLUDE_DIRS}
	)
endif()

if (Intl_LIBRARIES)
	target_link_libraries(app_gettext_interface
		PUBLIC
			${Intl_LIBRARIES}
	)
endif()

find_package(Gettext REQUIRED)

# See https://cmake.org/cmake/help/v3.15/module/FindGettext.html

