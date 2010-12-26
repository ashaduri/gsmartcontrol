/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_INFO_WINDOW_H
#define GSC_INFO_WINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/liststore.h>
#include <glibmm/ustring.h>

#include "applib/app_ui_res_utils.h"
#include "applib/storage_device.h"
#include "applib/wrapping_label.h"  // WrappingLabel
#include "applib/selftest.h"




// use create() / destroy() with this class instead of new / delete!

class GscInfoWindow : public AppUIResWidget<GscInfoWindow, true> {

	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_info_window);


		// glade/gtkbuilder needs this constructor
		GscInfoWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);


		virtual ~GscInfoWindow()
		{ }


		void set_drive(StorageDeviceRefPtr d)
		{
			if (drive)  // if an old drive is present, disconnect our callback from it.
				drive_changed_connection.disconnect();
			drive = d;
			drive_changed_connection = drive->signal_changed.connect(sigc::mem_fun(this,
					&GscInfoWindow::on_drive_changed));
		}


		// fill the dialog with info from "drive"
		void fill_ui_with_info(bool scan = true, bool clear_ui = true, bool clear_tests = true);

		// clear all info
		void clear_ui_info(bool clear_tests_too = true);


		void refresh_info(bool clear_tests_too = true)
		{
			this->set_sensitive(false);  // make insensitive until filled. helps with pressed F5 problem.

			// this->clear_ui_info();  // no need, fill_ui_with_info() will call it.
			this->fill_ui_with_info(true, true, clear_tests_too);

			this->set_sensitive(true);  // make sensitive again.
		}


		// show tests tab. called by main window.
		void show_tests();


	protected:


		// -------------------- Callbacks

		static gboolean test_idle_callback(void* data);


		// ---------- override virtual methods

		// by default, delete_event calls hide().
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks

		void on_refresh_info_button_clicked()
		{
			this->refresh_info();
		}

		void on_view_output_button_clicked();

		void on_save_info_button_clicked();

		void on_close_window_button_clicked();


		void on_test_type_combo_changed();

		void on_test_execute_button_clicked();

		void on_test_stop_button_clicked();


		// Callback attached to StorageDevice.
		void on_drive_changed(StorageDevice* pdrive);


		// --------- Connections

		sigc::connection error_log_row_selected_conn;

		sigc::connection test_type_combo_changed_conn;

		sigc::connection drive_changed_connection;  // connection of drive's signal_changed callback



		// --------- Data members

		// tab headers, to perform their coloration
		Glib::ustring tab_identity_name;
		Glib::ustring tab_capabilities_name;
		Glib::ustring tab_attributes_name;
		Glib::ustring tab_error_log_name;
		Glib::ustring tab_selftest_log_name;
		Glib::ustring tab_test_name;

		WrappingLabel* device_name_label;  // top label

		StorageDeviceRefPtr drive;  // represented drive.

		SelfTestPtr current_test;  // currently running test, or 0.

		// test idle callback temporaries
		std::string test_error_msg;  // our errors
		Glib::Timer test_timer_poll;
		Glib::Timer test_timer_bar;
		bool test_force_bar_update;


		// test type combobox stuff
		Gtk::TreeModelColumn<Glib::ustring> test_combo_col_name;
		Gtk::TreeModelColumn<Glib::ustring> test_combo_col_description;
		Gtk::TreeModelColumn<SelfTestPtr> test_combo_col_self_test;
		Glib::RefPtr<Gtk::ListStore> test_combo_model;

};






#endif
