/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup rconfig
/// \weakgroup rconfig
/// @{

#ifndef RCONFIG_RCAUTOSAVE_H
#define RCONFIG_RCAUTOSAVE_H

#include "hz/hz_config.h"  // ENABLE_GLIB


// Autosave functions are only available if GLib is enabled.
#if defined ENABLE_GLIB && ENABLE_GLIB


#include <string>
#include <glib.h>

#include "hz/debug.h"
#include "hz/fs_path.h"

#include "rcloadsave.h"



namespace rconfig {


/// Holder for static variables
struct AutoSaveHolder {
	static inline std::string config_file;  ///< Config file to autosave to.
	static inline bool autosave_enabled = false;  ///< Autosave enabled or not. This acts as a stopper flag for autosave callback.
};



extern "C" {

	/// Autosave timeout callback for Glib. internal.
	inline gboolean autosave_timeout_callback(void* data)
	{
		bool force = (bool)data;

		if (!force && !AutoSaveHolder::autosave_enabled)  // no more autosaves
			return false;  // remove timeout, disable autosave for real.

		debug_print_info("rconfig", "Autosaving config to \"%s\".\n", AutoSaveHolder::config_file.c_str());

		hz::FsPath p(AutoSaveHolder::config_file);
		if ((p.exists() && !p.is_regular()) || !p.is_writable()) {
			debug_out_error("rconfig", "Autosave failed: Cannot write to file: " << p.get_error_locale() << "\n");
			return (force ? false : true);  // if manual, return failure. else, don't stop the timeout.
		}

		bool status = rconfig::save_to_file(AutoSaveHolder::config_file);
		if (force)
			return status;  // return status to caller

		return true;  // continue timeouts
	}

}




/// Set config file to autosave to.
inline bool autosave_set_config_file(const std::string& file)
{
	if (file.empty()) {
		debug_print_error("rconfig", "autosave_set_config_file(): Error: Filename is empty.\n");
		return false;
	}

	AutoSaveHolder::config_file = file;

	debug_print_info("rconfig", "Setting autosave config file to \"%s\"\n", file.c_str());
	return true;
}



/// Enable autosave every \c sec_interval seconds.
inline bool autosave_start(unsigned int sec_interval)
{
	if (AutoSaveHolder::autosave_enabled) {  // already autosaving, you should stop it first.
		debug_print_warn("rconfig", "Error while starting config autosave: Autosave is active already.\n");
		return false;
	}

	AutoSaveHolder::autosave_enabled = true;
	debug_print_info("rconfig", "Starting config autosave with %u sec. interval.\n", sec_interval);

	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, sec_interval*1000,
			&autosave_timeout_callback, nullptr, nullptr);

	return true;
}



/// Disable autosave
inline void autosave_stop()
{
	debug_print_info("rconfig", "Stopping config autosave.\n");

	// set the stop flag. it will make autosave stop on next timeout callback call.
	AutoSaveHolder::autosave_enabled = false;
}



/// Forcibly save the config now.
inline bool autosave_force_now()
{
	return autosave_timeout_callback((void*)true);  // anyone tell me what is the C++ variant of this?
}





}  // ns



#endif  // glib


#endif

/// @}
