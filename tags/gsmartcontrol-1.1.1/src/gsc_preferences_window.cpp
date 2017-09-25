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

#include <map>
#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape

#include "hz/fs_path.h"
#include "hz/string_sprintf.h"
#include "hz/scoped_ptr.h"
#include "rconfig/rconfig_mini.h"
#include "applib/storage_settings.h"
#include "applib/app_gtkmm_utils.h"
#include "gsc_main_window.h"

#include "gsc_preferences_window.h"




/// Device Options tree view of the Preferences window
class GscPreferencesDeviceOptionsTreeView : public Gtk::TreeView {
	public:

		typedef GscPreferencesDeviceOptionsTreeView self_type;  ///< Self type, needed for CONNECT_VIRTUAL


		/// Constructor, gtkbuilder/glade needs this.
		GscPreferencesDeviceOptionsTreeView(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
				: Gtk::TreeView(gtkcobj), preferences_window(0)
		{
			Gtk::TreeModelColumnRecord model_columns;

			// Device, Type, [Parameters], [Device Real], [Type Real].
			// Device may hold "<empty>", while Device Real is "".
			// Type may hold "<all>", while Type Real is "".

			model_columns.add(col_device);
			this->append_column("Device", col_device);
			this->set_search_column(col_device.index());

			model_columns.add(col_type);
			this->append_column("Type", col_type);

			model_columns.add(col_parameters);
			model_columns.add(col_device_real);
			model_columns.add(col_type_real);


			// create a TreeModel (ListStore)
			model = Gtk::ListStore::create(model_columns);
			model->set_sort_column(col_device, Gtk::SORT_ASCENDING);  // default sort
			this->set_model(model);

			Glib::RefPtr<Gtk::TreeSelection> selection = this->get_selection();

			selection->signal_changed().connect(sigc::mem_fun(*this,
					&self_type::on_selection_changed) );

		}


		/// Set the parent window
		void set_preferences_window(GscPreferencesWindow* w)
		{
			preferences_window = w;
		}


		/// Remove selected row
		void remove_selected_row()
		{
			if (this->get_selection()->count_selected_rows()) {
				Gtk::TreeIter iter = this->get_selection()->get_selected();
				model->erase(iter);
			}
		}


		/// Add a new row (for a new device)
		void add_new_row(const std::string& device, const std::string& type, const std::string& params, bool select = true)
		{
			Gtk::TreeRow row = *(model->append());
			row[col_device] = (device.empty() ? "<empty>" : device);
			row[col_type] = (type.empty() ? "<all>" : type);
			row[col_parameters] = params;
			row[col_device_real] = device;
			row[col_type_real] = type;

			if (select)
				this->get_selection()->select(row);
		}


		/// Update selected row device entry
		void update_selected_row_device(const std::string& device)
		{
			if (this->get_selection()->count_selected_rows()) {
				Gtk::TreeRow row = *(this->get_selection()->get_selected());
				row[col_device] = (device.empty() ? "<empty>" : device);
				row[col_device_real] = device;
			}
		}


		/// Update selected row type entry
		void update_selected_row_type(const std::string& type)
		{
			if (this->get_selection()->count_selected_rows()) {
				Gtk::TreeRow row = *(this->get_selection()->get_selected());
				row[col_type] = (type.empty() ? "<all>" : type);
				row[col_type_real] = type;
			}
		}


		/// Update selected row parameters entry
		void update_selected_row_params(const std::string& params)
		{
			if (this->get_selection()->count_selected_rows()) {
				Gtk::TreeRow row = *(this->get_selection()->get_selected());
				row[col_parameters] = params;
			}
		}


		/// Remove all rows
		void clear_all()
		{
			model->clear();
		}


		/// Check whether there is a row selected
		bool has_selected_row()
		{
			return this->get_selection()->count_selected_rows();
		}


		/// Set the device map (as loaded from config)
		void set_device_map(const device_option_map_t& devmap)
		{
			clear_all();
			for (device_option_map_t::const_iterator iter = devmap.begin(); iter != devmap.end(); ++iter) {
				std::vector<std::string> parts;
				hz::string_split(iter->first, "::", parts, 2);
				std::string dev = (parts.size() > 0 ? parts.at(0) : "");
				std::string type = (parts.size() > 1 ? parts.at(1) : "");
				std::string params = iter->second;
				this->add_new_row(dev, type, params, false);
			}
		}


		/// Get the device map (to be saved to config)
		device_option_map_t get_device_map()
		{
			device_option_map_t devmap;

			Gtk::TreeNodeChildren children = model->children();
			for (Gtk::TreeNodeChildren::iterator iter = children.begin(); iter != children.end(); ++iter) {
				Gtk::TreeModel::Row row = *iter;
				std::string dev = row.get_value(col_device_real);
				if (!dev.empty()) {
					std::string type = row.get_value(col_type_real);
					if (!type.empty()) {
						dev += "::" + type;
					}
					if (devmap.find(dev) == devmap.end()) {
						devmap[dev] = row.get_value(col_parameters);
					}
				}
			}

			return devmap;
		}



		/// Selection change callback
		void on_selection_changed()
		{
			std::string dev, type, par;
			if (this->get_selection()->count_selected_rows()) {
				Gtk::TreeRow row = *(this->get_selection()->get_selected());
				dev = row[col_device_real];
				type = row[col_type_real];
				par = row[col_parameters];
				preferences_window->device_widget_set_remove_possible(true);

			} else {
				preferences_window->device_widget_set_remove_possible(false);
			}

			preferences_window->update_device_widgets(dev, type, par);
		}


	private:

		Glib::RefPtr<Gtk::ListStore> model;  ///< The list model

		Gtk::TreeModelColumn<Glib::ustring> col_device;  ///< Model column
		Gtk::TreeModelColumn<Glib::ustring> col_type;  ///< Model column
		Gtk::TreeModelColumn<std::string> col_parameters;  ///< Model column
		Gtk::TreeModelColumn<std::string> col_device_real;  ///< Model column
		Gtk::TreeModelColumn<std::string> col_type_real;  ///< Model column

		GscPreferencesWindow* preferences_window;  ///< The parent window

};







GscPreferencesWindow::GscPreferencesWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
		: AppUIResWidget<GscPreferencesWindow, true>(gtkcobj, ref_ui), device_options_treeview(0)
{
	// Connect callbacks

	APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	Gtk::Button* window_cancel_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_cancel_button, clicked);

