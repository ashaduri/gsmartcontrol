/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#include <glibmm.h>
#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape
#include <vector>  // better use vector, it's needed by others too
#include <algorithm>  // std::min, std::max
#include <memory>
#include <string>

#include "hz/string_num.h"  // number_to_string
#include "hz/string_sprintf.h"  // string_sprintf
#include "hz/string_algo.h"  // string_join
#include "hz/fs.h"
#include "hz/format_unit.h"  // format_time_length
#include "rconfig/rconfig.h"  // rconfig::*

#include "applib/app_gtkmm_tools.h"  // app_gtkmm_*
#include "applib/warning_colors.h"
#include "applib/gui_utils.h"  // gui_show_error_dialog
#include "applib/smartctl_executor_gui.h"
#include "applib/storage_property.h"
#include "applib/storage_device_detected_type.h"

#include "gsc_text_window.h"
#include "gsc_info_window.h"
#include "gsc_executor_error_dialog.h"
#include "gsc_startup_settings.h"



using namespace std::literals;



/// A label for AtaStorageProperty
struct PropertyLabel {
	/// Constructor
	PropertyLabel(std::string label_, const StorageProperty* prop, bool markup_ = false) :
		label(std::move(label_)), property(prop), markup(markup_)
	{ }

	std::string label;  ///< Label text
	const StorageProperty* property = nullptr;  ///< Storage property
	bool markup = false;  ///< Whether the label text uses markup
};




namespace {


	/// Set "top" labels - the generic text at the top of each tab page.
	inline void app_set_top_labels(Gtk::Box* vbox, const std::vector<PropertyLabel>& label_strings)
	{
		if (!vbox)
			return;

		// remove all first
		for (auto& w : vbox->get_children()) {
			vbox->remove(*w);
			delete w;  // since it's without parent anymore, it won't be auto-deleted.
		}

		vbox->set_visible(!label_strings.empty());

		if (label_strings.empty()) {
			// add one label only
// 			Gtk::Label* label = Gtk::manage(new Gtk::Label("No data available", Gtk::ALIGN_START));
// 			label->set_padding(6, 0);
// 			vbox->pack_start(*label, false, false);

		} else {

			// add one label per element
			for (const auto& label_string : label_strings) {
				const std::string label_text = (label_string.markup ? Glib::ustring(label_string.label) : Glib::Markup::escape_text(
						label_string.label));
				Gtk::Label* label = Gtk::manage(new Gtk::Label());
				label->set_markup(label_text);
				label->set_padding(6, 0);
				label->set_alignment(Gtk::ALIGN_START);
				// label->set_ellipsize(Pango::ELLIPSIZE_END);
				label->set_selectable(true);
				label->set_can_focus(false);

				std::string fg;
				if (app_property_get_label_highlight_color(label_string.property->warning_level, fg)) {
					label->set_markup(
							std::string("<span color=\"").append(fg).append("\">")
							.append(label_text).append("</span>") );
				}
				vbox->pack_start(*label, false, false);

				// set it after packing, else the old tooltips api won't have anything to attach them to.
				app_gtkmm_set_widget_tooltip(*label, // label_text + "\n\n" +  // add label text too, in case it's ellipsized
						label_string.property->get_description(), true);  // already markupped

				label->show();
			}
		}

		vbox->show_all_children(true);
	}



	/// Highlight a tab label according to \c warning
	inline void app_highlight_tab_label(Gtk::Widget* label_widget,
			WarningLevel warning, const Glib::ustring& original_label)
	{
		auto* label = dynamic_cast<Gtk::Label*>(label_widget);
		if (!label)
			return;

		if (warning == WarningLevel::None) {
			label->set_markup_with_mnemonic(original_label);
			return;
		}

		std::string fg;
		if (app_property_get_label_highlight_color(warning, fg))
			label->set_markup_with_mnemonic("<span color=\"" + fg + "\">" + original_label + "</span>");
	}



	/// Scroll to appropriate error in text when row is selected in tree.
	inline void on_error_log_treeview_row_selected(GscInfoWindow* window,
			Gtk::TreeModelColumn<Glib::ustring> mark_name_column)
	{
		auto* treeview = window->lookup_widget<Gtk::TreeView*>("error_log_treeview");
		auto* textview = window->lookup_widget<Gtk::TextView*>("error_log_textview");
		Glib::RefPtr<Gtk::TextBuffer> buffer;
		if (treeview != nullptr && textview != nullptr && (buffer = textview->get_buffer())) {
			auto iter = treeview->get_selection()->get_selected();
			if (iter) {
				auto mark = buffer->get_mark((*iter)[mark_name_column]);
				if (mark)
					textview->scroll_to(mark, 0., 0., 0.);
			}
		}
	}



}




