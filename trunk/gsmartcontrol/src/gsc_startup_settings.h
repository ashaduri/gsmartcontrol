/**************************************************************************
 Copyright:
      (C) 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
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
	bool hide_tabs_on_smart_disabled = true;  ///< Hide additional Info Window tabs if SMART is disabled.

};



/// Get startup settings
inline GscStartupSettings& get_startup_settings()
{
	static GscStartupSettings startup_settings;
	return startup_settings;
}





#endif

/// @}