	Gtk::Button* window_ok_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_ok_button, clicked);

	Gtk::Button* window_reset_all_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_reset_all_button, clicked);


	Glib::ustring smartctl_binary_tooltip = "A path to smartctl binary. If the path is not absolute, the binary will be looked for in user's PATH.";
#if defined CONFIG_KERNEL_FAMILY_WINDOWS
	smartctl_binary_tooltip += Glib::ustring("\n") + "Note: smartctl.exe shows a console during execution, while smartctl-nc.exe (default) doesn't (nc means no-console).";
#endif
	if (Gtk::Label* smartctl_binary_label = lookup_widget<Gtk::Label*>("smartctl_binary_label")) {
		app_gtkmm_set_widget_tooltip(*smartctl_binary_label, smartctl_binary_tooltip);
	}
	if (Gtk::Entry* smartctl_binary_entry = lookup_widget<Gtk::Entry*>("smartctl_binary_entry")) {
		app_gtkmm_set_widget_tooltip(*smartctl_binary_entry, smartctl_binary_tooltip);
	}

	Gtk::Button* smartctl_binary_browse_button = 0;
	APP_UI_RES_AUTO_CONNECT(smartctl_binary_browse_button, clicked);


	Gtk::Button* device_options_add_device_button = 0;
	APP_UI_RES_AUTO_CONNECT(device_options_add_device_button, clicked);

	Gtk::Button* device_options_remove_device_button = 0;
	APP_UI_RES_AUTO_CONNECT(device_options_remove_device_button, clicked);


	Gtk::Entry* device_options_device_entry = 0;
	APP_UI_RES_AUTO_CONNECT(device_options_device_entry, changed);

	Glib::ustring device_options_tooltip = "A device name to match";
#if defined CONFIG_KERNEL_FAMILY_WINDOWS
	device_options_tooltip = "A device name to match (for example, use \"pd0\" for the first physical drive)";
#elif defined CONFIG_KERNEL_LINUX
	device_options_tooltip = "A device name to match (for example, /dev/sda or /dev/twa0)";