GscInfoWindow::GscInfoWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
		: AppBuilderWidget<GscInfoWindow, true>(gtkcobj, std::move(ui))
{
	// Size
	{
		const int def_size_w = rconfig::get_data<int>("gui/info_window/default_size_w");
		const int def_size_h = rconfig::get_data<int>("gui/info_window/default_size_h");
		if (def_size_w > 0 && def_size_h > 0) {
			set_default_size(def_size_w, def_size_h);
		}
	}

	// Create missing widgets
	auto* device_name_hbox = lookup_widget<Gtk::Box*>("device_name_label_hbox");
	if (device_name_hbox) {
		device_name_label_ = Gtk::manage(new Gtk::Label(_("No data available"), Gtk::ALIGN_START));
		device_name_label_->set_selectable(true);
		device_name_label_->show();
		device_name_hbox->pack_start(*device_name_label_, true, true);
	}


	// Connect callbacks

	Gtk::Button* refresh_info_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(refresh_info_button, clicked);

	Gtk::Button* view_output_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(view_output_button, clicked);

	Gtk::Button* save_info_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(save_info_button, clicked);

	Gtk::Button* close_window_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(close_window_button, clicked);

	Gtk::ComboBox* test_type_combo = nullptr;
	APP_BUILDER_AUTO_CONNECT(test_type_combo, changed);

	Gtk::Button* test_execute_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(test_execute_button, clicked);

	Gtk::Button* test_stop_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(test_stop_button, clicked);


	// Accelerators
	if (close_window_button) {
		close_window_button->add_accelerator("clicked", this->get_accel_group(), GDK_KEY_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}

	// Context menu in treeviews
	{
		static const std::vector<std::string> treeview_names {
			"attributes_treeview",
			"nvme_attributes_treeview",
			"statistics_treeview",
			"selftest_log_treeview"
		};

		for (const auto& treeview_name : treeview_names) {
			auto* treeview = lookup_widget<Gtk::TreeView*>(treeview_name);
			treeview_menus_[treeview_name] = new Gtk::Menu();  // deleted in window destructor

			treeview->signal_button_press_event().connect(
					sigc::bind(sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::on_treeview_button_press_event), treeview), treeview_menus_[treeview_name]), false);  // before

			Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(_("Copy Selected Data"), true));
			item->signal_activate().connect(
					sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::on_treeview_menu_copy_clicked), treeview) );
			treeview_menus_[treeview_name]->append(*item);

			treeview_menus_[treeview_name]->show_all();  // Show all menu items when the menu pops up
		}
	}


	// ---------------

	// Create columns of treeviews
	columns_ = std::make_unique<GscInfoWindowColumns>();


	// Set default texts on TextView-s, because glade's "text" property doesn't work
	// on them in gtkbuilder.
	if (auto* textview = lookup_widget<Gtk::TextView*>("error_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}
	if (auto* textview = lookup_widget<Gtk::TextView*>("nvme_error_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}
	if (auto* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}
	if (auto* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}
	if (auto* textview = lookup_widget<Gtk::TextView*>("erc_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}
	if (auto* textview = lookup_widget<Gtk::TextView*>("phy_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}
	if (auto* textview = lookup_widget<Gtk::TextView*>("directory_log_textview")) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
		buffer->set_text("\n"s + _("No data available"));
	}


	// Save their original texts so that we can apply markup to them.
	Gtk::Label* tab_label = nullptr;

	tab_label = lookup_widget<Gtk::Label*>("general_tab_label");
	tab_names_.identity = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("attributes_tab_label");
	tab_names_.ata_attributes = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("nvme_attributes_tab_label");
	tab_names_.nvme_attributes = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("statistics_tab_label");
	tab_names_.statistics = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("test_tab_label");
	tab_names_.test = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("error_log_tab_label");
	tab_names_.ata_error_log = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("nvme_error_log_tab_label");
	tab_names_.nvme_error_log = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("temperature_log_tab_label");
	tab_names_.temperature = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("advanced_tab_label");
	tab_names_.advanced = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("capabilities_tab_label");
	tab_names_.capabilities = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("erc_tab_label");
	tab_names_.erc = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("selective_selftest_tab_label");
	tab_names_.selective_selftest = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("phy_tab_label");
	tab_names_.phy = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("directory_tab_label");
	tab_names_.directory = (tab_label ? tab_label->get_label() : "");

	// show();  // don't show here, removing tabs later is ugly.
}



GscInfoWindow::~GscInfoWindow()
{
	// Store window size. We don't store position to avoid overlaps.
	{
		int window_w = 0, window_h = 0;
		get_size(window_w, window_h);
		rconfig::set_data("gui/info_window/default_size_w", window_w);
		rconfig::set_data("gui/info_window/default_size_h", window_h);
	}

	for (auto& iter : treeview_menus_) {
		delete iter.second;
	}
}



void GscInfoWindow::set_drive(StorageDevicePtr d)
{
	if (drive_)  // if an old drive is present, disconnect our callback from it.
		drive_changed_connection_.disconnect();
	drive_ = std::move(d);
	drive_changed_connection_ = drive_->signal_changed().connect(sigc::mem_fun(this,
			&GscInfoWindow::on_drive_changed));
}



void GscInfoWindow::fill_ui_with_info(bool scan, bool clear_ui, bool clear_tests)
{
	debug_out_info("app", DBG_FUNC_MSG << "Scan " << (scan ? "" : "not ") << "requested.\n");

	if (clear_ui) {
		clear_ui_info(clear_tests);
	}

	if (!drive_->get_is_virtual()) {
		// fetch all smartctl info, even if it already has it (to refresh it).
		if (scan) {
			std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
			ex->create_running_dialog(this, Glib::ustring::compose(_("Running {command} on %1..."), drive_->get_device_with_type()));
			auto fetch_status = drive_->fetch_full_data_and_parse(ex);  // run it with GUI support

			if (!fetch_status) {
				gsc_executor_error_dialog_show(_("Cannot retrieve SMART data"), fetch_status.error().message(), this);
				return;
			}
		}
	}

	// disable refresh button if virtual
	if (drive_->get_is_virtual()) {
		auto* b = lookup_widget<Gtk::Button*>("refresh_info_button");
		if (b) {
			b->set_sensitive(false);
			app_gtkmm_set_widget_tooltip(*b, _("Cannot re-read information from virtual drive"));
		}
	}

	// Hide tabs which have no properties associated with them
	{
		const auto& prop_repo = drive_->get_property_repository();
		Gtk::Widget* note_page_box = nullptr;

		const bool has_ata_attributes = prop_repo.has_properties_for_section(StoragePropertySection::AtaAttributes);
		if (note_page_box = lookup_widget("attributes_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_ata_attributes);
		}

		const bool has_nvme_attributes = prop_repo.has_properties_for_section(StoragePropertySection::NvmeAttributes);
		if (note_page_box = lookup_widget("nvme_attributes_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_nvme_attributes);
		}

		const bool has_statistics = prop_repo.has_properties_for_section(StoragePropertySection::Statistics);
		if (note_page_box = lookup_widget("statistics_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_statistics);
		}

		const bool has_selftest = (drive_->get_self_test_support_status() == StorageDevice::SelfTestSupportStatus::Supported);
		if (note_page_box = lookup_widget("test_tab_vbox"); note_page_box != nullptr) {
			// Some USB flash drives erroneously report SMART as enabled.
			// note_page_box->set_visible(drive->get_smart_status() == StorageDevice::Status::Enabled);
			note_page_box->set_visible(has_selftest);
			if (has_selftest) {
				book_selftest_page_no_ = 4;
			} else {
				book_selftest_page_no_ = -1;
			}
		}

		const bool has_ata_error_log = prop_repo.has_properties_for_section(StoragePropertySection::AtaErrorLog);
		if (note_page_box = lookup_widget("error_log_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_ata_error_log);
		}

		const bool has_nvme_error_log = prop_repo.has_properties_for_section(StoragePropertySection::NvmeErrorLog);
		if (note_page_box = lookup_widget("nvme_error_log_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_nvme_error_log);
		}

		const bool has_temperature_log = prop_repo.has_properties_for_section(StoragePropertySection::TemperatureLog);
		if (note_page_box = lookup_widget("temperature_log_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_temperature_log);
		}

		// Advanced tab's subtabs
		const bool has_capabilities = prop_repo.has_properties_for_section(StoragePropertySection::Capabilities);
		if (note_page_box = lookup_widget("capabilities_scrolledwindow"); note_page_box != nullptr) {
			note_page_box->set_visible(has_capabilities);
		}

		const bool has_erc = prop_repo.has_properties_for_section(StoragePropertySection::ErcLog);
		if (note_page_box = lookup_widget("erc_scrolledwindow"); note_page_box != nullptr) {
			note_page_box->set_visible(has_erc);
		}

		const bool has_selective = prop_repo.has_properties_for_section(StoragePropertySection::SelectiveSelftestLog);
		if (note_page_box = lookup_widget("selective_selftest_scrolledwindow"); note_page_box != nullptr) {
			note_page_box->set_visible(has_selective);
		}

		const bool has_phy = prop_repo.has_properties_for_section(StoragePropertySection::PhyLog);
		if (note_page_box = lookup_widget("phy_scrolledwindow"); note_page_box != nullptr) {
			note_page_box->set_visible(has_phy);
		}

		const bool has_dir = prop_repo.has_properties_for_section(StoragePropertySection::DirectoryLog);
		if (note_page_box = lookup_widget("directory_scrolledwindow"); note_page_box != nullptr) {
			note_page_box->set_visible(has_dir);
		}

		const bool has_advanced =
				has_capabilities
				|| has_erc
				|| has_selective
				|| has_phy
				|| has_dir;
		if (note_page_box = lookup_widget("advanced_tab_vbox"); note_page_box != nullptr) {
			note_page_box->set_visible(has_advanced);
		}

		// Hide tab titles if only one tab is visible
		if (auto* notebook = lookup_widget<Gtk::Notebook*>("main_notebook")) {
			notebook->set_show_tabs(
					has_ata_attributes
					|| has_nvme_attributes
					|| has_statistics
					|| has_selftest
					|| has_ata_error_log
					|| has_nvme_error_log
					|| has_temperature_log
					|| has_advanced);
		}
	}

	// Top label - short device information
	{
		const std::string device = Glib::Markup::escape_text(drive_->get_device_with_type());
		const std::string model = Glib::Markup::escape_text(drive_->get_model_name().empty() ? _("Unknown model") : drive_->get_model_name());
		const std::string drive_letters = Glib::Markup::escape_text(drive_->format_drive_letters(false));

		/// Translators: %1 is device name, %2 is device model.
		this->set_title(Glib::ustring::compose(_("Device Information - %1: %2 - GSmartControl"), device, model));

		// Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label");
		if (device_name_label_) {
			/// Translators: %1 is device name, %2 is drive letters (if not empty), %3 is device model.
			device_name_label_->set_markup(Glib::ustring::compose(_("<b>Device:</b> %1%2  <b>Model:</b> %3"),
					device, (drive_letters.empty() ? "" : (" (<b>" + drive_letters + "</b>)")), model));
		}
	}


	// Fill the tabs with info

	// we need reference here - we take addresses of the elements
	const auto& property_repo = drive_->get_property_repository();  // it's a vector

	fill_ui_general(property_repo);
	fill_ui_ata_attributes(property_repo);
	fill_ui_nvme_attributes(property_repo);
	fill_ui_statistics(property_repo);
	if (clear_tests) {
		fill_ui_self_test_info();
	}
	fill_ui_self_test_log(property_repo);
	fill_ui_ata_error_log(property_repo);
	fill_ui_nvme_error_log(property_repo);
	fill_ui_temperature_log(property_repo);

	// Advanced tab
	auto caps_warning_level = fill_ui_capabilities(property_repo);
	auto errc_warning_level = fill_ui_error_recovery(property_repo);
	auto selective_warning_level = fill_ui_selective_self_test_log(property_repo);
	auto dir_warning_level = fill_ui_directory(property_repo);
	auto phy_warning_level = fill_ui_physical(property_repo);

	auto max_advanced_tab_warning = std::max({
		caps_warning_level,
		errc_warning_level,
		selective_warning_level,
		dir_warning_level,
		phy_warning_level
	});

	// Advanced tab label
	app_highlight_tab_label(lookup_widget("advanced_tab_label"), max_advanced_tab_warning, tab_names_.advanced);
}



void GscInfoWindow::clear_ui_info(bool clear_tests_too)
{
	// Note: We do NOT show/hide the notebook tabs here.
	// fill_ui_with_info() will do it all by itself.

	{
		this->set_title(_("Device Information - GSmartControl"));

		// Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label");
		if (device_name_label_) {
			device_name_label_->set_text(_("No data available"));
		}
	}

	{
		auto* identity_table = lookup_widget<Gtk::Grid*>("identity_table");
		if (identity_table) {
			// manually remove all children. without this visual corruption occurs.
			auto children = identity_table->get_children();
			for (auto& widget : children) {
				identity_table->remove(*widget);
			}
		}

		// tab label
		app_highlight_tab_label(lookup_widget("general_tab_label"), WarningLevel::None, tab_names_.identity);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("attributes_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		if (auto* treeview = lookup_widget<Gtk::TreeView*>("attributes_treeview")) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("attributes_tab_label"), WarningLevel::None, tab_names_.ata_attributes);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("nvme_attributes_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		if (auto* treeview = lookup_widget<Gtk::TreeView*>("nvme_attributes_treeview")) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("nvme_attributes_tab_label"), WarningLevel::None, tab_names_.nvme_attributes);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("statistics_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		if (auto* treeview = lookup_widget<Gtk::TreeView*>("statistics_treeview")) {
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("statistics_tab_label"), WarningLevel::None, tab_names_.statistics);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("selftest_log_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		if (auto* treeview = lookup_widget<Gtk::TreeView*>("selftest_log_treeview")) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		if (clear_tests_too) {
			auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");
			if (test_type_combo) {
				test_type_combo->set_sensitive(false);  // true if testing is possible and not active.
				// test_type_combo->clear();  // clear cellrenderers
				if (test_combo_model_)
					test_combo_model_->clear();
			}

			if (auto* min_duration_label = lookup_widget<Gtk::Label*>("min_duration_label"))
				min_duration_label->set_text("N/A");  // set on test selection

			if (auto* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button"))
				test_execute_button->set_sensitive(false);  // true if testing is possible and not active


			auto* test_description_textview = lookup_widget<Gtk::TextView*>("test_description_textview");
			if (test_description_textview != nullptr && test_description_textview->get_buffer())
				test_description_textview->get_buffer()->set_text("");  // set on test selection

			if (auto* test_completion_progressbar = lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar")) {
				test_completion_progressbar->set_text("");  // set when test is run or completed
				test_completion_progressbar->set_sensitive(false);  // set when test is run or completed
				test_completion_progressbar->hide();
			}

			if (auto* test_stop_button = lookup_widget<Gtk::Button*>("test_stop_button")) {
				test_stop_button->set_sensitive(false);  // true when test is active
				test_stop_button->hide();
			}

			if (auto* test_result_hbox = lookup_widget<Gtk::Box*>("test_result_hbox"))
				test_result_hbox->hide();  // hide by default. show when test is completed.
		}

		// tab label
		app_highlight_tab_label(lookup_widget("test_tab_label"), WarningLevel::None, tab_names_.test);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("error_log_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		auto* treeview = lookup_widget<Gtk::TreeView*>("error_log_treeview");
		if (treeview) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		auto* textview = lookup_widget<Gtk::TextView*>("error_log_textview");
		if (textview) {
			// we re-create the buffer to get rid of all the Marks
			textview->set_buffer(Gtk::TextBuffer::create());
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("error_log_tab_label"), WarningLevel::None, tab_names_.ata_error_log);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("nvme_error_log_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		auto* textview = lookup_widget<Gtk::TextView*>("nvme_error_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("nvme_error_log_tab_label"), WarningLevel::None, tab_names_.nvme_error_log);
	}

	{
		auto* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("temperature_log_tab_label"), WarningLevel::None, tab_names_.temperature);
	}

	// tab label
	app_highlight_tab_label(lookup_widget("advanced_tab_label"), WarningLevel::None, tab_names_.advanced);

	{
		if (auto* treeview = lookup_widget<Gtk::TreeView*>("capabilities_treeview")) {
			// It's better to clear the model rather than unset it. If we unset it, we'll have
			// to deattach the callbacks too. But if we clear it, we have to remember column vars.
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("capabilities_tab_label"), WarningLevel::None, tab_names_.capabilities);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("erc_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("erc_tab_label"), WarningLevel::None, tab_names_.erc);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("selective_selftest_tab_label"), WarningLevel::None, tab_names_.selective_selftest);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("phy_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("phy_tab_label"), WarningLevel::None, tab_names_.phy);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("directory_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("directory_tab_label"), WarningLevel::None, tab_names_.directory);
	}

	// Delete columns. We need to do this because otherwise,
	// the column objects store their old indexes and will break when re-added
	columns_.reset();
	columns_ = std::make_unique<GscInfoWindowColumns>();
}



void GscInfoWindow::refresh_info(bool clear_tests_too)
{
	this->set_sensitive(false);  // make insensitive until filled. helps with pressed F5 problem.

	// this->clear_ui_info();  // no need, fill_ui_with_info() will call it.
	this->fill_ui_with_info(true, true, clear_tests_too);

	this->set_sensitive(true);  // make sensitive again.
}



void GscInfoWindow::show_tests()
{
	if (auto* book = lookup_widget<Gtk::Notebook*>("main_notebook")) {
		if (book_selftest_page_no_ >= 0) {
			book->set_current_page(book_selftest_page_no_);  // the Tests tab
		} else {
			gui_show_warn_dialog(_("Self-Tests Not Supported"), _("Self-tests are not supported on this drive."), this);
		}
	}
}



bool GscInfoWindow::on_delete_event([[maybe_unused]] GdkEventAny* e)
{
	on_close_window_button_clicked();
	return true;  // event handled
}



void GscInfoWindow::on_refresh_info_button_clicked()
{
	this->refresh_info();
}



void GscInfoWindow::on_view_output_button_clicked()
{
	auto win = GscTextWindow<SmartctlOutputInstance>::create();
	// make save visible and enable monospace font

	std::string output = this->drive_->get_full_output();
	if (output.empty()) {
		output = this->drive_->get_basic_output();
	}

	win->set_text_from_command(_("Smartctl Output"), output);

	// Set text content for saving as .txt
	if (auto p = this->drive_->get_property_repository().lookup_property("smartctl/output"); !p.empty()) {
		const std::string text_output = p.get_value<std::string>();
		if (!text_output.empty()) {
			win->set_text_contents(text_output);
		}
	}

	const std::string filename = drive_->get_save_filename();
	if (!filename.empty())
		win->set_save_filename(filename);

	win->show();
}



void GscInfoWindow::on_save_info_button_clicked()
{
	static std::string last_dir;
	if (last_dir.empty()) {
		last_dir = rconfig::get_data<std::string>("gui/drive_data_open_save_dir");
	}
	int result = 0;

	const std::string filename = drive_->get_save_filename();

	Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
	specific_filter->set_name(_("JSON and Text Files"));
	specific_filter->add_pattern("*.json");
	specific_filter->add_pattern("*.txt");

	Glib::RefPtr<Gtk::FileFilter> json_filter = Gtk::FileFilter::create();
	json_filter->set_name(_("JSON Files"));
	json_filter->add_pattern("*.json");

	Glib::RefPtr<Gtk::FileFilter> txt_filter = Gtk::FileFilter::create();
	txt_filter->set_name(_("Text Files"));
	txt_filter->add_pattern("*.txt");

	Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
	all_filter->set_name(_("All Files"));
	all_filter->add_pattern("*");

#if GTK_CHECK_VERSION(3, 20, 0)
	std::unique_ptr<GtkFileChooserNative, decltype(&g_object_unref)> dialog(gtk_file_chooser_native_new(
			_("Save Data As..."), this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr),
			&g_object_unref);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), TRUE);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), json_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), txt_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

	if (!last_dir.empty())
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());

	if (!filename.empty())
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog.get()), filename.c_str());

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
	Gtk::FileChooserDialog dialog(*this, _("Save Data As..."),
			Gtk::FILE_CHOOSER_ACTION_SAVE);

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);

	dialog.set_do_overwrite_confirmation(true);

	dialog.add_filter(specific_filter);
	dialog.add_filter(json_filter);
	dialog.add_filter(txt_filter);
	dialog.add_filter(all_filter);

	if (!last_dir.empty())
		dialog.set_current_folder(last_dir);

	if (!filename.empty())
		dialog.set_current_name(filename);

	// Show the dialog and wait for a user response
	result = dialog.run();  // the main cycle blocks here
#endif

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			hz::fs::path file;
#if GTK_CHECK_VERSION(3, 20, 0)
			file = hz::fs_path_from_string(app_string_from_gchar(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog.get()))));
			last_dir = hz::fs_path_to_string(file.parent_path());
#else
			file = hz::fs_path_from_string(dialog.get_filename());  // in fs encoding
			last_dir = dialog.get_current_folder();  // save for the future
#endif
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

			bool txt_selected = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog.get())) == txt_filter->gobj();

			if (file.extension() != ".json" && file.extension() != ".txt") {
				file += (txt_selected ? ".txt" : ".json");
			}

			bool save_txt = txt_selected || file.extension() == ".txt";

			std::string data = this->drive_->get_full_output();
			if (data.empty()) {
				data = this->drive_->get_basic_output();
			}
			if (save_txt) {
				if (auto p = this->drive_->get_property_repository().lookup_property("smartctl/output"); !p.empty()) {
					const std::string text_output = p.get_value<std::string>();
					if (!text_output.empty()) {
						data = text_output;
					}
				}
			}

			const std::error_code ec = hz::fs_file_put_contents(file, data);
			if (ec) {
				gui_show_error_dialog(_("Cannot save SMART data to file"), ec.message(), this);
			}
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



void GscInfoWindow::on_close_window_button_clicked()
{
	if (drive_ && drive_->get_test_is_active()) {  // disallow close if test is active.
		gui_show_warn_dialog(_("Please wait until all tests are finished."), this);
	} else {
		destroy_instance();  // deletes this object and nullifies instance
	}
}



void GscInfoWindow::on_test_type_combo_changed()
{
	auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");

	Gtk::TreeRow row = *(test_type_combo->get_active());
	if (row) {
		std::shared_ptr<SelfTest> test = row[test_combo_columns_.self_test];

		//debug_out_error("app", test->get_min_duration_seconds() << "\n");
		if (auto* min_duration_label = lookup_widget<Gtk::Label*>("min_duration_label")) {
			auto duration = test->get_min_duration_seconds();
			min_duration_label->set_text(duration == std::chrono::seconds(-1) ? C_("duration", "N/A")
					: (duration.count() == 0 ? C_("duration", "Unknown") : hz::format_time_length(duration)));
		}

		auto* test_description_textview = lookup_widget<Gtk::TextView*>("test_description_textview");
		if (test_description_textview != nullptr && test_description_textview->get_buffer())
			test_description_textview->get_buffer()->set_text(row[test_combo_columns_.description]);
	}
}



void GscInfoWindow::fill_ui_general(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	// filter out some properties
	std::vector<StorageProperty> general_props, version_props, overall_health_props, nvme_health_props;

	for (auto&& p : props) {
		if (p.section == StoragePropertySection::Info) {
			if (p.generic_name == "smartctl/version/_merged_full") {
				version_props.push_back(p);
			} else if (p.generic_name == "smartctl/version/_merged") {
				continue;  // we use the full version string instead.
			} else {
				general_props.push_back(p);
			}
		} else if (p.section == StoragePropertySection::OverallHealth) {
			overall_health_props.push_back(p);
		} else if (p.section == StoragePropertySection::NvmeHealth) {
			nvme_health_props.push_back(p);
		}
	}

	// put version after all the info
	for (auto&& p : version_props) {
		general_props.push_back(p);
	}

	// health at the bottom
	for (auto&& p : overall_health_props) {
		general_props.push_back(p);
	}

	// nvme health properties are only present if there is health failure
	for (auto&& p : nvme_health_props) {
		general_props.push_back(p);
	}



	auto* identity_table = lookup_widget<Gtk::Grid*>("identity_table");

	identity_table->hide();

	WarningLevel max_tab_warning = WarningLevel::None;
	int row = 0;

	for (auto&& p : general_props) {
		if (!p.show_in_ui) {
			continue;  // hide debug messages from smartctl
		}

		if (p.generic_name == "smart_status/passed") {  // a little distance for this one
			Gtk::Label* empty_label = Gtk::manage(new Gtk::Label());
			empty_label->set_can_focus(false);
			identity_table->attach(*empty_label, 0, row, 2, 1);
			++row;
		}

		Gtk::Label* name = Gtk::manage(new Gtk::Label());
		// name->set_ellipsize(Pango::ELLIPSIZE_END);
		name->set_alignment(Gtk::ALIGN_END);  // right-align
		name->set_selectable(true);
		name->set_can_focus(false);
		name->set_markup("<b>" + Glib::Markup::escape_text(p.displayable_name) + "</b>");

		// If the above is Label, then this has to be Label too, else it will shrink
		// and "name" will take most of the horizontal space. If "name" is set to shrink,
		// then it stops being right-aligned.
		Gtk::Label* value = Gtk::manage(new Gtk::Label());
		// value->set_ellipsize(Pango::ELLIPSIZE_END);
		value->set_alignment(Gtk::ALIGN_START);  // left-align
		value->set_selectable(true);
		value->set_can_focus(false);
		value->set_markup(Glib::Markup::escape_text(p.format_value()));

		std::string fg;
		if (app_property_get_label_highlight_color(p.warning_level, fg)) {
			name->set_markup("<span color=\"" + fg + "\">" + name->get_label() + "</span>");
			value->set_markup("<span color=\"" + fg + "\">" + value->get_label() + "</span>");
		}

		identity_table->attach(*name, 0, row, 1, 1);
		identity_table->attach(*value, 1, row, 1, 1);

		app_gtkmm_set_widget_tooltip(*name, p.get_description(), true);
		app_gtkmm_set_widget_tooltip(*value, // value->get_label() + "\n\n" +
				p.get_description(), true);

		if (int(p.warning_level) > int(max_tab_warning)) {
			max_tab_warning = p.warning_level;
		}

		++row;
	}

	identity_table->show_all();

	// tab label
	app_highlight_tab_label(lookup_widget("general_tab_label"), max_tab_warning, tab_names_.identity);
}



void GscInfoWindow::fill_ui_ata_attributes(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* treeview = lookup_widget<Gtk::TreeView*>("attributes_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// ID (int), Name, Flag (hex), Normalized Value (uint8), Worst (uint8), Thresh (uint8), Raw (int64), Type (string),
	// Updated (string), When Failed (string)

	model_columns.add(columns_->ata_attribute_table_columns.id);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.id, *treeview, _("ID"), _("Attribute ID"), true);

	model_columns.add(columns_->ata_attribute_table_columns.displayable_name);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.displayable_name, *treeview,
			_("Name"), _("Attribute name (this is deduced from ID by smartctl and may be incorrect, as it's highly vendor-specific)"), true);
	treeview->set_search_column(columns_->ata_attribute_table_columns.displayable_name.index());

	model_columns.add(columns_->ata_attribute_table_columns.when_failed);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.when_failed, *treeview,
			_("Failed"), _("When failed (that is, the normalized value became equal to or less than threshold)"), true, true);

	model_columns.add(columns_->ata_attribute_table_columns.normalized_value);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.normalized_value, *treeview,
			C_("value", "Normalized"), _("Normalized value (highly vendor-specific; converted from Raw value by the drive's firmware)"), false);

	model_columns.add(columns_->ata_attribute_table_columns.worst);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.worst, *treeview,
			C_("value", "Worst"), _("The worst normalized value recorded for this attribute during the drive's lifetime (with SMART enabled)"), false);

	model_columns.add(columns_->ata_attribute_table_columns.threshold);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.threshold, *treeview,
			C_("value", "Threshold"), _("Threshold for normalized value. Normalized value should be greater than threshold (unless vendor thinks otherwise)."), false);

	model_columns.add(columns_->ata_attribute_table_columns.raw);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.raw, *treeview,
			_("Raw value"), _("Raw value as reported by drive. May or may not be sensible."), false);

	model_columns.add(columns_->ata_attribute_table_columns.type);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.type, *treeview,
			_("Type"), _("Alarm condition is reached when normalized value becomes less than or equal to threshold. Type indicates whether it's a signal of drive's pre-failure time or just an old age."), false, true);

	// Doesn't carry that much info. Advanced users can look at the flags.
