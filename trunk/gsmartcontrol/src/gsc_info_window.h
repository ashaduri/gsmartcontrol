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

#ifndef GSC_INFO_WINDOW_H
#define GSC_INFO_WINDOW_H

#include <gtkmm.h>
#include <map>

#include "applib/app_ui_res_utils.h"
#include "applib/storage_device.h"
#include "applib/selftest.h"




/// The "Drive Information" window.
/// Use create() / destroy() with this class instead of new / delete!
class GscInfoWindow : public AppUIResWidget<GscInfoWindow, true> {
	public:

		// name of ui file without a .ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_info_window);


		/// Constructor, GtkBuilder needs this.
		GscInfoWindow(BaseObjectType* gtkcobj, const Glib::RefPtr<Gtk::Builder>& ref_ui);

		/// Virtual destructor
		virtual ~GscInfoWindow();

		// Overridden from InstanceManager
		void obj_destroy() override;


		/// Set the drive to show
		void set_drive(StorageDeviceRefPtr d);

		/// Fill the dialog with info from the drive
		void fill_ui_with_info(bool scan = true, bool clear_ui = true, bool clear_tests = true);

		/// Clear all info in UI
		void clear_ui_info(bool clear_tests_too = true);

		/// Refresh the drive information in UI
		void refresh_info(bool clear_tests_too = true);


		// Show the Tests tab. Called by main window.
		void show_tests();


	protected:


		// -------------------- callbacks

		/// An idle callback to update the status while the test is running.
		static gboolean test_idle_callback(void* data);


		// ---------- overriden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// If a test is running, show a question dialog first.
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks

		/// Button click callback
		void on_refresh_info_button_clicked();

		/// Button click callback
		void on_view_output_button_clicked();

		/// Button click callback
		void on_save_info_button_clicked();

		/// Button click callback
		void on_close_window_button_clicked();


		/// Callback
		void on_test_type_combo_changed();

		/// Button click callback
		void on_test_execute_button_clicked();

		/// Button click callback
		void on_test_stop_button_clicked();


		/// Callback attached to StorageDevice change signal.
		void on_drive_changed(StorageDevice* pdrive);

		/// Callback
		bool on_treeview_button_press_event(GdkEventButton* button_event, Gtk::Menu* menu, Gtk::TreeView* treeview);

		/// Callback
		void on_treeview_menu_copy_clicked(Gtk::TreeView* treeview);


	private:

		// --------- Connections

		sigc::connection error_log_row_selected_conn;  ///< Callback connection

		sigc::connection test_type_combo_changed_conn;  ///< Callback connection

		sigc::connection drive_changed_connection;  // Callback connection of drive's signal_changed callback


		// --------- Data members

		std::map<std::string, Gtk::Menu*> treeview_menus;  ///< Context menus

		// tab headers, to perform their coloration
		Glib::ustring tab_identity_name;  ///< Tab header name
		Glib::ustring tab_attributes_name;  ///< Tab header name
		Glib::ustring tab_statistics_name;  ///< Tab header name
		Glib::ustring tab_test_name;  ///< Tab header name
		Glib::ustring tab_error_log_name;  ///< Tab header name
		Glib::ustring tab_temperature_name;  ///< Tab header name
		Glib::ustring tab_advanced_name;  ///< Tab header name

		Glib::ustring tab_capabilities_name;  ///< Tab header name
		Glib::ustring tab_erc_name;  ///< Tab header name
		Glib::ustring tab_selective_selftest_name;  ///< Tab header name
		Glib::ustring tab_phy_name;  ///< Tab header name
		Glib::ustring tab_directory_name;  ///< Tab header name

		Gtk::Label* device_name_label;  ///< Top label

		StorageDeviceRefPtr drive;  ///< The drive we're showing

		SelfTestPtr current_test;  ///< Currently running test, or 0.

		// test idle callback temporaries
		std::string test_error_msg;  ///< Our errors
		Glib::Timer test_timer_poll;  ///< Timer for testing phase
		Glib::Timer test_timer_bar;  ///< Timer for testing phase
		bool test_force_bar_update;  ///< Helper for testing callback


		// test type combobox stuff
		Gtk::TreeModelColumn<Glib::ustring> test_combo_col_name;  ///< Combobox model column
		Gtk::TreeModelColumn<Glib::ustring> test_combo_col_description;  ///< Combobox model column
		Gtk::TreeModelColumn<SelfTestPtr> test_combo_col_self_test;  ///< Combobox model column
		Glib::RefPtr<Gtk::ListStore> test_combo_model;  ///< Combobox model

};






#endif

/// @}
