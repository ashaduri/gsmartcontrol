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

#include <glibmm.h>
#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape
#include <map>
#include <type_traits>  // std::decay_t
#include <memory>

#include "build_config.h"
#include "hz/fs_ns.h"
#include "hz/string_sprintf.h"
#include "rconfig/rconfig.h"
#include "applib/storage_settings.h"
#include "applib/app_gtkmm_tools.h"
#include "gsc_main_window.h"

#include "gsc_preferences_window.h"



using namespace std::literals;


/// Device Options tree view of the Preferences window
class GscPreferencesDeviceOptionsTreeView : public Gtk::TreeView {
	public:

		/// Constructor, GtkBuilder needs this.
		GscPreferencesDeviceOptionsTreeView(BaseObjectType* gtkcobj, [[maybe_unused]] const Glib::RefPtr<Gtk::Builder>& ref_ui)
				: Gtk::TreeView(gtkcobj)
		{
			Gtk::TreeModelColumnRecord model_columns;

			// Device, Type, [Parameters], [Device Real], [Type Real].
			// Device may hold "<empty>", while Device Real is "".
			// Type may hold "<all>", while Type Real is "".

			model_columns.add(col_device);
			this->append_column(_("Device"), col_device);
			this->set_search_column(col_device.index());

			model_columns.add(col_type);
			this->append_column(_("Type"), col_type);

			model_columns.add(col_parameters);
			model_columns.add(col_device_real);
			model_columns.add(col_type_real);


			// create a TreeModel (ListStore)
			model = Gtk::ListStore::create(model_columns);
			model->set_sort_column(col_device, Gtk::SORT_ASCENDING);  // default sort
			this->set_model(model);

			Glib::RefPtr<Gtk::TreeSelection> selection = this->get_selection();

			selection->signal_changed().connect(sigc::mem_fun(*this,
					&GscPreferencesDeviceOptionsTreeView::on_selection_changed) );
		}


		/// Set the parent window
		void set_preferences_window(GscPreferencesWindow* w)
		{
			preferences_window = w;
		}


		/// Remove selected row
		void remove_selected_row()
		{
			if (this->get_selection()->count_selected_rows() > 0) {
				Gtk::TreeIter iter = this->get_selection()->get_selected();
				model->erase(iter);
			}
		}


		/// Add a new row (for a new device)
		void add_new_row(const std::string& device, const std::string& type, const std::string& params, bool select = true)
		{
			Gtk::TreeRow row = *(model->append());
			row[col_device] = (device.empty() ? "<"s + C_("name", "empty") + ">" : device);
			row[col_type] = (type.empty() ? "<"s + C_("types", "all") + ">" : type);
			row[col_parameters] = params;
			row[col_device_real] = device;
			row[col_type_real] = type;

			if (select)
				this->get_selection()->select(row);
		}


		/// Update selected row device entry
		void update_selected_row_device(const std::string& device)
		{
			if (this->get_selection()->count_selected_rows() > 0) {
				Gtk::TreeRow row = *(this->get_selection()->get_selected());
				row[col_device] = (device.empty() ? "<"s + C_("name", "empty") + ">" : device);
				row[col_device_real] = device;
			}
		}


		/// Update selected row type entry
		void update_selected_row_type(const std::string& type)
		{
			if (this->get_selection()->count_selected_rows() > 0) {
				Gtk::TreeRow row = *(this->get_selection()->get_selected());
				row[col_type] = (type.empty() ? "<"s + C_("types", "all") + ">" : type);
				row[col_type_real] = type;
			}
		}