// 		model_columns.add(attribute_table_columns.updated);
// 		tree_col = app_gtkmm_create_tree_view_column(attribute_table_columns.updated, *treeview,
// 				"Updated", "The attribute is usually updated continuously, or during Offline Data Collection only. This column indicates that.", true);

	model_columns.add(columns_->ata_attribute_table_columns.flag_value);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->ata_attribute_table_columns.flag_value, *treeview,
			_("Flags"), _("Flags") + "\n\n"s
					+ Glib::ustring::compose(_("If given in %1 format, the presence of each letter indicates that the flag is on."), "POSRCK+") + "\n"
					+ _("P: pre-failure attribute (if the attribute failed, the drive is failing)") + "\n"
					+ _("O: updated continuously (as opposed to updated on offline data collection)") + "\n"
					+ _("S: speed / performance attribute") + "\n"
					+ _("R: error rate") + "\n"
					+ _("C: event count") + "\n"
					+ _("K: auto-keep") + "\n"
					+ _("+: undocumented bits present"), false);

	model_columns.add(columns_->ata_attribute_table_columns.tooltip);
	treeview->set_tooltip_column(columns_->ata_attribute_table_columns.tooltip.index());

	model_columns.add(columns_->ata_attribute_table_columns.storage_property);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(columns_->ata_attribute_table_columns.id, Gtk::SORT_ASCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::cell_renderer_for_ata_attributes), i));
	}


	WarningLevel max_tab_warning = WarningLevel::None;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (const auto& p : props) {
		if (p.section != StoragePropertySection::AtaAttributes || !p.show_in_ui)
			continue;

		// add non-attribute-type properties to label above
		if (!p.is_value_type<AtaStorageAttribute>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);

			if (int(p.warning_level) > int(max_tab_warning))
				max_tab_warning = p.warning_level;
			continue;
		}

		const auto& attr = p.get_value<AtaStorageAttribute>();

		Gtk::TreeRow row = *(list_store->append());
		row[columns_->ata_attribute_table_columns.id] = attr.id;
		row[columns_->ata_attribute_table_columns.displayable_name] = Glib::Markup::escape_text(p.displayable_name);
		row[columns_->ata_attribute_table_columns.flag_value] = Glib::Markup::escape_text(attr.flag);  // it's a string, not int.
		row[columns_->ata_attribute_table_columns.normalized_value] = Glib::Markup::escape_text(attr.value.has_value() ? hz::number_to_string_locale(attr.value.value()) : "-");
		row[columns_->ata_attribute_table_columns.worst] = Glib::Markup::escape_text(attr.worst.has_value() ? hz::number_to_string_locale(attr.worst.value()) : "-");
		row[columns_->ata_attribute_table_columns.threshold] = Glib::Markup::escape_text(attr.threshold.has_value() ? hz::number_to_string_locale(attr.threshold.value()) : "-");
		row[columns_->ata_attribute_table_columns.raw] = Glib::Markup::escape_text(attr.format_raw_value());
		row[columns_->ata_attribute_table_columns.type] = Glib::Markup::escape_text(
				AtaStorageAttribute::get_readable_attribute_type_name(attr.attr_type));
