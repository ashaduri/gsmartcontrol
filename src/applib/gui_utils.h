/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "local_glibmm.h"
#include <gtkmm.h>
#include <string>



// These functions won't return until the dialogs are closed.
// Messages must not contain any markup.


/// Show an error dialog
void gui_show_error_dialog(const std::string& message, Gtk::Window* parent = nullptr);

/// Show an error dialog with a (possibly markupped) secondary message
void gui_show_error_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = nullptr, bool sec_msg_markup = false);


/// Show a warning dialog
void gui_show_warn_dialog(const std::string& message, Gtk::Window* parent = nullptr);

/// Show a warning dialog with a (possibly markupped) secondary message
void gui_show_warn_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = nullptr, bool sec_msg_markup = false);


/// Show an informational dialog
void gui_show_info_dialog(const std::string& message, Gtk::Window* parent = nullptr);

/// Show an informational dialog with a (possibly markupped) secondary message
void gui_show_info_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = nullptr, bool sec_msg_markup = false);


/// Show a text entry dialog. \c result is filled with the user-entered string on success.
/// \return false if Cancel was clicked.
bool gui_show_text_entry_dialog(const std::string& title, const std::string& message,
		std::string& result, const std::string& default_str, Gtk::Window* parent = nullptr);

/// Show a text entry dialog with a (possibly markupped) secondary message.
/// \c result is filled with the user-entered string on success.
/// \return false if Cancel was clicked.
bool gui_show_text_entry_dialog(const std::string& title, const std::string& message, const std::string& sec_message,
		std::string& result, const std::string& default_str, Gtk::Window* parent = 0, bool sec_msg_markup = false);






#endif

/// @}
