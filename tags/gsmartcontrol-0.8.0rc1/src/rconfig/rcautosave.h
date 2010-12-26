/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef RCONFIG_RCAUTOSAVE_H
#define RCONFIG_RCAUTOSAVE_H


// Autosave functions are only available if GLib is enabled.
#ifdef ENABLE_GLIB


#include <string>
#include <glib.h>

#include "hz/debug.h"
#include "hz/sync.h"
#include "hz/sync_policy_glib.h"
#include "hz/fs_path.h"

#include "rcloadsave.h"



namespace rconfig {


// We use glib because it's available already and we _need_ threading.
// Glib callbacks are executed in separate threads, so threading should
// be available. Don't forget to initialize GThread!
typedef hz::SyncPolicyGlib AutoSaveLockPolicy;


// specify the same type to get the same set of variables.
template<typename Dummy>
struct AutoSaveStaticHolder {
	static std::string config_file;  // config file to autosave to.
	static bool autosave_enabled;  // autosave enabled or not. this acts as a stopper flag for autosave callback.
	static AutoSaveLockPolicy::Mutex mutex;  // mutex for variables above.
};

// definitions
template<typename Dummy> std::string AutoSaveStaticHolder<Dummy>::config_file;
template<typename Dummy> bool AutoSaveStaticHolder<Dummy>::autosave_enabled = false;
template<typename Dummy> AutoSaveLockPolicy::Mutex AutoSaveStaticHolder<Dummy>::mutex;


typedef AutoSaveStaticHolder<void> AutoSaveHolder;  // one (and only) specialization.



extern "C" {

	// timeout callback. internal.
	inline gboolean autosave_timeout_callback(gpointer data)
	{
		bool force = (bool)data;

		// If previous call is active, return (may happen with too small timeout?).
		// Don't do this for forced call, because it may expect the file to be updated
		// once this function exits.
		if (!force) {
			if (!AutoSaveLockPolicy::trylock(AutoSaveHolder::mutex))  // can't lock, another save active, exit.
				return true;  // automatically try next time
			AutoSaveLockPolicy::unlock(AutoSaveHolder::mutex);  // unlock so we can use the scoped lock.
		}

		// manually called or no parallel threads saving:

		// lock all static vars of this file.
		AutoSaveLockPolicy::ScopedLock locker(AutoSaveHolder::mutex);

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




// set config file to autosave to.
inline bool autosave_set_config_file(const std::string& file)
{
	if (file.empty()) {
		debug_print_error("rconfig", "autosave_set_config_file(): Error: Filename is empty.\n");
		return false;
	}

	AutoSaveLockPolicy::ScopedLock locker(AutoSaveHolder::mutex);
	AutoSaveHolder::config_file = file;

	debug_print_info("rconfig", "Setting autosave config file to \"%s\"\n", file.c_str());
	return true;
}



// enable autosave
inline bool autosave_start(unsigned int sec_interval)
{
	AutoSaveLockPolicy::ScopedLock locker(AutoSaveHolder::mutex);

	if (AutoSaveHolder::autosave_enabled) {  // already autosaving, you should stop it first.
		debug_print_warn("rconfig", "Error while starting config autosave: Autosave is active already.\n");
		return false;
	}

	AutoSaveHolder::autosave_enabled = true;
	debug_print_info("rconfig", "Starting config autosave with %u sec. interval.\n", sec_interval);

	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, sec_interval*1000,
			&autosave_timeout_callback, NULL, NULL);

	return true;
}



// disable autosave
inline void autosave_stop()
{
	debug_print_info("rconfig", "Stopping config autosave.\n");

	AutoSaveLockPolicy::ScopedLock locker(AutoSaveHolder::mutex);

	// set the stop flag. it will make autosave stop on next timeout callback call.
	AutoSaveHolder::autosave_enabled = false;
}



// force saving of config
inline bool autosave_force_now()
{
	return autosave_timeout_callback((void*)true);  // anyone tell me what is the C++ variant of this?
}





}  // ns



#endif  // glib


#endif
