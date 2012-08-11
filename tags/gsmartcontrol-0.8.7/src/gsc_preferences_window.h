/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_PREFERENCES_WINDOW_H
#define GSC_PREFERENCES_WINDOW_H

#include <gtkmm/window.h>

#include "applib/app_ui_res_utils.h"



class GscPreferencesDeviceOptionsTreeView;  // defined in cpp file



/// The Preferences window.
/// Use create() / destroy() with this class instead of new / delete!
class GscPreferencesWindow : public AppUIResWidget<GscPreferencesWindow, true> {
	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_preferences_window);


		/// Constructor, gtkbuilder/glade needs this.
		GscPreferencesWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);

		/// Virtual destructor
		virtual ~GscPreferencesWindow()
		{ }


		/// Update the device widgets (per-device parameters), used by GscPreferencesDeviceOptionsTreeView.
		void update_device_widgets(const std::string& device, const std::string& type, const std::string& params);

		/// Set whether to allow removing device entries, used by GscPreferencesDeviceOptionsTreeView.
		void device_widget_set_remove_possible(bool b);


	protected:

		/// Import the configuration into UI
		void import_config();

		/// Export the configuration from UI
		void export_config();


		// -------------------- callbacks

		// ---------- overriden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks

		/// Button click callback
		void on_window_cancel_button_clicked();

		/// Button click callback
		void on_window_ok_button_clicked();

		/// Button click callback
		void on_window_reset_all_button_clicked();


		/// Button click callback
		void on_smartctl_binary_browse_button_clicked();

		/// Button click callback
		void on_device_options_remove_device_button_clicked();

		/// Button click callback
		void on_device_options_add_device_button_clicked();

		/// Callback
		void on_device_options_device_entry_changed();

		/// Callback
		void on_device_options_type_entry_changed();

		/// Callback
		void on_device_options_parameter_entry_changed();


	private:

		GscPreferencesDeviceOptionsTreeView* device_options_treeview;  ///< Device options tree view

};






#endif

/// @}
