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

#include <glibmm.h>
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
		std::string& result, const std::string& default_str, Gtk::Window* parent = nullptr, bool sec_msg_markup = false);



/// Get the fractional scaling percentage detected on Windows (0 if not detected or integer scale).
/// For example, at 150% scaling, this returns 150; at 125% scaling, this returns 125; at 250% scaling, this returns 250.
/// Returns 0 for exact integer scales (100%, 200%, etc.).
int app_get_windows_fractional_scaling_percent();


/// Set the fractional scaling percentage detected on Windows.
/// This should only be called once during application initialization.
void app_set_windows_fractional_scaling_percent(int percent);


/// Apply fractional scaling to default window size if fractional scaling is detected.
/// This compensates for GTK3's lack of fractional scaling support on Windows.
/// \param window The window to apply scaling to
/// \param config_size_w Configured width (0 if using glade default)
/// \param config_size_h Configured height (0 if using glade default)
void app_apply_fractional_scaling_to_default_size(Gtk::Window* window, int config_size_w, int config_size_h);



#endif

/// @}
