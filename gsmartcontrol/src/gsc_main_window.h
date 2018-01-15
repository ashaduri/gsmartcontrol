/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_MAIN_WINDOW_H
#define GSC_MAIN_WINDOW_H

#include <map>
#include <gtkmm.h>

#include "applib/app_ui_res_utils.h"
#include "applib/storage_device.h"
#include "applib/app_gtkmm_features.h"  // APP_GTKMM_CONNECT_VIRTUAL



class GscMainWindowIconView;  // defined in gsc_main_window_iconview.h

class GscInfoWindow;  // declared in gsc_info_window.h



/// The main window.
/// Use create() / destroy() with this class instead of new / delete!
class GscMainWindow : public AppUIResWidget<GscMainWindow, false> {
	public:

		friend class GscMainWindowIconView;  // It needs our privates

		// name of ui file without a .ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_main_window);


		/// Constructor, GtkBuilder needs this.
		GscMainWindow(BaseObjectType* gtkcobj, const Glib::RefPtr<Gtk::Builder>& ref_ui);


		// Overridden from InstanceManager
		void obj_destroy() override;


		/// Scan for devices and fill the iconview
		void rescan_devices();


		/// Execute update-smart-drivedb
		void run_update_drivedb();


		/// Manually add device file to icon list
		bool add_device(const std::string& file, const std::string& type_arg, const std::string& extra_args);


		/// Read smartctl data from file, add it as a virtual drive to icon list
		bool add_virtual_drive(const std::string& file);


		/// If at least one drive is having a test performed, return true.
		bool testing_active() const;


		/// Show the info window for the drive
		GscInfoWindow* show_device_info_window(const StorageDevicePtr& drive);

		/// Show "Preferences updated, please rescan" message
		void show_prefs_updated_message();


	protected:

		/// Action type
		enum action_t {
			action_quit,

			action_view_details,
			action_enable_smart,
			action_enable_aodc,
			action_reread_device_data,
			action_perform_tests,
			action_remove_device,
			action_remove_virtual_device,
			action_add_device,
			action_load_virtual,
			action_rescan_devices,

			action_executor_log,
			action_update_drivedb,
			action_preferences,

			action_online_documentation,
			action_support,
			action_about
		};


		/// Enable/disable items in Drive menu, set toggles in menu items
		void set_drive_menu_status(const StorageDevicePtr& drive);

		/// Get popup menu for a drive
		Gtk::Menu* get_popup_menu(const StorageDevicePtr& drive);

		/// Update status widgets (status area, etc...)
		void update_status_widgets();


		/// Create the widgets - iconview, gtkuimanager stuff (menus), custom labels
		bool create_widgets();

		/// scan and populate iconview widget with drive icons
		void populate_iconview(bool smartctl_valid);

		/// Show "Add Device" window
		void show_add_device_chooser();

		/// Show "Load Virtual File" dialog
		void show_load_virtual_file_chooser();


		/// Called when quit has been requested (by delete event or Quit action)
		bool quit_requested();


		// -------------------- callbacks

		// ---------- overriden virtual methods

		/// Quit the application on delete event (by default it calls hide()).
		/// If some test is running, show a question dialog first.
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks


// 		void on_action_activated(Glib::RefPtr<Gtk::Action> action, action_t action_type);

		/// Action activate callback
		void on_action_activated(action_t action_type);

		/// Action callback
		void on_action_enable_smart_toggled(Gtk::ToggleAction* action);

		/// Action callback
		void on_action_enable_aodc_toggled(Gtk::ToggleAction* action);

		/// Action callback
		void on_action_reread_device_data();


	private:

		GscMainWindowIconView* iconview;  ///< The main icon view, as created by obj_create()
		std::vector<StorageDevicePtr> drives;  ///< Scanned drives

		Glib::RefPtr<Gtk::UIManager> ui_manager;  ///< UI manager
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_main;  ///< Action group
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_device;  ///< Action group
		bool action_handling_enabled_;  ///< Whether action handling is enabled or not
		std::map<action_t, Glib::RefPtr<Gtk::Action> > action_map;  ///< Used by on_action_activated().

		Gtk::Label* name_label;  ///< A UI label
		Gtk::Label* health_label;  ///< A UI label
		Gtk::Label* family_label;  ///< A UI label

		bool scanning_;  ///< If the scanning is in process or not

};






#endif

/// @}
