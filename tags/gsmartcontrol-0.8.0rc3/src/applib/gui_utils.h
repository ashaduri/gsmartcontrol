/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <string>

#include <gtkmm/window.h>


// These functions won't return until the dialogs are closed.

void gui_show_error_dialog(const std::string& message, Gtk::Window* parent = 0);

void gui_show_error_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


void gui_show_warn_dialog(const std::string& message, Gtk::Window* parent = 0);

void gui_show_warn_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


void gui_show_info_dialog(const std::string& message, Gtk::Window* parent = 0);

void gui_show_info_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);






#endif
