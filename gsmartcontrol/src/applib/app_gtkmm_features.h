/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_GTKMM_FEATURES_H
#define APP_GTKMM_FEATURES_H

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // NOT NEEDED
#undef throw

#include <gtkmm.h>


/// \def APP_GTKMM_CHECK_VERSION(major, minor, micro)
/// Similar to GTK_CHECK_VERSION, but for gtkmm, which lacks this for some reason.
#ifndef APP_GTKMM_CHECK_VERSION
	#define APP_GTKMM_CHECK_VERSION(major, minor, micro) \
		(GTKMM_MAJOR_VERSION > (major) \
			|| (GTKMM_MAJOR_VERSION == (major) && (GTKMM_MINOR_VERSION > (minor) \
				|| (GTKMM_MINOR_VERSION == (minor) && GTKMM_MICRO_VERSION >= (micro)) \
				) \
			) \
		)
#endif




// If default virtual on_* members are disabled in gtkmm, use this in constructors
// to connect on_signal_name methods.

// Note: Some signals may be missing from earlier gtkmm versions. E.g. gtkmm 2.6
// doesn't have on_delete_event virtual function to override. In such cases it's better
// to just connect a non-virtual signal.
/*
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	// nothing
	#define APP_GTKMM_CONNECT_VIRTUAL(signal_name)

#else

	#define APP_GTKMM_CONNECT_VIRTUAL(signal_name) \
		this->signal_ ## signal_name ().connect(sigc::mem_fun(*this, &self_type::on_ ## signal_name))

#endif
*/


/// Connect to a signal _before_ the default handler. That is, if you want
/// to have, say, on_delete_event() in your window-inherited class, define
/// on_delete_event_before() instead and return true (handled) from it
/// if it's X event handler.
/// This method will work regardless of presence of default virtual handlers
/// in parent class. It will also avoid calling the handler twice (one from
/// signal, another from default virtual handler).
#define APP_GTKMM_CONNECT_VIRTUAL(signal_name) \
	this->signal_ ## signal_name ().connect(sigc::mem_fun(*this, &self_type::on_ ## signal_name ## _before), false)






#endif

/// @}
