/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef GUI_UTILS_H
#define GUI_UTILS_H

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // NOT NEEDED
#undef throw

#include <string>

#include <gtkmm.h>


// These functions won't return until the dialogs are closed.
// Messages must not contain any markup.


/// Show an error dialog
void gui_show_error_dialog(const std::string& message, Gtk::Window* parent = 0);

/// Show an error dialog with a (possibly markupped) secondary message
void gui_show_error_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


/// Show a warning dialog
void gui_show_warn_dialog(const std::string& message, Gtk::Window* parent = 0);

/// Show a warning dialog with a (possibly markupped) secondary message
void gui_show_warn_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


/// Show an informational dialog
void gui_show_info_dialog(const std::string& message, Gtk::Window* parent = 0);

/// Show an informational dialog with a (possibly markupped) secondary message
void gui_show_info_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent = 0, bool sec_msg_markup = false);


/// Show a text entry dialog. \c result is filled with the user-entered string on success.
/// \return false if Cancel was clicked.
bool gui_show_text_entry_dialog(const std::string& title, const std::string& message,
		std::string& result, const std::string& default_str, Gtk::Window* parent = 0);

/// Show a text entry dialog with a (possibly markupped) secondary message.
/// \c result is filled with the user-entered string on success.
/// \return false if Cancel was clicked.
bool gui_show_text_entry_dialog(const std::string& title, const std::string& message, const std::string& sec_message,
		std::string& result, const std::string& default_str, Gtk::Window* parent = 0, bool sec_msg_markup = false);






#endif

/// @}
