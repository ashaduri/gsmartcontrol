/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_MAIN_WINDOW_H
#define GSC_MAIN_WINDOW_H

#include <map>
#include <gtkmm/window.h>
#include <gtkmm/menu.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/action.h>
#include <gtkmm/toggleaction.h>
#include <gtkmm/actiongroup.h>

#include "applib/app_ui_res_utils.h"
#include "applib/storage_device.h"
#include "applib/app_gtkmm_features.h"  // APP_GTKMM_CONNECT_VIRTUAL
#include "applib/wrapping_label.h"



class GscMainWindowIconView;  // defined in gsc_main_window_iconview.h

class GscInfoWindow;  // declared in gsc_info_window.h



// use create() / destroy() with this class instead of new / delete!

class GscMainWindow : public AppUIResWidget<GscMainWindow, false> {

	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_main_window);


		// glade/gtkbuilder needs this constructor
		GscMainWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);


		virtual ~GscMainWindow()
		{ }



		 // scan for devices and fill the iconview
		void rescan_devices();


		// manually add device file to icon list
		bool add_device(const std::string& file);


		// read smartctl data from file, add it as a virtual drive to icon list
		bool add_virtual_drive(const std::string& file);


		// if at least one drive is having a test performed, return true.
		bool testing_active() const;


		// show the info window for device
		GscInfoWindow* show_device_info_window(StorageDeviceRefPtr drive);


		// enable/disable items in Drive menu, set toggles in menu items
		void set_drive_menu_status(StorageDeviceRefPtr drive);


		Gtk::Menu* get_popup_menu(StorageDeviceRefPtr drive);


		void update_status_widgets();  // status area, etc...


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
			action_preferences,

			action_general_help,
			action_permission_problems,
			action_how_to_enable_smart,
			action_report_bug,
			action_about
		};


	protected:

		bool create_widgets();  // iconview, gtkuimanager stuff (menus), custom labels

		void populate_iconview(bool smartctl_valid);  // scan and populate iconview widget with drive icons


		void show_add_device_chooser();

		void show_load_virtual_file_chooser();



		GscMainWindowIconView* iconview;  // created by obj_create()
		std::vector<StorageDeviceRefPtr> drives;  // scanned drives

		Glib::RefPtr<Gtk::UIManager> ui_manager;
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_main;
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_device;
		bool action_handling_enabled_;
		std::map<action_t, Glib::RefPtr<Gtk::Action> > action_map;  // used by on_action_activated().

		WrappingLabel* name_label;
		WrappingLabel* health_label;
		WrappingLabel* family_label;

		bool scanning_;


		// -------------------- Callbacks


		// ---------- override virtual methods

		// by default, delete_event calls hide().
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks


// 		void on_action_activated(Glib::RefPtr<Gtk::Action> action, action_t action_type);
		void on_action_activated(action_t action_type);

		void on_action_enable_smart_toggled(Gtk::ToggleAction* action);

		void on_action_enable_aodc_toggled(Gtk::ToggleAction* action);

		void on_action_reread_device_data();


};






#endif