#endif
	if (Gtk::Label* device_options_device_label = lookup_widget<Gtk::Label*>("device_options_device_label")) {
		app_gtkmm_set_widget_tooltip(*device_options_device_label, device_options_tooltip);
	}
	if (device_options_device_entry) {
		app_gtkmm_set_widget_tooltip(*device_options_device_entry, device_options_tooltip);
	}


	Gtk::Entry* device_options_type_entry = 0;
	APP_UI_RES_AUTO_CONNECT(device_options_type_entry, changed);

	Gtk::Entry* device_options_parameter_entry = 0;
	APP_UI_RES_AUTO_CONNECT(device_options_parameter_entry, changed);


	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_cancel_button) {
		window_cancel_button->add_accelerator("clicked", accel_group, GDK_KEY_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


	// create Device Options treeview
	get_ui()->get_widget_derived("device_options_treeview", device_options_treeview);
	device_options_treeview->set_preferences_window(this);

	// we can't do this in treeview's constructor, it doesn't know about this window yet.
	this->device_widget_set_remove_possible(false);  // initial state

	// hide win32-only options for non-win32.
#ifndef _WIN32
	Gtk::CheckButton* smartctl_search_check = this->lookup_widget<Gtk::CheckButton*>("search_in_smartmontools_first_check");
	if (smartctl_search_check)
		smartctl_search_check->hide();
#endif


	// ---------------

	import_config();

	// show();
}



void GscPreferencesWindow::set_main_window(GscMainWindow* window)
{
	main_window_ = window;
}




void GscPreferencesWindow::update_device_widgets(const std::string& device, const std::string& type, const std::string& params)
{
	Gtk::Entry* entry = 0;

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_device_entry")))
		entry->set_text(device);

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_type_entry")))
		entry->set_text(type);

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_parameter_entry")))
		entry->set_text(params);
}



void GscPreferencesWindow::device_widget_set_remove_possible(bool b)
{
	Gtk::Button* button = 0;
	if ((button = this->lookup_widget<Gtk::Button*>("device_options_remove_device_button")))
		button->set_sensitive(b);
}



namespace {

	/// Set configuration in a smart way - don't set the defaults.
	template<typename T>
	inline void prefs_config_set(const std::string& path, const T& data)
	{
		T tmp = T();
		// we set the data only if one of the following is true:
		// 1. config node with that path already exists.
		// 2. data differs from default data.
		if (rconfig::get_config_data(path, tmp)) {
			rconfig::set_data(path, data);

		} else if (rconfig::get_default_data(path, tmp)) {
			if (tmp != data)
				rconfig::set_data(path, data);

		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Path \"" << path << "\" doesn't exist in config trees.\n");
		}
	}


	/// Get configuration for \c path
	template<typename T>
	inline bool prefs_config_get(const std::string& path, T& data)
	{
		if (rconfig::get_data(path, data))
			return true;

		debug_out_error("app", DBG_FUNC_MSG << "Path \"" << path << "\" doesn't exist in config trees.\n");
		return false;
	}

}



