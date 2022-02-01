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

#ifndef GSC_INFO_WINDOW_H
#define GSC_INFO_WINDOW_H

#include <gtkmm.h>
#include <map>

#include "applib/app_builder_widget.h"
#include "applib/storage_device.h"
#include "applib/selftest.h"




/// The "Drive Information" window.
/// Use create() / destroy() with this class instead of new / delete!
class GscInfoWindow : public AppBuilderWidget<GscInfoWindow, true> {
	public:

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline const std::string_view ui_name = "gsc_info_window";


		/// Constructor, GtkBuilder needs this.
		GscInfoWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);

		/// Virtual destructor
		~GscInfoWindow() override;


		/// Set the drive to show
		void set_drive(StorageDevicePtr d);

		/// Fill the dialog with info from the drive
		void fill_ui_with_info(bool scan = true, bool clear_ui = true, bool clear_tests = true);

		/// Clear all info in UI
		void clear_ui_info(bool clear_tests_too = true);

		/// Refresh the drive information in UI
		void refresh_info(bool clear_tests_too = true);


		// Show the Tests tab. Called by main window.
		void show_tests();


	protected:

		/// fill_ui_with_info() helper
		void fill_ui_general(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		void fill_ui_attributes(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		void fill_ui_statistics(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		void fill_ui_self_test_info();

		/// fill_ui_with_info() helper
		void fill_ui_self_test_log(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		void fill_ui_error_log(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		void fill_ui_temperature_log(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		WarningLevel fill_ui_capabilities(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		WarningLevel fill_ui_error_recovery(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		WarningLevel fill_ui_selective_self_test_log(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		WarningLevel fill_ui_physical(const std::vector<AtaStorageProperty>& props);

		/// fill_ui_with_info() helper
		WarningLevel fill_ui_directory(const std::vector<AtaStorageProperty>& props);


		// ---------- Helpers

		/// Cell renderer function for a table
		void cell_renderer_for_attributes(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& iter,
				int column_index) const;

		/// Cell renderer function for a table
		void cell_renderer_for_statistics(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& iter,
				int column_index) const;

		/// Cell renderer function for a table
		void cell_renderer_for_self_test_log(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& iter,
				int column_index) const;

		/// Cell renderer function for a table
		void cell_renderer_for_error_log(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& iter,
				int column_index) const;

		/// Cell renderer function for a table
		void cell_renderer_for_capabilities(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& iter,
				int column_index) const;


		// ---------- Callbacks

		/// An idle callback to update the status while the test is running.
		static gboolean test_idle_callback(void* data);


		// ---------- Overridden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// If a test is running, show a question dialog first.
		/// Reimplemented from Gtk::Window.
		bool on_delete_event(GdkEventAny* e) override;


		// ---------- Other callbacks

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

		// ---------- Connections

		sigc::connection error_log_row_selected_conn;  ///< Callback connection

		sigc::connection test_type_combo_changed_conn;  ///< Callback connection

		sigc::connection drive_changed_connection;  // Callback connection of drive's signal_changed callback


		// ---------- Data members

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

		Gtk::Label* device_name_label = nullptr;  ///< Top label

		StorageDevicePtr drive;  ///< The drive we're showing

		std::shared_ptr<SelfTest> current_test;  ///< Currently running test, or 0.

		// Test idle callback temporaries
		std::string test_error_msg;  ///< Our errors
		Glib::Timer test_timer_poll;  ///< Timer for testing phase
		Glib::Timer test_timer_bar;  ///< Timer for testing phase
		bool test_force_bar_update = false;  ///< Helper for testing callback

		/// Attributes table model columns
		struct {
			Gtk::TreeModelColumn<int32_t> id;
			Gtk::TreeModelColumn<Glib::ustring> displayable_name;
			Gtk::TreeModelColumn<Glib::ustring> when_failed;
			Gtk::TreeModelColumn<std::string> normalized_value;
			Gtk::TreeModelColumn<std::string> worst;
			Gtk::TreeModelColumn<std::string> threshold;
			Gtk::TreeModelColumn<std::string> raw;
			Gtk::TreeModelColumn<Glib::ustring> type;
			// Gtk::TreeModelColumn<Glib::ustring> updated;
			Gtk::TreeModelColumn<std::string> flag_value;
			Gtk::TreeModelColumn<Glib::ustring> tooltip;
			Gtk::TreeModelColumn<const AtaStorageProperty*> storage_property;
		} attribute_table_columns;

		/// Statistics table model columns
		struct {
			Gtk::TreeModelColumn<Glib::ustring> displayable_name;
			Gtk::TreeModelColumn<std::string> value;
			Gtk::TreeModelColumn<std::string> flags;
			Gtk::TreeModelColumn<std::string> page_offset;
			Gtk::TreeModelColumn<Glib::ustring> tooltip;
			Gtk::TreeModelColumn<const AtaStorageProperty*> storage_property;
		} statistics_table_columns;

		/// Self-test log table model columns
		struct {
			Gtk::TreeModelColumn<uint32_t> log_entry_index;
			Gtk::TreeModelColumn<std::string> type;
			Gtk::TreeModelColumn<std::string> status;
			Gtk::TreeModelColumn<std::string> percent;
			Gtk::TreeModelColumn<std::string> hours;
			Gtk::TreeModelColumn<std::string> lba;
			Gtk::TreeModelColumn<Glib::ustring> tooltip;
			Gtk::TreeModelColumn<const AtaStorageProperty*> storage_property;
		} self_test_log_table_columns;

		/// Error log table model columns
		struct {
			Gtk::TreeModelColumn<uint32_t> log_entry_index;
			Gtk::TreeModelColumn<std::string> hours;
			Gtk::TreeModelColumn<std::string> state;
			Gtk::TreeModelColumn<Glib::ustring> type;
			Gtk::TreeModelColumn<std::string> details;
			Gtk::TreeModelColumn<Glib::ustring> tooltip;
			Gtk::TreeModelColumn<const AtaStorageProperty*> storage_property;
			Gtk::TreeModelColumn<Glib::ustring> mark_name;
		} error_log_table_columns;

		/// Capabilities table model columns
		struct {
			Gtk::TreeModelColumn<int> entry_index;
			Gtk::TreeModelColumn<Glib::ustring> name;
			Gtk::TreeModelColumn<std::string> flag_value;
			Gtk::TreeModelColumn<Glib::ustring> str_values;
			Gtk::TreeModelColumn<Glib::ustring> tooltip;
			Gtk::TreeModelColumn<const AtaStorageProperty*> storage_property;
		} capabilities_table_columns;

		// "Test type" combobox columns
		struct {
			Gtk::TreeModelColumn<Glib::ustring> name;  ///< Combobox model column
			Gtk::TreeModelColumn<Glib::ustring> description;  ///< Combobox model column
			Gtk::TreeModelColumn<std::shared_ptr<SelfTest>> self_test;  ///< Combobox model column
		} test_combo_columns;

		Glib::RefPtr<Gtk::ListStore> test_combo_model;  ///< Combobox model

};






#endif

/// @}
