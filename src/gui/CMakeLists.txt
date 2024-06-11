###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 - 2024 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# gsmartcontrol binary
add_executable(gsmartcontrol WIN32)

target_sources(gsmartcontrol PRIVATE
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
	gsc_main_window_iconview.cpp
	gsc_main_window_iconview.h
	gsc_preferences_window.cpp
	gsc_preferences_window.h
	gsc_startup_settings.h
	gsc_text_window.h
)

# Win32 resource file
if (WIN32)
	configure_file("gsc_winres.in.rc" "${CMAKE_CURRENT_BINARY_DIR}/gsc_winres.rc" ESCAPE_QUOTES @ONLY NEWLINE_STYLE DOS)
	file(COPY "${CMAKE_SOURCE_DIR}/data/gsmartcontrol.ico" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")  # required by winres
	target_sources(gsmartcontrol PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gsc_winres.rc")
endif()

# Win32 manifest
if (WIN32)
	set(WINDOWS_ARCH "x86")  # Needed for manifest .in file
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(WINDOWS_ARCH "amd64")
	endif()
	configure_file("gsmartcontrol.exe.in.manifest" "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.exe.manifest"
		ESCAPE_QUOTES @ONLY NEWLINE_STYLE DOS)

	# We link the manifest into the binary instead of installing it.
	# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.exe.manifest" DESTINATION "${CMAKE_INSTALL_SBINDIR}/")
	target_sources(gsmartcontrol PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gsmartcontrol.exe.manifest")
endif()


target_link_libraries(gsmartcontrol
	PRIVATE
		applib
		app_gtkmm_interface
		build_config
)

if (WIN32)
	install(TARGETS gsmartcontrol DESTINATION .)
else()
	install(TARGETS gsmartcontrol DESTINATION "${CMAKE_INSTALL_SBINDIR}/")
endif()


add_subdirectory(ui)

