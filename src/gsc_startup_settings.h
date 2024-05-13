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

#ifndef GSC_STARTUP_SETTINGS_H
#define GSC_STARTUP_SETTINGS_H



/// Settings passed using command-line options
struct GscStartupSettings {

	bool no_scan = false;  ///< No scanning on startup
	std::vector<std::string> load_virtuals;  ///< Virtual files to load
	std::vector<std::string> add_devices;  ///< Devices to add (with options)

};



/// Get startup settings
[[nodiscard]] inline GscStartupSettings& get_startup_settings()
{
	static GscStartupSettings startup_settings;
	return startup_settings;
}





#endif

/// @}