// 		row[attribute_table_columns.updated] = Glib::Markup::escape_text(AtaStorageAttribute::get_update_type_name(attr.update_type));
		row[columns_->ata_attribute_table_columns.when_failed] = Glib::Markup::escape_text(
				AtaStorageAttribute::get_readable_fail_time_name(attr.when_failed));
		row[columns_->ata_attribute_table_columns.tooltip] = p.get_description();  // markup
		row[columns_->ata_attribute_table_columns.storage_property] = &p;

		if (int(p.warning_level) > int(max_tab_warning))
			max_tab_warning = p.warning_level;
	}


	auto* label_vbox = lookup_widget<Gtk::Box*>("attributes_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("attributes_tab_label"), max_tab_warning, tab_names_.ata_attributes);
}



void GscInfoWindow::fill_ui_nvme_attributes(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* treeview = lookup_widget<Gtk::TreeView*>("nvme_attributes_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	model_columns.add(columns_->nvme_attribute_table_columns.displayable_name);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->nvme_attribute_table_columns.displayable_name, *treeview,
			_("Description"), _("Entry description"), true);
	treeview->set_search_column(columns_->nvme_attribute_table_columns.displayable_name.index());

	model_columns.add(columns_->nvme_attribute_table_columns.value);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->nvme_attribute_table_columns.value, *treeview,
			_("Value"), _("Value"), false);

	model_columns.add(columns_->nvme_attribute_table_columns.tooltip);
	treeview->set_tooltip_column(columns_->nvme_attribute_table_columns.tooltip.index());

	model_columns.add(columns_->nvme_attribute_table_columns.storage_property);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::cell_renderer_for_nvme_attributes), i));
	}

	WarningLevel max_tab_warning = WarningLevel::None;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (const auto& p : props) {
		if (p.section != StoragePropertySection::NvmeAttributes || !p.show_in_ui)
			continue;

		Gtk::TreeRow row = *(list_store->append());

		const auto& value = p.format_value();
		row[columns_->nvme_attribute_table_columns.displayable_name] = Glib::Markup::escape_text(p.displayable_name);
		row[columns_->nvme_attribute_table_columns.value] = Glib::Markup::escape_text(value);
		row[columns_->nvme_attribute_table_columns.tooltip] = p.get_description();  // markup
		row[columns_->nvme_attribute_table_columns.storage_property] = &p;

		if (int(p.warning_level) > int(max_tab_warning))
			max_tab_warning = p.warning_level;
	}

	auto* label_vbox = lookup_widget<Gtk::Box*>("nvme_attributes_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("nvme_attributes_tab_label"), max_tab_warning, tab_names_.nvme_attributes);
}



void GscInfoWindow::fill_ui_statistics(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* treeview = lookup_widget<Gtk::TreeView*>("statistics_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	model_columns.add(columns_->statistics_table_columns.displayable_name);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->statistics_table_columns.displayable_name, *treeview,
			_("Description"), _("Entry description"), true);
	treeview->set_search_column(columns_->statistics_table_columns.displayable_name.index());

	model_columns.add(columns_->statistics_table_columns.value);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->statistics_table_columns.value, *treeview,
			_("Value"), Glib::ustring::compose(_("Value (can be normalized if '%1' flag is present)"), "N"), false);

	model_columns.add(columns_->statistics_table_columns.flags);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->statistics_table_columns.flags, *treeview,
			_("Flags"), _("Flags") + "\n\n"s
					+ _("V: valid") + "\n"
					+ _("N: value is normalized") + "\n"
					+ _("D: supports Device Statistics Notification (DSN)") + "\n"
					+ _("C: monitored condition met") + "\n"  // Related to DSN? From the specification, it looks like something user-controllable.
					+ _("+: undocumented bits present"), false);

	model_columns.add(columns_->statistics_table_columns.page_offset);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->statistics_table_columns.page_offset, *treeview,
			_("Page, Offset"), _("Page and offset of the entry"), false);

	model_columns.add(columns_->statistics_table_columns.tooltip);
	treeview->set_tooltip_column(columns_->statistics_table_columns.tooltip.index());

	model_columns.add(columns_->statistics_table_columns.storage_property);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	treeview->set_model(list_store);
	// No sorting (we don't want to screw up the headers).

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::cell_renderer_for_statistics), i));
	}

	WarningLevel max_tab_warning = WarningLevel::None;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (const auto& p : props) {
		if (p.section != StoragePropertySection::Statistics || !p.show_in_ui)
			continue;

		// add non-entry-type properties to label above
		if (!p.is_value_type<AtaStorageStatistic>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);

			if (int(p.warning_level) > int(max_tab_warning))
				max_tab_warning = p.warning_level;
			continue;
		}

		Gtk::TreeRow row = *(list_store->append());

		const auto& st = p.get_value<AtaStorageStatistic>();
		row[columns_->statistics_table_columns.displayable_name] = Glib::Markup::escape_text(st.is_header ? p.displayable_name : ("    " + p.displayable_name));
		row[columns_->statistics_table_columns.value] = Glib::Markup::escape_text(st.format_value());
		row[columns_->statistics_table_columns.flags] = Glib::Markup::escape_text(st.flags);  // it's a string, not int.
		row[columns_->statistics_table_columns.page_offset] = Glib::Markup::escape_text(st.is_header ? std::string()
				: hz::string_sprintf("0x%02x, 0x%03x", int(st.page), int(st.offset)));
		row[columns_->statistics_table_columns.tooltip] = p.get_description();  // markup
		row[columns_->statistics_table_columns.storage_property] = &p;

		if (int(p.warning_level) > int(max_tab_warning))
			max_tab_warning = p.warning_level;
	}

	auto* label_vbox = lookup_widget<Gtk::Box*>("statistics_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("statistics_tab_label"), max_tab_warning, tab_names_.statistics);
}



