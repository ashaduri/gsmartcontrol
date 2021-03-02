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

#ifndef GSC_PREFERENCES_WINDOW_H
#define GSC_PREFERENCES_WINDOW_H

#include <gtkmm.h>

#include "applib/app_builder_widget.h"



class GscPreferencesDeviceOptionsTreeView;  // defined in cpp file

class GscMainWindow;  // declared in gsc_main_window.h



/// The Preferences window.
/// Use create() / destroy() with this class instead of new / delete!
class GscPreferencesWindow : public AppBuilderWidget<GscPreferencesWindow, true> {
	public:

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline const std::string_view ui_name = "gsc_preferences_window";


		/// Constructor, GtkBuilder needs this.
		GscPreferencesWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);


		/// Set main window so that we can manipulate it
		void set_main_window(GscMainWindow* window);


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
		bool on_delete_event(GdkEventAny* e) override;


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

		GscMainWindow* main_window_ = nullptr;  ///< Main window that called us.

		GscPreferencesDeviceOptionsTreeView* device_options_treeview = nullptr;  ///< Device options tree view

};






#endif

/// @}