		/// Update selected row parameters entry
		void update_selected_row_params(const std::string& params)
		{
			if (this->get_selection()->count_selected_rows() > 0) {
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
			return this->get_selection()->count_selected_rows() > 0;
		}


		/// Set the device map (as loaded from config)
		void set_device_map(const AppDeviceOptionMap& devmap)
		{
			clear_all();
			for (const auto& value : devmap.value) {
				std::string dev = value.first.first;
				std::string type = value.first.second;
				std::string params = value.second;
				this->add_new_row(dev, type, params, false);
			}
		}


		/// Get the device map (to be saved to config)
		AppDeviceOptionMap get_device_map()
		{
			AppDeviceOptionMap devmap;

			Gtk::TreeNodeChildren children = model->children();
			for (const auto& row : children) {
				std::string dev = row.get_value(col_device_real);
				if (!dev.empty()) {
					std::string type = row.get_value(col_type_real);
					std::string params = row.get_value(col_parameters);
					devmap.value[std::pair(dev, type)] = params;
				}
			}

			return devmap;
		}



		/// Selection change callback
		void on_selection_changed()
		{
			std::string dev, type, par;
			if (this->get_selection()->count_selected_rows() > 0) {
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

		GscPreferencesWindow* preferences_window = nullptr;  ///< The parent window

};







GscPreferencesWindow::GscPreferencesWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
		: AppBuilderWidget<GscPreferencesWindow, true>(gtkcobj, std::move(ui))
{
	// Connect callbacks

	Gtk::Button* window_cancel_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(window_cancel_button, clicked);

	Gtk::Button* window_ok_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(window_ok_button, clicked);

	Gtk::Button* window_reset_all_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(window_reset_all_button, clicked);


	Glib::ustring smartctl_binary_tooltip = _("A path to smartctl binary. If the path is not absolute, the binary will be looked for in user's PATH.");
	if constexpr(BuildEnv::is_kernel_family_windows()) {
		smartctl_binary_tooltip += Glib::ustring("\n") + _("Note: smartctl.exe shows a console during execution, while smartctl-nc.exe (default) doesn't (nc means no-console).");
	}
	if (auto* smartctl_binary_label = lookup_widget<Gtk::Label*>("smartctl_binary_label")) {
		app_gtkmm_set_widget_tooltip(*smartctl_binary_label, smartctl_binary_tooltip);
	}
	if (auto* smartctl_binary_entry = lookup_widget<Gtk::Entry*>("smartctl_binary_entry")) {
		app_gtkmm_set_widget_tooltip(*smartctl_binary_entry, smartctl_binary_tooltip);
	}

	Gtk::Button* smartctl_binary_browse_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(smartctl_binary_browse_button, clicked);


	Gtk::Button* device_options_add_device_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(device_options_add_device_button, clicked);

	Gtk::Button* device_options_remove_device_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(device_options_remove_device_button, clicked);


	Gtk::Entry* device_options_device_entry = nullptr;
	APP_BUILDER_AUTO_CONNECT(device_options_device_entry, changed);

	Glib::ustring device_options_tooltip = _("A device name to match");
	if constexpr(BuildEnv::is_kernel_family_windows()) {
		device_options_tooltip = _("A device name to match (for example, use \"pd0\" for the first physical drive)");
	} else if constexpr(BuildEnv::is_kernel_linux()) {
		device_options_tooltip = _("A device name to match (for example, /dev/sda or /dev/twa0)");
	}
	if (auto* device_options_device_label = lookup_widget<Gtk::Label*>("device_options_device_label")) {
		app_gtkmm_set_widget_tooltip(*device_options_device_label, device_options_tooltip);
	}
	if (device_options_device_entry) {
		app_gtkmm_set_widget_tooltip(*device_options_device_entry, device_options_tooltip);
	}


	Gtk::Entry* device_options_type_entry = nullptr;
	APP_BUILDER_AUTO_CONNECT(device_options_type_entry, changed);

	Gtk::Entry* device_options_parameter_entry = nullptr;
	APP_BUILDER_AUTO_CONNECT(device_options_parameter_entry, changed);


	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_cancel_button) {
		window_cancel_button->add_accelerator("clicked", accel_group, GDK_KEY_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


	// create Device Options treeview
	get_ui()->get_widget_derived("device_options_treeview", device_options_treeview_);
	if (device_options_treeview_)
		device_options_treeview_->set_preferences_window(this);

	// we can't do this in treeview's constructor, it doesn't know about this window yet.
	this->device_widget_set_remove_possible(false);  // initial state

	// hide win32-only options for non-win32.
	if constexpr(!BuildEnv::is_kernel_family_windows()) {
		if (auto* smartctl_search_check = this->lookup_widget<Gtk::CheckButton*>("search_in_smartmontools_first_check")) {
			smartctl_search_check->hide();
		}
	}

	import_config();

	// show();
}



void GscPreferencesWindow::set_main_window(GscMainWindow* window)
{
	main_window_ = window;
}




void GscPreferencesWindow::update_device_widgets(const std::string& device, const std::string& type, const std::string& params)
{
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_device_entry"))
		entry->set_text(device);

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_type_entry"))
		entry->set_text(type);

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_parameter_entry"))
		entry->set_text(params);
}



void GscPreferencesWindow::device_widget_set_remove_possible(bool b)
{
	if (auto* button = this->lookup_widget<Gtk::Button*>("device_options_remove_device_button"))
		button->set_sensitive(b);
}



namespace {

	/// Set configuration in a smart way - don't set the defaults.
	template<typename T>
	void prefs_config_set(const std::string& path, T&& data)
	{
		using data_type = std::decay_t<T>;
		auto def_value = rconfig::get_default_data<data_type>(path);
		if (def_value != data) {
			rconfig::set_data(path, data);
		} else {
			rconfig::unset_data(path);
		}
	}

}



void GscPreferencesWindow::import_config()
{
	// Clear and fill the entries.

	// ------- General tab

	bool scan_on_startup = rconfig::get_data<bool>("gui/scan_on_startup");
	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("scan_on_startup_check"))
		check->set_active(scan_on_startup);

	bool show_smart_capable_only = rconfig::get_data<bool>("gui/show_smart_capable_only");
	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("show_smart_capable_only_check"))
		check->set_active(show_smart_capable_only);

	bool icons_show_device_name = rconfig::get_data<bool>("gui/icons_show_device_name");
	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("show_device_name_under_icon_check"))
		check->set_active(icons_show_device_name);

	bool icons_show_serial_number = rconfig::get_data<bool>("gui/icons_show_serial_number");
	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("show_serial_number_under_icon_check"))
		check->set_active(icons_show_serial_number);