void GscPreferencesWindow::import_config()
{
	// Clear and fill the entries.

	Gtk::CheckButton* check = 0;
	Gtk::Entry* entry = 0;


	// ------- General tab

	bool scan_on_startup = 0;
	if ( prefs_config_get("gui/scan_on_startup", scan_on_startup)
			&& (check = this->lookup_widget<Gtk::CheckButton*>("scan_on_startup_check")) )
		check->set_active(scan_on_startup);

	bool show_smart_capable_only = 0;
	if ( prefs_config_get("gui/show_smart_capable_only", show_smart_capable_only)
			&& (check = this->lookup_widget<Gtk::CheckButton*>("show_smart_capable_only_check")) )
		check->set_active(show_smart_capable_only);

	bool icons_show_device_name = 0;
	if ( prefs_config_get("gui/icons_show_device_name", icons_show_device_name)
			&& (check = this->lookup_widget<Gtk::CheckButton*>("show_device_name_under_icon_check")) )
		check->set_active(icons_show_device_name);

	bool icons_show_serial_number = 0;
	if ( prefs_config_get("gui/icons_show_serial_number", icons_show_serial_number)
			&& (check = this->lookup_widget<Gtk::CheckButton*>("show_serial_number_under_icon_check")) )
		check->set_active(icons_show_serial_number);

	bool win32_search_smartctl_in_smartmontools = 0;
	if ( prefs_config_get("system/win32_search_smartctl_in_smartmontools", win32_search_smartctl_in_smartmontools)
			&& (check = this->lookup_widget<Gtk::CheckButton*>("search_in_smartmontools_first_check")) )
		check->set_active(win32_search_smartctl_in_smartmontools);

	std::string smartctl_binary;
	if ( prefs_config_get("system/smartctl_binary", smartctl_binary)
			&& (entry = this->lookup_widget<Gtk::Entry*>("smartctl_binary_entry")) )
		entry->set_text(smartctl_binary);

	std::string smartctl_options;
	if ( prefs_config_get("system/smartctl_options", smartctl_options)
			&& (entry = this->lookup_widget<Gtk::Entry*>("smartctl_options_entry")) )
		entry->set_text(smartctl_options);


	// ------- Drives tab

	std::string device_blacklist_patterns;
	if ( prefs_config_get("system/device_blacklist_patterns", device_blacklist_patterns)
			&& (entry = this->lookup_widget<Gtk::Entry*>("device_blacklist_patterns_entry")) )
		entry->set_text(device_blacklist_patterns);


	std::string devmap_str;
	if ( prefs_config_get("system/smartctl_device_options", devmap_str) ) {
		device_option_map_t devmap = app_unserialize_device_option_map(devmap_str);
		device_options_treeview->set_device_map(devmap);
	}

}



void GscPreferencesWindow::export_config()
{
	// we don't clear config here, it might contain non-dialog options too.

	Gtk::CheckButton* check = 0;
	Gtk::Entry* entry = 0;


	// ------- General tab

	if ((check = this->lookup_widget<Gtk::CheckButton*>("scan_on_startup_check")))
		prefs_config_set("gui/scan_on_startup", bool(check->get_active()));

	if ((check = this->lookup_widget<Gtk::CheckButton*>("show_smart_capable_only_check")))
		prefs_config_set("gui/show_smart_capable_only", bool(check->get_active()));

	if ((check = this->lookup_widget<Gtk::CheckButton*>("show_device_name_under_icon_check")))
		prefs_config_set("gui/icons_show_device_name", bool(check->get_active()));

	if ((check = this->lookup_widget<Gtk::CheckButton*>("show_serial_number_under_icon_check")))
		prefs_config_set("gui/icons_show_serial_number", bool(check->get_active()));

	if ((check = this->lookup_widget<Gtk::CheckButton*>("search_in_smartmontools_first_check")))
		prefs_config_set("system/win32_search_smartctl_in_smartmontools", bool(check->get_active()));

	if ((entry = this->lookup_widget<Gtk::Entry*>("smartctl_binary_entry")))
		prefs_config_set("system/smartctl_binary", std::string(entry->get_text()));

	if ((entry = this->lookup_widget<Gtk::Entry*>("smartctl_options_entry")))
		prefs_config_set("system/smartctl_options", std::string(entry->get_text()));


	// ------- Drives tab

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_blacklist_patterns_entry")))
		prefs_config_set("system/device_blacklist_patterns", std::string(entry->get_text()));

	device_option_map_t devmap = device_options_treeview->get_device_map();
	std::string devmap_str = app_serialize_device_option_map(devmap);
	prefs_config_set("system/smartctl_device_options", devmap_str);
}



bool GscPreferencesWindow::on_delete_event_before(GdkEventAny* e)
{
	destroy(this);  // deletes this object and nullifies instance
	return true;  // event handled, don't call default virtual handler
}



void GscPreferencesWindow::on_window_cancel_button_clicked()
{
	destroy(this);
}