void GscInfoWindow::fill_ui_self_test_info()
{
	auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");

	// don't check with get_model(), it comes pre-modeled from glade.
	if (!test_combo_model_) {
		Gtk::TreeModelColumnRecord model_columns;

		// Test name, [description], [selftest_obj]
		model_columns.add(test_combo_columns_.name);  // we can use the column variable by value after this.
		model_columns.add(test_combo_columns_.description);
		model_columns.add(test_combo_columns_.self_test);

		test_combo_model_ = Gtk::ListStore::create(model_columns);
		test_type_combo->set_model(test_combo_model_);

		// visible columns
		test_type_combo->clear();  // clear old (glade) cellrenderers

		test_type_combo->pack_start(test_combo_columns_.name);
	}

	// add possible tests

	Gtk::TreeModel::Row row;

//	auto test_ioffline = std::make_shared<SelfTest>(drive, SelfTest::TestType::ImmediateOffline);
//	if (test_ioffline->is_supported()) {
//		row = *(test_combo_model->append());
//		row[test_combo_columns.name] = SelfTest::get_test_displayable_name(SelfTest::TestType::ImmediateOffline);
//		row[test_combo_columns.description] =
//				_("Immediate Offline Test (also known as Immediate Offline Data Collection)"
//				" is the manual version of Automatic Offline Data Collection, which, if enabled, is automatically run"
//				" every four hours. If an error occurs during this test, it will be reported in Error Log. Besides that,"
//				" its effects are visible only in that it updates the \"Offline\" Attribute values.");
//		row[test_combo_columns.self_test] = test_ioffline;
//	}

	auto test_short = std::make_shared<SelfTest>(drive_, SelfTest::TestType::ShortTest);
	if (test_short->is_supported()) {
		row = *(test_combo_model_->append());
		row[test_combo_columns_.name] = SelfTest::get_test_displayable_name(SelfTest::TestType::ShortTest);
		row[test_combo_columns_.description] =
				_("Short self-test consists of a collection of test routines that have the highest chance"
				" of detecting drive problems. Its result is reported in the Self-Test Log."
				" Note that this test is in no way comprehensive. Its main purpose is to detect totally damaged"
				" drives without running a full surface scan."
				"\nNote: On some drives this actually runs several consequent tests, which may"
				" cause the program to display the test progress incorrectly.");  // seagate multi-pass test on 7200.11.
		row[test_combo_columns_.self_test] = test_short;
	}

	auto test_long = std::make_shared<SelfTest>(drive_, SelfTest::TestType::LongTest);
	if (test_long->is_supported()) {
		row = *(test_combo_model_->append());
		row[test_combo_columns_.name] = SelfTest::get_test_displayable_name(SelfTest::TestType::LongTest);
		row[test_combo_columns_.description] =
				_("Extended self-test examines complete disk surface and performs various test routines"
				" built into the drive. Its result is reported in the Self-Test Log.");
		row[test_combo_columns_.self_test] = test_long;
	}

	auto test_conveyance = std::make_shared<SelfTest>(drive_, SelfTest::TestType::Conveyance);
	if (test_conveyance->is_supported()) {
		row = *(test_combo_model_->append());
		row[test_combo_columns_.name] = SelfTest::get_test_displayable_name(SelfTest::TestType::Conveyance);
		row[test_combo_columns_.description] =
				_("Conveyance self-test is intended to identify damage incurred during transporting of the drive.");
		row[test_combo_columns_.self_test] = test_conveyance;
	}

	if (!test_combo_model_->children().empty()) {
		test_type_combo->set_sensitive(true);
		test_type_combo->set_active(test_combo_model_->children().begin());  // select first entry

		// At least one test is possible, so enable test button.
		// Note: we disable only Execute button on virtual drives. The combo is left
		// sensitive so that the user can see which tests are supported by the drive.
		auto* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button");
		if (test_execute_button)
			test_execute_button->set_sensitive(!drive_->get_is_virtual());
	}
}



void GscInfoWindow::fill_ui_self_test_log(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* treeview = lookup_widget<Gtk::TreeView*>("selftest_log_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// Test num., Type, Status, % Completed, Lifetime hours, LBA of the first error

	model_columns.add(columns_->self_test_log_table_columns.log_entry_index);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->self_test_log_table_columns.log_entry_index, *treeview,
			_("Test #"), _("Test # (greater may mean newer or older depending on drive model)"), true);

	model_columns.add(columns_->self_test_log_table_columns.type);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->self_test_log_table_columns.type, *treeview,
			_("Type"), _("Type of the test performed"), true);
	treeview->set_search_column(columns_->self_test_log_table_columns.type.index());

	model_columns.add(columns_->self_test_log_table_columns.status);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->self_test_log_table_columns.status, *treeview,
			_("Status"), _("Test completion status"), true);

	model_columns.add(columns_->self_test_log_table_columns.percent);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->self_test_log_table_columns.percent, *treeview,
			_("% Completed"), _("Percentage of the test completed. Instantly-aborted tests have 10%, while unsupported ones <i>may</i> have 100%."), true, false, true);

	model_columns.add(columns_->self_test_log_table_columns.hours);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->self_test_log_table_columns.hours, *treeview,
			_("Lifetime hours"), _("Hour of the drive's powered-on lifetime when the test completed or aborted.\nThe value wraps after 65535 hours."), true);

	model_columns.add(columns_->self_test_log_table_columns.lba);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->self_test_log_table_columns.lba, *treeview,
			_("LBA of the first error"), _("LBA of the first error (if an LBA-related error happened)"), true);

	model_columns.add(columns_->self_test_log_table_columns.tooltip);
	treeview->set_tooltip_column(columns_->self_test_log_table_columns.tooltip.index());

	model_columns.add(columns_->self_test_log_table_columns.storage_property);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(columns_->self_test_log_table_columns.log_entry_index, Gtk::SORT_ASCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::cell_renderer_for_self_test_log), i));
	}


	WarningLevel max_tab_warning = WarningLevel::None;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	bool ata_entries_found = false;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::SelftestLog || !p.show_in_ui)
			continue;

		if (p.generic_name == "ata_smart_self_test_log/_merged")  // the whole section, we don't need it
			continue;

		if (p.is_value_type<AtaStorageSelftestEntry>()) {
			ata_entries_found = true;
		}

		// add non-entry properties to label above
		if (!p.is_value_type<AtaStorageSelftestEntry>() && !p.is_value_type<NvmeStorageSelftestEntry>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);

			if (int(p.warning_level) > int(max_tab_warning))
				max_tab_warning = p.warning_level;
			continue;
		}

		Gtk::TreeRow row = *(list_store->append());

		if (p.is_value_type<AtaStorageSelftestEntry>()) {
			const auto& entry = p.get_value<AtaStorageSelftestEntry>();
			row[columns_->self_test_log_table_columns.log_entry_index] = entry.test_num;
			row[columns_->self_test_log_table_columns.type] = Glib::Markup::escape_text(entry.type);
			row[columns_->self_test_log_table_columns.status] = Glib::Markup::escape_text(entry.get_readable_status());
			if (entry.remaining_percent != -1) {  // only extended log supports this
				row[columns_->self_test_log_table_columns.percent] = Glib::Markup::escape_text(hz::number_to_string_locale(100 - entry.remaining_percent) + "%");
			}
			row[columns_->self_test_log_table_columns.hours] = Glib::Markup::escape_text(hz::number_to_string_locale(entry.lifetime_hours));
			row[columns_->self_test_log_table_columns.lba] = Glib::Markup::escape_text(entry.lba_of_first_error);

		} else if (p.is_value_type<NvmeStorageSelftestEntry>()) {
			const auto& entry = p.get_value<NvmeStorageSelftestEntry>();
			row[columns_->self_test_log_table_columns.log_entry_index] = entry.test_num;
			row[columns_->self_test_log_table_columns.type] = Glib::Markup::escape_text(NvmeSelfTestTypeExt::get_displayable_name(entry.type));
			row[columns_->self_test_log_table_columns.status] = Glib::Markup::escape_text(NvmeSelfTestResultTypeExt::get_displayable_name(entry.result));
			row[columns_->self_test_log_table_columns.hours] = Glib::Markup::escape_text(hz::number_to_string_locale(entry.power_on_hours));
			row[columns_->self_test_log_table_columns.lba] = Glib::Markup::escape_text(entry.lba.has_value() ? hz::number_to_string_locale(entry.lba.value()) : std::string("-"));
		}
		// There are no descriptions in self-test log entries, so don't display
		// "No description available" for all of them.
		// row[columns_->self_test_log_table_columns.tooltip] = p.get_description();
		row[columns_->self_test_log_table_columns.storage_property] = &p;

		if (int(p.warning_level) > int(max_tab_warning))
			max_tab_warning = p.warning_level;
	}

	// Hide percentage column if NVMe as there is no such field in output.
	treeview->get_column(3)->set_visible(ata_entries_found);  // % Completed


	auto* label_vbox = lookup_widget<Gtk::Box*>("selftest_log_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("test_tab_label"), max_tab_warning, tab_names_.test);
}



void GscInfoWindow::fill_ui_ata_error_log(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* treeview = lookup_widget<Gtk::TreeView*>("error_log_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// Error Number, Lifetime Hours, State, Type, Details, [tooltips]

	model_columns.add(columns_->error_log_table_columns.log_entry_index);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->error_log_table_columns.log_entry_index, *treeview,
			_("Error #"), _("Error # in the error log (greater means newer)"), true);

	model_columns.add(columns_->error_log_table_columns.hours);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->error_log_table_columns.hours, *treeview,
			_("Lifetime hours"), _("Hour of the drive's powered-on lifetime when the error occurred"), true);

	model_columns.add(columns_->error_log_table_columns.state);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->error_log_table_columns.state, *treeview,
			C_("power", "State"), _("Power state of the drive when the error occurred"), false);

	model_columns.add(columns_->error_log_table_columns.lba);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->error_log_table_columns.lba, *treeview,
			_("LBA"), _("LBA Address"), true);

	model_columns.add(columns_->error_log_table_columns.details);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->error_log_table_columns.details, *treeview,
			_("Details"), _("Additional details"), true);

	model_columns.add(columns_->error_log_table_columns.tooltip);
	treeview->set_tooltip_column(columns_->error_log_table_columns.tooltip.index());

	model_columns.add(columns_->error_log_table_columns.storage_property);

	model_columns.add(columns_->error_log_table_columns.mark_name);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(columns_->error_log_table_columns.log_entry_index, Gtk::SORT_DESCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::cell_renderer_for_error_log), i));
	}


	WarningLevel max_tab_warning = WarningLevel::None;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	bool supports_details = false;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::AtaErrorLog || !p.show_in_ui)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "ata_smart_error_log/_merged") {
			supports_details = true;  // Text parser only
			if (auto* textview = lookup_widget<Gtk::TextView*>("error_log_textview")) {
				// Add complete error log to textview window.
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				if (buffer) {
					buffer->set_text("\n" + Glib::ustring::compose(_("Complete error log: %1"), "\n\n" + p.get_value<std::string>()));

					// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
					Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
					tag->property_family() = "Monospace";
					buffer->apply_tag(tag, buffer->begin(), buffer->end());

					// Set marks so we can scroll to them
					if (!error_log_row_selected_conn_.connected()) {  // avoid double-connect
						error_log_row_selected_conn_ = treeview->get_selection()->signal_changed().connect(
								sigc::bind(sigc::bind(sigc::ptr_fun(on_error_log_treeview_row_selected), columns_->error_log_table_columns.mark_name), this));
					}

					Gtk::TextIter titer = buffer->begin();
					Gtk::TextIter match_start, match_end;
					// TODO Change this for json
					while (titer.forward_search("\nError ", Gtk::TEXT_SEARCH_TEXT_ONLY, match_start, match_end)) {
						match_start.forward_char();  // place after newline
						match_end.forward_word_end();  // include error number
						titer = match_end;  // continue searching from here

						const Glib::ustring mark_name = match_start.get_slice(match_end);  // e.g. "Error 3"
						buffer->create_mark(mark_name, titer);
					}
				}
			}


			// add non-tree properties to label above
		} else if (!p.is_value_type<AtaStorageErrorBlock>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);
			if (p.generic_name == "ata_smart_error_log/extended/count")
				label_strings.back().label += " "s + _("(Note: The number of entries may be limited to the newest ones)");

		} else {
			const auto& eb = p.get_value<AtaStorageErrorBlock>();

			Gtk::TreeRow row = *(list_store->append());
			row[columns_->error_log_table_columns.log_entry_index] = eb.error_num;
			row[columns_->error_log_table_columns.hours] = Glib::Markup::escape_text(hz::number_to_string_locale(eb.lifetime_hours));
			row[columns_->error_log_table_columns.state] = Glib::Markup::escape_text(eb.device_state);

			std::string details_str = eb.type_more_info;  // parsed in JSON
			if (details_str.empty()) {
				details_str = AtaStorageErrorBlock::format_readable_error_types(eb.reported_types);  // parsed in Text
			}

			row[columns_->error_log_table_columns.lba] = Glib::Markup::escape_text(hz::number_to_string_locale(eb.lba));
			row[columns_->error_log_table_columns.details] = Glib::Markup::escape_text(details_str.empty() ? "-" : details_str);
			row[columns_->error_log_table_columns.tooltip] = p.get_description();  // markup
			row[columns_->error_log_table_columns.storage_property] = &p;
			row[columns_->error_log_table_columns.mark_name] = Glib::ustring::compose(_("Error %1"), eb.error_num);
		}

		if (int(p.warning_level) > int(max_tab_warning))
			max_tab_warning = p.warning_level;
	}

	// JSON parser does not support details, so hide the bottom area.
	auto* details_area = lookup_widget<Gtk::ScrolledWindow*>("error_log_details_scrolledwindow");
	details_area->set_visible(supports_details);

	auto* label_vbox = lookup_widget<Gtk::Box*>("error_log_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// inner tab label
	app_highlight_tab_label(lookup_widget("error_log_tab_label"), max_tab_warning, tab_names_.ata_error_log);
}



