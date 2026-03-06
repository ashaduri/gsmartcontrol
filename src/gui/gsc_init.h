/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_INIT_H
#define GSC_INIT_H

#include <string>


namespace Gtk {
	class Window;
}


/// Initialize the application and run the main loop
bool app_init_and_loop(int& argc, char**& argv);


/// Quit the application (exit the main loop)
void app_quit();


/// Return everything that went through libdebug's channels.
/// Useful for showing logs.
std::string app_get_debug_buffer_str();


/// Get the fractional scaling percentage detected on Windows (0 if not detected).
/// For example, at 150% scaling, this returns 50; at 125% scaling, this returns 25.
int app_get_windows_fractional_scaling_percent();


/// Apply fractional scaling to default window size if fractional scaling is detected.
/// This compensates for GTK3's lack of fractional scaling support on Windows.
/// \param window The window to apply scaling to
/// \param config_size_w Configured width (0 if using glade default)
/// \param config_size_h Configured height (0 if using glade default)
void app_apply_fractional_scaling_to_default_size(Gtk::Window* window, int config_size_w, int config_size_h);



#endif

/// @}