void GscPreferencesWindow::on_window_ok_button_clicked()
{
	// Check if device map contains drives with empty device names or parameters.
	device_option_map_t devmap = device_options_treeview->get_device_map();
	bool contains_empty = false;
	for (device_option_map_t::const_iterator iter = devmap.begin(); iter != devmap.end(); ++iter) {
		if (iter->first.empty() || iter->second.empty()) {
			contains_empty = true;
		}
	}

	if (contains_empty) {
		Gtk::MessageDialog dialog(*this,
				"You have specified an empty Parameters field for one or more entries"
				" in Per-Drive Smartctl Parameters section. Such entries will be discarded.\n"
				"\nDo you want to continue?",
				true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		if (dialog.run() != Gtk::RESPONSE_YES) {
			return;
		}
	}

	export_config();

	if (main_window_) {
		main_window_->show_prefs_updated_message();
	}

	destroy(this);
}



void GscPreferencesWindow::on_window_reset_all_button_clicked()
{
	Gtk::MessageDialog dialog(*this,
			"\nAre you sure you want to reset all program settings to their defaults?\n",
			true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

	if (dialog.run() == Gtk::RESPONSE_YES) {
		rconfig::clear_config_all();
		import_config();
		// close the window, because the user might get the impression that "Cancel" will revert.
		destroy(this);
	}
}



void GscPreferencesWindow::on_smartctl_binary_browse_button_clicked()
{
	std::string default_file;

	Gtk::Entry* entry = this->lookup_widget<Gtk::Entry*>("smartctl_binary_entry");
	if (!entry)
		return;

	hz::FsPath path(entry->get_text());

	int result = 0;

#ifdef _WIN32
	Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
	specific_filter->set_name("Executable Files");
	specific_filter->add_pattern("*.exe");

	Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
	all_filter->set_name("All Files");
	all_filter->add_pattern("*");
#endif

#if GTK_CHECK_VERSION(3, 20, 0)
	hz::scoped_ptr<GtkFileChooserNative> dialog(gtk_file_chooser_native_new(
			"Choose Smartctl Binary...", this->gobj(), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, NULL), g_object_unref);

	if (path.is_absolute())
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog.get()), path.c_str());

#ifdef _WIN32
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());
#endif

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
	Gtk::FileChooserDialog dialog(*this, "Choose Smartctl Binary...",
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

#ifdef _WIN32
	dialog.add_filter(specific_filter);
	dialog.add_filter(all_filter);
#endif

	// Note: This works on absolute paths only (otherwise it's gtk warning).
	if (path.is_absolute())
		dialog.set_filename(path.str());  // change to its dir and select it if exists.

	// Show the dialog and wait for a user response
	result = dialog.run();  // the main cycle blocks here
#endif

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			Glib::ustring file;
#if GTK_CHECK_VERSION(3, 20, 0)
			file = app_ustring_from_gchar(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog.get())));
#else
			file = dialog.get_filename();  // in fs encoding
#endif
			entry->set_text(file);
			break;
		}

		case Gtk::RESPONSE_CANCEL: case Gtk::RESPONSE_DELETE_EVENT:
			// nothing, the dialog is closed already
			break;

		default:
			debug_out_error("app", DBG_FUNC_MSG << "Unknown dialog response code: " << result << ".\n");
			break;
	}

}




void GscPreferencesWindow::on_device_options_remove_device_button_clicked()
{
	device_options_treeview->remove_selected_row();
}



void GscPreferencesWindow::on_device_options_add_device_button_clicked()
{
	Gtk::Entry* entry = 0;
	std::string dev, type, par;

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_device_entry")))
		dev = entry->get_text();

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_type_entry")))
		type = entry->get_text();

	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_parameter_entry")))
		par = entry->get_text();

	if (device_options_treeview->has_selected_row()) {
		device_options_treeview->add_new_row("", "", "");  // without this it would clone the existing.
	} else {
		device_options_treeview->add_new_row(dev, type, par);
	}
}



void GscPreferencesWindow::on_device_options_device_entry_changed()
{
	Gtk::Entry* entry = 0;
	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_device_entry")))
		device_options_treeview->update_selected_row_device(entry->get_text());
}



void GscPreferencesWindow::on_device_options_type_entry_changed()
{
	Gtk::Entry* entry = 0;
	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_type_entry")))
		device_options_treeview->update_selected_row_type(entry->get_text());
}



void GscPreferencesWindow::on_device_options_parameter_entry_changed()
{
	Gtk::Entry* entry = 0;
	if ((entry = this->lookup_widget<Gtk::Entry*>("device_options_parameter_entry")))
		device_options_treeview->update_selected_row_params(entry->get_text());
}








/// @}
