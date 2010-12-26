/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <string>

#include <gtkmm/window.h>


// These functions won't return until the dialogs are closed.

// Message must not contain any markup.

void gui_show_error_dialog(const std::string& message, Gtk::Window* parent = 0);

void gui_show_error_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


void gui_show_warn_dialog(const std::string& message, Gtk::Window* parent = 0);

void gui_show_warn_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


void gui_show_info_dialog(const std::string& message, Gtk::Window* parent = 0);

void gui_show_info_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


// Returns false if Cancel was clicked. Puts the user-entered string into result otherwise.
bool gui_show_text_entry_dialog(const std::string& title, const std::string& message,
		std::string& result, const std::string& default_str, Gtk::Window* parent = 0);

bool gui_show_text_entry_dialog(const std::string& title, const std::string& message, const std::string& sec_message,
		std::string& result, const std::string& default_str, Gtk::Window* parent = 0, bool sec_msg_markup = false);






#endif
