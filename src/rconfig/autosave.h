/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup rconfig
/// \weakgroup rconfig
/// @{

#ifndef RCONFIG_AUTOSAVE_H
#define RCONFIG_AUTOSAVE_H

// Autosave functions are only available if GLib is enabled.
#if !defined ENABLE_GLIB || !(ENABLE_GLIB)
	#error "Glib support must be enabled."
#endif

#include <string>
#include <chrono>
#include <glib.h>

#include "hz/debug.h"
#include "hz/fs.h"

#include "loadsave.h"


// TODO gtkmm4: Port this to Glib::SignalTimeout



namespace rconfig {


namespace impl {
	
	inline hz::fs::path autosave_config_file;  ///< Config file to autosave to.
	inline bool autosave_enabled = false;  ///< Autosave enabled or not. This acts as a stopper flag for autosave callback.

}



extern "C" {

	/// Data for autosave_timeout_callback() function
	struct AutosaveCallbackData {
		bool force = true;
	};



	/// Autosave timeout callback for Glib. internal.
	inline gboolean autosave_timeout_callback(void* data)
	{
		bool force = false;
		if (data) {
			force = reinterpret_cast<AutosaveCallbackData*>(data)->force;
		}

		if (!force && !impl::autosave_enabled)  // no more autosaves
			return FALSE;  // remove timeout, disable autosave for real.

		auto file = impl::autosave_config_file;
		debug_out_info("rconfig", "Autosaving config to \"" << file << "\"." << std::endl);

		std::error_code ec;
		if ((hz::fs::exists(file, ec) && !hz::fs::is_regular_file(file, ec)) || !hz::fs_path_is_writable(file, ec)) {
			debug_out_error("rconfig", "Autosave failed: Cannot write to file:  " << ec.message() << std::endl);
			return static_cast<gboolean>(force);  // if manual, return failure. else, don't stop the timeout.
		}

		const bool status = rconfig::save_to_file(impl::autosave_config_file);
		if (force)
			return static_cast<gboolean>(status);  // return status to caller

		return TRUE;  // continue timeouts
	}

}




/// Set config file to autosave to.
inline bool autosave_set_config_file(const hz::fs::path& file)
{
	if (file.empty()) {
		debug_out_error("rconfig", DBG_FUNC_MSG << "Error: Filename is empty." << std::endl);
		return false;
	}

	impl::autosave_config_file = file;

	debug_out_info("rconfig", "Setting autosave config file to \"" << file << "\"." << std::endl);
	return true;
}



/// Enable autosave every \c sec_interval seconds.
inline bool autosave_start(std::chrono::seconds sec_interval)
{
	if (impl::autosave_enabled) {  // already autosaving, you should stop it first.
		debug_out_warn("rconfig", "Error while starting config autosave: Autosave is active already." << std::endl);
		return false;
	}

	impl::autosave_enabled = true;
	debug_out_info("rconfig", "Starting config autosave with " << sec_interval.count() << " sec. interval." << std::endl);

	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, guint(std::chrono::milliseconds(sec_interval).count()),
			&autosave_timeout_callback, nullptr, nullptr);

	return true;
}



/// Disable autosave
inline void autosave_stop()
{
	debug_out_info("rconfig", "Stopping config autosave." << std::endl);

	// set the stop flag. it will make autosave stop on next timeout callback call.
	impl::autosave_enabled = false;
}



/// Forcibly save the config now.
inline bool autosave_force_now()
{
	AutosaveCallbackData data;
	return static_cast<bool>(autosave_timeout_callback(&data));
}





}  // ns


#endif

/// @}
