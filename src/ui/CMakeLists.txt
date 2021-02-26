###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2021 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

install(FILES
	gsc_about_dialog.glade
	gsc_add_device_window.glade
	gsc_executor_log_window.glade
	gsc_info_window.glade
	gsc_main_window.glade
	gsc_preferences_window.glade
	gsc_text_window.glade

	DESTINATION "${CMAKE_INSTALL_DATADIR}/gsmartcontrol/ui/"
)