void GscInfoWindow::fill_ui_nvme_error_log(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* textview = lookup_widget<Gtk::TextView*>("nvme_error_log_textview");

	WarningLevel max_tab_warning = WarningLevel::None;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::NvmeErrorLog || !p.show_in_ui)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "nvme_error_information_log/_merged") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("NVMe Non-Persistent Error Information Log: %1"), "\n\n" + p.get_value<std::string>()));

			// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_family() = "Monospace";
			buffer->apply_tag(tag, buffer->begin(), buffer->end());
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("nvme_error_log_tab_label"), max_tab_warning, tab_names_.nvme_error_log);
}



void GscInfoWindow::fill_ui_temperature_log(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");

	WarningLevel max_tab_warning = WarningLevel::None;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	std::string temperature;
	StorageProperty temp_property;
	enum { temp_attr2 = 1, temp_attr1, temp_stat, temp_sct, temp_info };  // less important to more important
	int temp_prop_source = 0;

	for (const auto& p : props) {
		// Find temperature
		if (temp_prop_source < temp_info && p.generic_name == "temperature/current") {  // Protocol-independent temperature
			temperature = hz::number_to_string_locale(p.get_value<int64_t>());
			temp_property = p;
			temp_prop_source = temp_info;
		}
		if (temp_prop_source < temp_sct && p.generic_name == "ata_sct_status/temperature/current") {
			temperature = hz::number_to_string_locale(p.get_value<int64_t>());
			temp_property = p;
			temp_prop_source = temp_sct;
		}
		if (temp_prop_source < temp_stat && p.generic_name == "stat_temperature_celsius") {
			temperature = hz::number_to_string_locale(p.get_value<AtaStorageStatistic>().value_int);
			temp_property = p;
			temp_prop_source = temp_stat;
		}
		if (temp_prop_source < temp_attr1 && p.generic_name == "attr_temperature_celsius") {
			// Note: raw value may encode min/max as well, leading to very large values.
			// Instead, convert the string value (can be "27" or "27 (Min/Max 11/59)").
			std::int64_t temp_int = 0;
			if (hz::string_is_numeric_nolocale(p.get_value<AtaStorageAttribute>().raw_value, temp_int, false)) {
				temperature = hz::number_to_string_locale(temp_int);
				temp_property = p;
				temp_prop_source = temp_attr1;
			}
		}
		if (temp_prop_source < temp_attr2 && p.generic_name == "attr_temperature_celsius_x10") {
			temperature = hz::number_to_string_locale(p.get_value<AtaStorageAttribute>().raw_value_int / 10);
			temp_property = p;
			temp_prop_source = temp_attr2;
		}

		if (p.section != StoragePropertySection::TemperatureLog || !p.show_in_ui)
			continue;

		if (p.generic_name == "_text_only/ata_sct_status/_not_present" && p.get_value<bool>()) {  // only show if unsupported
			label_strings.emplace_back(_("SCT temperature commands not supported."), &p);
			if (int(p.warning_level) > int(max_tab_warning))
				max_tab_warning = p.warning_level;
			continue;
		}

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "ata_sct_status/_and/ata_sct_temperature_history/_merged") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete SCT temperature log: %1"), "\n\n" + p.get_value<std::string>()));

			// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_family() = "Monospace";
			buffer->apply_tag(tag, buffer->begin(), buffer->end());
		}
	}

	if (temperature.empty()) {
		temperature = C_("value", "Unknown");
	} else {
		temperature = Glib::ustring::compose(C_("temperature", "%1° C"), temperature);
	}
	temp_property.set_description(_("Current drive temperature in Celsius."));  // overrides attribute description
	label_strings.emplace_back(Glib::ustring::compose(_("Current temperature: %1"),
			"<b>" + Glib::Markup::escape_text(temperature) + "</b>"), &temp_property, true);
	if (int(temp_property.warning_level) > int(max_tab_warning))
		max_tab_warning = temp_property.warning_level;


	auto* label_vbox = lookup_widget<Gtk::Box*>("temperature_log_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("temperature_log_tab_label"), max_tab_warning, tab_names_.temperature);
}



WarningLevel GscInfoWindow::fill_ui_capabilities(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* treeview = lookup_widget<Gtk::TreeView*>("capabilities_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// N, Name, Flag, Capabilities, [tooltips]

	model_columns.add(columns_->capabilities_table_columns.entry_index);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->capabilities_table_columns.entry_index, *treeview, _("#"), _("Entry #"), true);

	model_columns.add(columns_->capabilities_table_columns.name);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->capabilities_table_columns.name, *treeview, _("Name"), _("Name"), true);
	treeview->set_search_column(columns_->capabilities_table_columns.name.index());

	model_columns.add(columns_->capabilities_table_columns.flag_value);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->capabilities_table_columns.flag_value, *treeview, _("Flags"), _("Flags"), false);
//	treeview->get_column(num_tree_col - 1)->set_visible(false);  //

	model_columns.add(columns_->capabilities_table_columns.str_values);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->capabilities_table_columns.str_values, *treeview, _("Capabilities"), _("Capabilities"), false);

	model_columns.add(columns_->capabilities_table_columns.value);
	num_tree_col = app_gtkmm_create_tree_view_column(columns_->capabilities_table_columns.value, *treeview, _("Value"), _("Value"), false);

	model_columns.add(columns_->capabilities_table_columns.tooltip);
	treeview->set_tooltip_column(columns_->capabilities_table_columns.tooltip.index());

	model_columns.add(columns_->capabilities_table_columns.storage_property);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(columns_->capabilities_table_columns.entry_index, Gtk::SORT_ASCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::cell_renderer_for_capabilities), i));
	}


	WarningLevel max_tab_warning = WarningLevel::None;
	int index = 1;

	bool has_text_parser_capabilities = false;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::Capabilities || !p.show_in_ui)
			continue;

		std::string flag_value;
		Glib::ustring str_value;

		if (p.is_value_type<AtaStorageTextCapability>()) {
			flag_value = hz::number_to_string_nolocale(p.get_value<AtaStorageTextCapability>().flag_value, 16);  // 0xXX
			str_value = hz::string_join(p.get_value<AtaStorageTextCapability>().strvalues, "\n");
			has_text_parser_capabilities = true;
		} else {
			// no flag value here
			str_value = p.format_value();
		}

		Gtk::TreeRow row = *(list_store->append());
		row[columns_->capabilities_table_columns.entry_index] = index;
		row[columns_->capabilities_table_columns.name] = Glib::Markup::escape_text(p.displayable_name);
		row[columns_->capabilities_table_columns.flag_value] = Glib::Markup::escape_text(flag_value.empty() ? "-" : flag_value);
		row[columns_->capabilities_table_columns.str_values] = Glib::Markup::escape_text(str_value);
		row[columns_->capabilities_table_columns.value] = Glib::Markup::escape_text(str_value);
		row[columns_->capabilities_table_columns.tooltip] = p.get_description();  // markup
		row[columns_->capabilities_table_columns.storage_property] = &p;

		if (int(p.warning_level) > int(max_tab_warning))
			max_tab_warning = p.warning_level;

		++index;
	}

	// Show/hide columns according to parser type.
	treeview->get_column(2)->set_visible(has_text_parser_capabilities);  // flag_value
	treeview->get_column(3)->set_visible(has_text_parser_capabilities);  // str_values
	treeview->get_column(4)->set_visible(!has_text_parser_capabilities);  // value

	// tab label
	app_highlight_tab_label(lookup_widget("capabilities_tab_label"), max_tab_warning, tab_names_.capabilities);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_error_recovery(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* textview = lookup_widget<Gtk::TextView*>("erc_log_textview");

	WarningLevel max_tab_warning = WarningLevel::None;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::ErcLog || !p.show_in_ui)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "ata_sct_erc/_merged") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete SCT Error Recovery Control settings: %1"), "\n\n" + p.get_value<std::string>()));

			// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_family() = "Monospace";
			buffer->apply_tag(tag, buffer->begin(), buffer->end());
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("erc_tab_label"), max_tab_warning, tab_names_.erc);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_selective_self_test_log(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview");

	WarningLevel max_tab_warning = WarningLevel::None;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::SelectiveSelftestLog || !p.show_in_ui)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "ata_smart_selective_self_test_log/_merged") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete selective self-test log: %1"), "\n\n" + p.get_value<std::string>()));

			// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_family() = "Monospace";
			buffer->apply_tag(tag, buffer->begin(), buffer->end());
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("selective_selftest_tab_label"), max_tab_warning, tab_names_.selective_selftest);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_physical(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* textview = lookup_widget<Gtk::TextView*>("phy_log_textview");

	WarningLevel max_tab_warning = WarningLevel::None;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::PhyLog || !p.show_in_ui)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "sata_phy_event_counters/_merged") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete phy log: %1"), "\n\n" + p.get_value<std::string>()));

			// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_family() = "Monospace";
			buffer->apply_tag(tag, buffer->begin(), buffer->end());
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("phy_tab_label"), max_tab_warning, tab_names_.phy);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_directory(const StoragePropertyRepository& property_repo)
{
	const auto& props = property_repo.get_properties();

	auto* textview = lookup_widget<Gtk::TextView*>("directory_log_textview");

	WarningLevel max_tab_warning = WarningLevel::None;

	for (auto&& p : props) {
		if (p.section != StoragePropertySection::DirectoryLog || !p.show_in_ui)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "ata_log_directory/_merged") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete directory log: %1"), "\n\n" + p.get_value<std::string>()));

			// Make the text monospace (the 3.16+ glade property does not work anymore for some reason).
			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_family() = "Monospace";
			buffer->apply_tag(tag, buffer->begin(), buffer->end());
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("directory_tab_label"), max_tab_warning, tab_names_.directory);

	return max_tab_warning;
}