	bool win32_search_smartctl_in_smartmontools = rconfig::get_data<bool>("system/win32_search_smartctl_in_smartmontools");
	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("search_in_smartmontools_first_check"))
		check->set_active(win32_search_smartctl_in_smartmontools);

	auto smartctl_binary = rconfig::get_data<std::string>("system/smartctl_binary");
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("smartctl_binary_entry"))
		entry->set_text(smartctl_binary);

	auto smartctl_options = rconfig::get_data<std::string>("system/smartctl_options");
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("smartctl_options_entry"))
		entry->set_text(smartctl_options);


	// ------- Drives tab

	auto device_blacklist_patterns = rconfig::get_data<std::string>("system/device_blacklist_patterns");
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_blacklist_patterns_entry"))
		entry->set_text(device_blacklist_patterns);

	if (device_options_treeview_) {
		device_options_treeview_->set_device_map(app_config_get_device_option_map());
	}

}



void GscPreferencesWindow::export_config()
{
	// we don't clear config here, it might contain non-dialog options too.

	// ------- General tab

	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("scan_on_startup_check"))
		prefs_config_set("gui/scan_on_startup", bool(check->get_active()));

	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("show_smart_capable_only_check"))
		prefs_config_set("gui/show_smart_capable_only", bool(check->get_active()));

	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("show_device_name_under_icon_check"))
		prefs_config_set("gui/icons_show_device_name", bool(check->get_active()));

	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("show_serial_number_under_icon_check"))
		prefs_config_set("gui/icons_show_serial_number", bool(check->get_active()));

	if (auto* check = this->lookup_widget<Gtk::CheckButton*>("search_in_smartmontools_first_check"))
		prefs_config_set("system/win32_search_smartctl_in_smartmontools", bool(check->get_active()));

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("smartctl_binary_entry"))
		prefs_config_set("system/smartctl_binary", std::string(entry->get_text()));

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("smartctl_options_entry"))
		prefs_config_set("system/smartctl_options", std::string(entry->get_text()));


	// ------- Drives tab

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_blacklist_patterns_entry"))
		prefs_config_set("system/device_blacklist_patterns", std::string(entry->get_text()));

	auto devmap = device_options_treeview_->get_device_map();
	prefs_config_set("system/smartctl_device_options", devmap);
}



bool GscPreferencesWindow::on_delete_event([[maybe_unused]] GdkEventAny* e)
{
	on_window_cancel_button_clicked();
	return true;  // event handled
}



