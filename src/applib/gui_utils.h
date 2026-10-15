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
#include <utility>



/// Determine whether a given full scale percentage represents fractional scaling.
/// Fractional scaling exists when the percentage is above 100 and not a multiple of 100.
/// Scales below 100% are never considered fractional (GTK3 does not scale below 1x).
/// \param full_scale_percent The full scale percentage (e.g., 125, 200, 250)
/// \return true if the scaling has a fractional component
bool app_is_fractional_scaling(int full_scale_percent);


/// Get the fractional part of a full scale percentage (e.g., 250 -> 50, 125 -> 25).
/// Returns 0 if the scaling is not fractional (see app_is_fractional_scaling()).
int app_get_fractional_scaling_part_percent(int full_scale_percent);


/// Compute the fractional scaling correction ratio from a full scale percentage.
/// For example, 125% -> correction = 1.25, 250% -> correction = 1.25 (since GTK uses 2x integer scaling).
/// Returns 0.0 if the scaling is not fractional (see app_is_fractional_scaling()).
double app_compute_fractional_scaling_correction(int full_scale_percent);


/// Apply the fractional scaling correction to a window size.
/// \param base_w Base width (before fractional correction)
/// \param base_h Base height (before fractional correction)
/// \param correction The correction ratio from app_compute_fractional_scaling_correction()
/// \return Scaled (width, height) pair, or the unchanged base size if correction or size is not positive.
std::pair<int, int> app_apply_fractional_scaling_to_size(int base_w, int base_h, double correction);


/// Compute the full scale percentage from a DPI value.
/// \param h_ppi Horizontal pixels per inch
/// \return Full scale percentage (e.g., 96 DPI -> 100, 120 DPI -> 125, 240 DPI -> 250), or 0 if h_ppi <= 0.
int app_compute_scaling_percent_from_dpi(double h_ppi);


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


/// Check if a dark GTK theme is currently active
bool gui_is_dark_theme_active();

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