/// Set cell renderer's foreground and background colors according to property warning level.
inline void cell_renderer_set_warning_fg_bg(Gtk::CellRendererText* crt, const StorageProperty& p)
{
	std::string fg, bg;
	if (app_property_get_row_highlight_colors(p.warning_level, fg, bg)) {
		// Note: property_cell_background makes horizontal tree lines disappear around it,
		// but property_background doesn't play nice with sorted column color.
		crt->property_cell_background() = bg;
		crt->property_foreground() = fg;
	} else {
		// this is needed because cellrenderer is shared in column, so the previous call
		// may set the color for all subsequent cells.
		crt->property_cell_background().reset_value();
		crt->property_foreground().reset_value();
	}
}



void GscInfoWindow::cell_renderer_for_ata_attributes(Gtk::CellRenderer* cr,
		const Gtk::TreeModel::iterator& iter, [[maybe_unused]] int column_index) const
{
	const StorageProperty* prop = (*iter)[columns_->ata_attribute_table_columns.storage_property];
	if (!prop) {
		return;
	}
	const auto& attribute = prop->get_value<AtaStorageAttribute>();

	if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
		cell_renderer_set_warning_fg_bg(crt, *prop);

		if (column_index == columns_->ata_attribute_table_columns.displayable_name.index()) {
			crt->property_weight() = Pango::WEIGHT_BOLD;
		}
		if (column_index == columns_->ata_attribute_table_columns.type.index()) {
			if (attribute.attr_type == AtaStorageAttribute::AttributeType::Prefail) {
				crt->property_weight() = Pango::WEIGHT_BOLD;
			} else {  // reset to default value if reloading
				crt->property_weight().reset_value();
			}
		}
		if (column_index == columns_->ata_attribute_table_columns.when_failed.index()) {
			if (attribute.when_failed != AtaStorageAttribute::FailTime::None) {
				crt->property_weight() = Pango::WEIGHT_BOLD;
			} else {  // reset to default value if reloading
				// Do not use WEIGHT_NORMAL here, it interferes with cell markup
				crt->property_weight().reset_value();
			}
		}

		// Monospace, align all numeric values
		if (column_index == columns_->ata_attribute_table_columns.normalized_value.index()
				|| column_index == columns_->ata_attribute_table_columns.worst.index()
				|| column_index == columns_->ata_attribute_table_columns.threshold.index()
				|| column_index == columns_->ata_attribute_table_columns.raw.index() ) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = 1.;  // right-align
		}
		if (column_index == columns_->ata_attribute_table_columns.id.index()
				|| column_index == columns_->ata_attribute_table_columns.flag_value.index()) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = .5;  // center-align
		}
		if (column_index == columns_->ata_attribute_table_columns.type.index()) {
			crt->property_xalign() = .5;  // center-align
		}
	}
}



void GscInfoWindow::cell_renderer_for_nvme_attributes(Gtk::CellRenderer* cr,
		const Gtk::TreeModel::iterator& iter, int column_index) const
{
	const StorageProperty* prop = (*iter)[this->columns_->nvme_attribute_table_columns.storage_property];
	if (!prop) {
		return;
	}

	if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
		cell_renderer_set_warning_fg_bg(crt, *prop);

		if (column_index == columns_->nvme_attribute_table_columns.displayable_name.index()) {
			crt->property_weight() = Pango::WEIGHT_BOLD;
		}

		if (column_index == columns_->nvme_attribute_table_columns.value.index()) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = 1.;  // right-align
		}
	}
}



void GscInfoWindow::cell_renderer_for_statistics(Gtk::CellRenderer* cr,
		const Gtk::TreeModel::iterator& iter, [[maybe_unused]] int column_index) const
{
	const StorageProperty* prop = (*iter)[this->columns_->statistics_table_columns.storage_property];
	if (!prop) {
		return;
	}
	const auto& statistic = prop->get_value<AtaStorageStatistic>();

	if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
		cell_renderer_set_warning_fg_bg(crt, *prop);

		if (statistic.is_header) {  // subheader
			crt->property_weight() = Pango::WEIGHT_BOLD;
		} else {  // reset to default value if reloading
			crt->property_weight().reset_value();
		}

		// Monospace, align all numeric values
		if (column_index == columns_->statistics_table_columns.value.index()) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = 1.;  // right-align
		}
		if (column_index == columns_->statistics_table_columns.flags.index()
				|| column_index == columns_->statistics_table_columns.page_offset.index() ) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = .5;  // center-align
		}
	}
}



void GscInfoWindow::cell_renderer_for_self_test_log(Gtk::CellRenderer* cr,
		const Gtk::TreeModel::iterator& iter, [[maybe_unused]] int column_index) const
{
	const StorageProperty* prop = (*iter)[this->columns_->self_test_log_table_columns.storage_property];
	if (!prop) {
		return;
	}

	if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
		cell_renderer_set_warning_fg_bg(crt, *prop);

		if (column_index == columns_->self_test_log_table_columns.log_entry_index.index()) {
			crt->property_weight() = Pango::WEIGHT_BOLD;
		}

		// Monospace, align all numeric values
		if (column_index == columns_->self_test_log_table_columns.log_entry_index.index()
				|| column_index == columns_->self_test_log_table_columns.percent.index()
				|| column_index == columns_->self_test_log_table_columns.hours.index() ) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = 1.;  // right-align
		}
		if (column_index == columns_->self_test_log_table_columns.log_entry_index.index()) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = .5;  // center-align
		}
		if (column_index == columns_->self_test_log_table_columns.lba.index()) {
			crt->property_family() = "Monospace";
		}
	}
}



void GscInfoWindow::cell_renderer_for_error_log(Gtk::CellRenderer* cr,
		const Gtk::TreeModel::iterator& iter, [[maybe_unused]] int column_index) const
{
	const StorageProperty* prop = (*iter)[this->columns_->error_log_table_columns.storage_property];
	if (!prop) {
		return;
	}

	if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
		cell_renderer_set_warning_fg_bg(crt, *prop);

		if (column_index == columns_->error_log_table_columns.log_entry_index.index()) {
			crt->property_weight() = Pango::WEIGHT_BOLD;
		}

		// Monospace, align all numeric values
		if (column_index == columns_->error_log_table_columns.log_entry_index.index()) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = .5;  // center-align
		}
		if (column_index == columns_->error_log_table_columns.hours.index()) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = 1.;  // right-align
		}
	}
}



void GscInfoWindow::cell_renderer_for_capabilities(Gtk::CellRenderer* cr,
		const Gtk::TreeModel::iterator& iter, [[maybe_unused]] int column_index) const
{
	const StorageProperty* prop = (*iter)[this->columns_->capabilities_table_columns.storage_property];
	if (!prop) {
		return;
	}

	if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
		cell_renderer_set_warning_fg_bg(crt, *prop);

		if (column_index == columns_->capabilities_table_columns.name.index()) {
			crt->property_weight() = Pango::WEIGHT_BOLD;
		}

		// Monospace, align all numeric values
		if (column_index == columns_->capabilities_table_columns.entry_index.index()
				|| column_index == columns_->capabilities_table_columns.flag_value.index() ) {
			crt->property_family() = "Monospace";
			crt->property_xalign() = .5;  // center-align
		}
	}
}