void GscPreferencesWindow::on_window_cancel_button_clicked()
{
	destroy_instance();
}



void GscPreferencesWindow::on_window_ok_button_clicked()
{
	// Check if device map contains drives with empty device names or parameters.
	auto devmap = device_options_treeview_->get_device_map();
	bool contains_empty = false;
	for (const auto& iter : devmap.value) {
		if (iter.first.first.empty() || iter.second.empty()) {
			contains_empty = true;
		}
	}

	if (contains_empty) {
		Gtk::MessageDialog dialog(*this,
				_("You have specified an empty Parameters field for one or more entries"
				" in Per-Drive Smartctl Parameters section. Such entries will be discarded.\n"
				"\nDo you want to continue?"),
				true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		if (dialog.run() != Gtk::RESPONSE_YES) {
			return;
		}
	}

	export_config();

	if (main_window_) {
		main_window_->show_prefs_updated_message();
	}

	destroy_instance();
}



void GscPreferencesWindow::on_window_reset_all_button_clicked()
{
	Gtk::MessageDialog dialog(*this,
			"\n"s + _("Are you sure you want to reset all program settings to their defaults?") + "\n",
			true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

	if (dialog.run() == Gtk::RESPONSE_YES) {
		rconfig::clear_config();
		import_config();
		// close the window, because the user might get the impression that "Cancel" will revert.
		destroy_instance();
	}
}



void GscPreferencesWindow::on_smartctl_binary_browse_button_clicked()
{
	auto* entry = this->lookup_widget<Gtk::Entry*>("smartctl_binary_entry");
	auto path = hz::fs_path_from_string(std::string(entry->get_text()));

	int result = 0;

	Glib::RefPtr<Gtk::FileFilter> specific_filter, all_filter;
	if constexpr(BuildEnv::is_kernel_family_windows()) {
		specific_filter = Gtk::FileFilter::create();
		specific_filter->set_name(_("Executable Files"));
		specific_filter->add_pattern("*.exe");

		all_filter = Gtk::FileFilter::create();
		all_filter->set_name(_("All Files"));
		all_filter->add_pattern("*");
	}

#if GTK_CHECK_VERSION(3, 20, 0)
	std::unique_ptr<GtkFileChooserNative, decltype(&g_object_unref)> dialog(gtk_file_chooser_native_new(
			_("Choose Smartctl Binary..."), this->gobj(), GTK_FILE_CHOOSER_ACTION_OPEN, nullptr, nullptr),
			&g_object_unref);

	if (path.is_absolute())
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog.get()), hz::fs_path_to_string(path).c_str());

	if constexpr(BuildEnv::is_kernel_family_windows()) {
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());
	}

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
	Gtk::FileChooserDialog dialog(*this, _("Choose Smartctl Binary..."),
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

	if constexpr(BuildEnv::is_kernel_family_windows()) {
		dialog.add_filter(specific_filter);
		dialog.add_filter(all_filter);
	}

	// Note: This works on absolute paths only (otherwise it's gtk warning).
	if (path.is_absolute())
		dialog.set_filename(hz::fs_path_to_string(path));  // change to its dir and select it if exists.

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
	device_options_treeview_->remove_selected_row();
}



void GscPreferencesWindow::on_device_options_add_device_button_clicked()
{
	std::string dev, type, par;

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_device_entry"))
		dev = entry->get_text();

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_type_entry"))
		type = entry->get_text();

	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_parameter_entry"))
		par = entry->get_text();

	if (device_options_treeview_->has_selected_row()) {
		device_options_treeview_->add_new_row("", "", "");  // without this it would clone the existing.
	} else {
		device_options_treeview_->add_new_row(dev, type, par);
	}
}



void GscPreferencesWindow::on_device_options_device_entry_changed()
{
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_device_entry"))
		device_options_treeview_->update_selected_row_device(entry->get_text());
}



void GscPreferencesWindow::on_device_options_type_entry_changed()
{
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_type_entry"))
		device_options_treeview_->update_selected_row_type(entry->get_text());
}



void GscPreferencesWindow::on_device_options_parameter_entry_changed()
{
	if (auto* entry = this->lookup_widget<Gtk::Entry*>("device_options_parameter_entry"))
		device_options_treeview_->update_selected_row_params(entry->get_text());
}








/// @}
