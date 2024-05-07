###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

add_library(applib STATIC)

target_sources(applib PRIVATE
	async_command_executor.cpp
	async_command_executor.h
	app_builder_widget.h
	app_gtkmm_tools.cpp
	app_gtkmm_tools.h
	app_pcrecpp.h
	command_executor.h
	command_executor.cpp
	command_executor_3ware.h
	command_executor_areca.h
	command_executor_gui.cpp
	command_executor_gui.h
	command_executor_factory.cpp
	command_executor_factory.h
	gsc_settings.h
	gui_utils.cpp
	gui_utils.h
	selftest.cpp
	selftest.h
	smartctl_parser.cpp
	smartctl_parser.h
	smartctl_json_ata_parser.cpp
	smartctl_json_ata_parser.h
	smartctl_json_basic_parser.cpp
	smartctl_json_basic_parser.h
	smartctl_json_nvme_parser.cpp
	smartctl_json_nvme_parser.h
	smartctl_json_parser_helpers.h
	smartctl_executor.cpp
	smartctl_executor_gui.h
	smartctl_executor.h
	smartctl_parser_types.h
	smartctl_text_ata_parser.cpp
	smartctl_text_ata_parser.h
	smartctl_text_basic_parser.cpp
	smartctl_text_basic_parser.h
	smartctl_text_parser_helper.cpp
	smartctl_text_parser_helper.h
	smartctl_version_parser.cpp
	smartctl_version_parser.h
	storage_detector.cpp
	storage_detector.h
	storage_detector_helpers.h
	storage_detector_linux.cpp
	storage_detector_linux.h
	storage_detector_other.cpp
	storage_detector_other.h
	storage_detector_win32.cpp
	storage_detector_win32.h
	storage_device.cpp
	storage_device.h
	storage_property.cpp
	storage_property.h
	storage_property_descr.cpp
	storage_property_descr.h
	storage_property_repository.cpp
	storage_property_repository.h
	storage_settings.h
	warning_colors.h
	warning_level.h
	window_instance_manager.h
)

target_link_libraries(applib
	PUBLIC
		libdebug
		hz
		rconfig
		app_gtkmm_interface
		app_pcrecpp_interface
		app_gettext_interface
		build_config
)

target_include_directories(applib
	PUBLIC
		"${CMAKE_SOURCE_DIR}/src"
)


add_subdirectory(examples)
add_subdirectory(tests)