// Note: Another loop like this may run inside it for another drive.
gboolean GscInfoWindow::test_idle_callback(void* data)
{
	auto* self = static_cast<GscInfoWindow*>(data);
	DBG_ASSERT_RETURN(self, false);

	if (!self->current_test_)  // shouldn't happen
		return FALSE;  // stop

	auto* test_completion_progressbar =
			self->lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar");


	bool active = true;

	do {  // goto
		if (!self->current_test_->is_active()) {  // check status
			active = false;
			break;
		}

		const std::int8_t rem_percent = self->current_test_->get_remaining_percent();
		const std::string rem_percent_str = (rem_percent == -1 ? C_("value", "Unknown") : hz::number_to_string_locale(100 - rem_percent));

		auto poll_in = self->current_test_->get_poll_in_seconds();  // sec


		// One update() is performed by start(), so do the timeout first.

		// Wait until next poll (up to several minutes). Meanwhile, interpolate
		// the remaining time, update the progressbar, etc.
		if (self->test_timer_poll_.elapsed() < static_cast<double>(poll_in.count())) {  // elapsed() is seconds in double.

			// Update progress bar right after poll, plus every 5 seconds.
			if (self->test_force_bar_update_ || self->test_timer_bar_.elapsed() >= 5.) {

				auto rem_seconds = self->current_test_->get_remaining_seconds();

				if (test_completion_progressbar) {
					const std::string rem_seconds_str = (rem_seconds == std::chrono::seconds(-1) ? C_("duration", "Unknown") : hz::format_time_length(rem_seconds));

					Glib::ustring bar_str;

					if (self->test_error_msg_.empty()) {
						bar_str = Glib::ustring::compose(_("Test completion: %1%%; ETA: %2"), rem_percent_str, rem_seconds_str);
					} else {
						bar_str = self->test_error_msg_;  // better than popup every few seconds
					}

					test_completion_progressbar->set_text(bar_str);
					test_completion_progressbar->set_fraction(std::max(0., std::min(1., 1. - (rem_percent / 100.))));
				}

				self->test_force_bar_update_ = false;
				self->test_timer_bar_.start();  // restart
			}

			if (!self->current_test_->is_active()) {  // check status
				active = false;
				break;
			}


		} else {  // it's poll time

			if (!self->current_test_->is_active()) {  // the inner loop stopped, stop this one too
				active = false;
				break;
			}

			std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
			ex->create_running_dialog(self);

			auto test_status = self->current_test_->update(ex);
			self->test_error_msg_ = (!test_status ? test_status.error().message() : "");
			if (!self->test_error_msg_.empty()) {
// 				gui_show_error_dialog("Cannot monitor test progress", self->test_error_msg, this);  // better show in progressbar.
				[[maybe_unused]] auto result = self->current_test_->force_stop(ex);  // what else can we do?
				active = false;
				break;
			}

			self->test_timer_poll_.start();  // restart it
			self->test_force_bar_update_ = true;  // force progressbar / ETA update on the next tick
		}


	} while (false);


	if (active) {
		return TRUE;  // continue the idle callback
	}


	// Test is finished, clean up

	self->test_timer_poll_.stop();  // just in case
	self->test_timer_bar_.stop();  // just in case

	auto status = self->current_test_->get_status();

	bool aborted = false;
	SelfTestStatusSeverity severity = SelfTestStatusSeverity::None;
	std::string result_details_msg;

	if (!self->test_error_msg_.empty()) {
		aborted = true;
		severity = SelfTestStatusSeverity::Error;
		result_details_msg = Glib::ustring::compose(_("<b>Test aborted: %1</b>"), Glib::Markup::escape_text(self->test_error_msg_));

	} else {
		severity = get_self_test_status_severity(status);
		if (status == SelfTestStatus::ManuallyAborted) {
			aborted = true;
			result_details_msg = "<b>"s + _("Test was manually aborted.") + "</b>";  // it's a StatusSeverity::none message

		} else {
			result_details_msg = Glib::ustring::compose(_("<b>Test result: %1</b>."),
					Glib::Markup::escape_text(SelfTestStatusExt::get_displayable_name(status)));

			// It may not reach 100% somehow, so do it manually.
			if (test_completion_progressbar)
				test_completion_progressbar->set_fraction(1.);  // yes, we're at 100% anyway (at least logically).
		}
	}

	std::string result_main_msg;
	if (aborted) {
		result_main_msg = _("TEST ABORTED!");
	} else {
		switch (status) {
			case SelfTestStatus::Unknown:
				result_main_msg = _("TEST STATUS UNKNOWN.");
				break;
			case SelfTestStatus::InProgress:
				result_main_msg = _("TEST IN PROGRESS.");
				break;
			case SelfTestStatus::ManuallyAborted:
				result_main_msg = _("TEST ABORTED!");
				break;
			case SelfTestStatus::Interrupted:
				result_main_msg = _("TEST INTERRUPTED!");
				break;
			case SelfTestStatus::CompletedNoError:
				result_main_msg = _("TEST SUCCESSFUL.");
				break;
			case SelfTestStatus::CompletedWithError:
				result_main_msg = _("TEST FAILED!");
				break;
			case SelfTestStatus::Reserved:
				result_main_msg = _("TEST STATUS UNKNOWN.");
				break;
		}
	}

	switch (severity) {
		case SelfTestStatusSeverity::None:
			break;
		case SelfTestStatusSeverity::Warning:
			result_details_msg = "\n"s + _("Check the Self-Test Log for more information.");
			break;
		case SelfTestStatusSeverity::Error:
			if (!result_main_msg.empty()) {  // Highlight in red
				result_main_msg = "<span color=\"#FF0000\">"s + result_main_msg + "</span>";
			}
			result_details_msg += "\n"s + _("Check the Self-Test Log for more information.");
			break;
	}

	if (!result_main_msg.empty()) {
		result_main_msg = "<b>"s + result_main_msg + "</b>\n";
	}
	std::string result_msg = result_main_msg + result_details_msg;


	if (auto* test_type_combo = self->lookup_widget<Gtk::ComboBox*>("test_type_combo"))
		test_type_combo->set_sensitive(true);

	if (auto* test_execute_button = self->lookup_widget<Gtk::Button*>("test_execute_button"))
		test_execute_button->set_sensitive(true);

	if (test_completion_progressbar) {
		test_completion_progressbar->set_text("");
	}

	if (auto* test_stop_button = self->lookup_widget<Gtk::Button*>("test_stop_button"))
		test_stop_button->set_sensitive(false);

	std::string icon_name = "dialog-error";
	if (severity == SelfTestStatusSeverity::None) {
		icon_name = "dialog-information";
	} else if (severity == SelfTestStatusSeverity::Warning) {
		icon_name = "dialog-warning";
	}

	// we use large icon size here because the icons we use are from dialogs.
	// unfortunately, there are no non-dialog icons of this sort.
	if (auto* test_result_image = self->lookup_widget<Gtk::Image*>("test_result_image")) {
		test_result_image->set_from_icon_name(icon_name, Gtk::ICON_SIZE_DND);
	}

	if (auto* test_result_label = self->lookup_widget<Gtk::Label*>("test_result_label")) {
		test_result_label->set_markup(result_msg);
	}

	if (auto* test_result_hbox = self->lookup_widget<Gtk::Box*>("test_result_hbox")) {
		test_result_hbox->show();
	}

	self->refresh_info(false);  // don't clear the tests tab

	return FALSE;  // stop idle callback
}





void GscInfoWindow::on_test_execute_button_clicked()
{
	auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");
	DBG_ASSERT_RETURN_NONE(test_type_combo);

	Gtk::TreeRow row = *(test_type_combo->get_active());
	if (!row)
		return;

	std::shared_ptr<SelfTest> test_from_combo = row[test_combo_columns_.self_test];
	auto test = std::make_shared<SelfTest>(drive_, test_from_combo->get_test_type());
	if (!test)
		return;

	// hide previous test results from GUI
	if (auto* test_result_hbox = this->lookup_widget<Gtk::Box*>("test_result_hbox"))
		test_result_hbox->hide();

	std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
	ex->create_running_dialog(this);

	auto test_status = test->start(ex);  // this runs update() too.
	if (!test_status) {
		/// Translators: %1 is test name
		gui_show_error_dialog(Glib::ustring::compose(_("Cannot run %1"),
				SelfTest::get_test_displayable_name(test->get_test_type())), test_status.error().message(), this);
		return;
	}

	current_test_ = test;


	// Switch GUI to "running test" mode
	test_type_combo->set_sensitive(false);

	if (auto* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button"))
		test_execute_button->set_sensitive(false);

	if (auto* test_completion_progressbar = lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar")) {
		test_completion_progressbar->set_text("");
		test_completion_progressbar->set_sensitive(true);
		test_completion_progressbar->show();
	}

	if (auto* test_stop_button = lookup_widget<Gtk::Button*>("test_stop_button")) {
		test_stop_button->set_sensitive(true);  // true while test is active
		test_stop_button->show();
	}


	// reset these
	test_error_msg_.clear();
	test_timer_poll_.start();
	test_timer_bar_.start();
	test_force_bar_update_ = true;


	// We don't use idle function here, because it has the following problem:
	// CommandExecutor::execute() (which is called on force_stop()) calls g_main_context_pending(),
	// which returns true EVERY time, until the idle callback returns false.
	// So, force_stop() exits its "execute abort" command only when the
	// idle callback polls the drive on the next timeout and sees that the test
	// has been actually aborted.
	// So, we use timeout callback - and hope that there are no usleeps
	// with 300ms so that g_main_context_pending() returns false at least once,
	// to escape the execute() loop.

// 	g_idle_add(test_idle_callback, this);
	g_timeout_add(300, test_idle_callback, this);  // every 300ms
}




void GscInfoWindow::on_test_stop_button_clicked()
{
	if (!current_test_)
		return;

	std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
	ex->create_running_dialog(this);

	auto test_status = current_test_->force_stop(ex);
	if (!test_status) {
		/// Translators: %1 is test name
		gui_show_error_dialog(Glib::ustring::compose(_("Cannot stop %1"),
				SelfTest::get_test_displayable_name(current_test_->get_test_type())), test_status.error().message(), this);
		return;
	}

	// nothing else to do - the cleanup is performed by the idle callback.
}




// Callback attached to StorageDevice.
// We don't refresh automatically (that would make it impossible to do
// several same-drive info window comparisons side by side).
// But we need to look for testing status change, to avoid aborting it.
void GscInfoWindow::on_drive_changed([[maybe_unused]] StorageDevice* pdrive)
{
	if (!drive_)
		return;
	const bool test_active = drive_->get_test_is_active();

	// disable refresh button if test is active or if it's a virtual drive
	if (auto* refresh_info_button = lookup_widget<Gtk::Button*>("refresh_info_button"))
		refresh_info_button->set_sensitive(!test_active && !drive_->get_is_virtual());

	// disallow close. usually modal dialogs are used for this, but we can't have
	// per-drive modal dialogs.
	if (auto* close_window_button = lookup_widget<Gtk::Button*>("close_window_button"))
		close_window_button->set_sensitive(!test_active);

	// test_active is also checked in delete_event handler, because this call may not
	// succeed - the window manager may refuse to do it.
	this->set_deletable(!test_active);
}



bool GscInfoWindow::on_treeview_button_press_event(GdkEventButton* button_event, Gtk::Menu* menu, Gtk::TreeView* treeview)
{
	if (button_event->type == GDK_BUTTON_PRESS && button_event->button == 3) {
		const bool selection_empty = treeview->get_selection()->get_selected_rows().empty();
		std::vector<Widget*> children = menu->get_children();
		for (auto& child : children) {
			child->set_sensitive(!selection_empty);
		}
		menu->popup(button_event->button, button_event->time);
		return true;
	}
	return false;
}



void GscInfoWindow::on_treeview_menu_copy_clicked(Gtk::TreeView* treeview)
{
	std::string text;

	const int num_cols = static_cast<int>(treeview->get_n_columns());
	std::vector<std::string> col_texts;
	for (int i = 0; i < num_cols; ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		col_texts.push_back("\"" + hz::string_replace_copy(tcol->get_title(), "\"", "\"\"") + "\"");
	}
	text += hz::string_join(col_texts, ',') + "\n";

	auto selection = treeview->get_selection()->get_selected_rows();
	auto list_store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
	for (const auto& path : selection) {
		std::vector<std::string> cell_texts;
		Gtk::TreeRow row = *(list_store->get_iter(path));

		for (int j = 0; j < num_cols; ++j) {  // gather data only from tree columns, not model columns (like tooltips and helper data)
			const GType type = list_store->get_column_type(j);
			if (type == G_TYPE_INT) {
				int32_t value = 0;
				row.get_value(j, value);
				cell_texts.push_back(hz::number_to_string_nolocale(value));  // no locale for export
			} else if (type == G_TYPE_STRING) {
				std::string value;
				row.get_value(j, value);
				cell_texts.push_back("\"" + hz::string_replace_copy(value, "\"", "\"\"") + "\"");
			}
		}
		text += hz::string_join(cell_texts, ',') + "\n";
	}

	if (auto clipboard = Gtk::Clipboard::get()) {
		clipboard->set_text(text);
	}
}







/// @}
