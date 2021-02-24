
# Manifest file
if (WIN32)
	configure_file("gsmartcontrol.exe.in.manifest" "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.exe.manifest" ESCAPE_QUOTES @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.exe.manifest" DESTINATION "${CMAKE_INSTALL_SBINDIR}/")
endif()

# gsmartcontrol binary
add_executable(gsmartcontrol WIN32)

target_sources(gsmartcontrol PRIVATE
	doxy_main_page.h
	gsc_about_dialog.cpp
	gsc_about_dialog.h
	gsc_add_device_window.cpp
	gsc_add_device_window.h
	gsc_executor_error_dialog.cpp
	gsc_executor_error_dialog.h
	gsc_executor_log_window.cpp
	gsc_executor_log_window.h
	gsc_info_window.cpp
	gsc_info_window.h
	gsc_init.cpp
	gsc_init.h
	gsc_main.cpp
	gsc_main_window.cpp
	gsc_main_window.h
	gsc_main_window_iconview.h
	gsc_preferences_window.cpp
	gsc_preferences_window.h
	gsc_settings.h
	gsc_startup_settings.h
	gsc_text_window.h
	local_glibmm.h
)

# Win32 resource file
if (WIN32)
	configure_file("gsc_winres.in.rc" "${CMAKE_CURRENT_BINARY_DIR}/gsc_winres.rc" ESCAPE_QUOTES @ONLY)
	file(COPY "${CMAKE_SOURCE_DIR}/data/gsmartcontrol.ico" "${CMAKE_CURRENT_BINARY_DIR}")  # required by winres
	target_sources(gsmartcontrol PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gsc_winres.rc")
endif()

target_link_libraries(gsmartcontrol
	PRIVATE
		applib
		app_pcrecpp_interface
		app_gtkmm_interface
		compiler_warnings
		build_config
)

install(TARGETS gsmartcontrol DESTINATION "${CMAKE_INSTALL_SBINDIR}/")

# TODO
#target_compile_definitions(gsmartcontrol
#	PRIVATE
#		-DPACKAGE_PKGDATA_DIR=\"/usr/local/share/gsmartcontrol\"
#		-DPACKAGE_SYSCONF_DIR=\"/usr/local/etc\"
#		-DPACKAGE_LOCALE_DIR=\"/usr/local/share/locale/gsmartcontrol\"
#		-DPACKAGE_DOC_DIR=\"/usr/share/doc/packages/gsmartcontrol\"
#		-DTOP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
#		-DHZ_USE_LIBDEBUG=1
#		-DHZ_ENABLE_COMPILED_RES_DATA
#		-DDEBUG_BUILD
#)


add_subdirectory(build_config)
add_subdirectory(applib)
add_subdirectory(hz)
add_subdirectory(libdebug)
add_subdirectory(rconfig)
add_subdirectory(ui)

