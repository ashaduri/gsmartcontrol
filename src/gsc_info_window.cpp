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

#include "local_glibmm.h"
#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape
#include <vector>  // better use vector, it's needed by others too
#include <algorithm>  // std::min, std::max
#include <memory>

#include "hz/string_num.h"  // number_to_string
#include "hz/string_sprintf.h"  // string_sprintf
#include "hz/string_algo.h"  // string_join
#include "hz/fs.h"
#include "hz/format_unit.h"  // format_time_length
#include "rconfig/rconfig.h"  // rconfig::*

#include "applib/app_gtkmm_utils.h"  // app_gtkmm_*
#include "applib/storage_property_colors.h"
#include "applib/gui_utils.h"  // gui_show_error_dialog
#include "applib/smartctl_executor_gui.h"

#include "gsc_text_window.h"
#include "gsc_info_window.h"
#include "gsc_executor_error_dialog.h"
#include "gsc_startup_settings.h"



using namespace std::literals;



/// A label for StorageProperty
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
				std::string label_text = (label_string.markup ? Glib::ustring(label_string.label) : Glib::Markup::escape_text(
						label_string.label));
				Gtk::Label* label = Gtk::manage(new Gtk::Label());
				label->set_markup(label_text);
				label->set_padding(6, 0);
				label->set_alignment(Gtk::ALIGN_START);
				// label->set_ellipsize(Pango::ELLIPSIZE_END);
				label->set_selectable(true);
				label->set_can_focus(false);

				std::string fg;
				if (app_property_get_label_highlight_color(label_string.property->warning, fg)) {
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



	/// Cell renderer functions for list cells
	inline void app_list_cell_renderer_func(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& iter,
			Gtk::TreeModelColumn<const StorageProperty*> storage_column)
	{
		const StorageProperty* p = (*iter)[storage_column];
		if (auto* crt = dynamic_cast<Gtk::CellRendererText*>(cr)) {
			std::string fg, bg;
			if (app_property_get_row_highlight_colors(p->warning, fg, bg)) {
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
			if (p->is_value_type<StorageStatistic>() && p->get_value<StorageStatistic>().is_header) {
				crt->property_weight() = Pango::WEIGHT_BOLD;
			} else {
				crt->property_weight().reset_value();
			}
		}
	}



	/// Highlight a tab label according to \c warning
	inline void app_highlight_tab_label(Gtk::Widget* label_widget,
			WarningLevel warning, const Glib::ustring& original_label)
	{
		auto* label = dynamic_cast<Gtk::Label*>(label_widget);
		if (!label)
			return;

		if (warning == WarningLevel::none) {
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
			Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
			if (iter) {
				Glib::RefPtr<Gtk::TextMark> mark = buffer->get_mark((*iter)[mark_name_column]);
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
		int def_size_w = rconfig::get_data<int>("gui/info_window/default_size_w");
		int def_size_h = rconfig::get_data<int>("gui/info_window/default_size_h");
		if (def_size_w > 0 && def_size_h > 0) {
			set_default_size(def_size_w, def_size_h);
		}
	}

	// Create missing widgets
	auto* device_name_hbox = lookup_widget<Gtk::Box*>("device_name_label_hbox");
	if (device_name_hbox) {
		device_name_label = Gtk::manage(new Gtk::Label(_("No data available"), Gtk::ALIGN_START));
		device_name_label->set_selectable(true);
		device_name_label->show();
		device_name_hbox->pack_start(*device_name_label, true, true);
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
			"statistics_treeview",
			"selftest_log_treeview"
		};

		for (const auto& treeview_name : treeview_names) {
			auto* treeview = lookup_widget<Gtk::TreeView*>(treeview_name);
			treeview_menus[treeview_name] = new Gtk::Menu();  // deleted in window destructor

			treeview->signal_button_press_event().connect(
					sigc::bind(sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::on_treeview_button_press_event), treeview), treeview_menus[treeview_name]), false);  // before

			Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(_("Copy Selected Data"), true));
			item->signal_activate().connect(
					sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::on_treeview_menu_copy_clicked), treeview) );
			treeview_menus[treeview_name]->append(*item);

			treeview_menus[treeview_name]->show_all();  // Show all menu items when the menu pops up
		}
	}


	// ---------------


	// Set default texts on TextView-s, because glade's "text" property doesn't work
	// on them in gtkbuilder.
	if (auto* textview = lookup_widget<Gtk::TextView*>("error_log_textview")) {
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
	tab_identity_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("attributes_tab_label");
	tab_attributes_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("statistics_tab_label");
	tab_statistics_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("test_tab_label");
	tab_test_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("error_log_tab_label");
	tab_error_log_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("temperature_log_tab_label");
	tab_temperature_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("advanced_tab_label");
	tab_advanced_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("capabilities_tab_label");
	tab_capabilities_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("erc_tab_label");
	tab_erc_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("selective_selftest_tab_label");
	tab_selective_selftest_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("phy_tab_label");
	tab_phy_name = (tab_label ? tab_label->get_label() : "");

	tab_label = lookup_widget<Gtk::Label*>("directory_tab_label");
	tab_directory_name = (tab_label ? tab_label->get_label() : "");

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

	for (auto& iter : treeview_menus) {
		delete iter.second;
	}
}



void GscInfoWindow::set_drive(StorageDevicePtr d)
{
	if (drive)  // if an old drive is present, disconnect our callback from it.
		drive_changed_connection.disconnect();
	drive = std::move(d);
	drive_changed_connection = drive->signal_changed.connect(sigc::mem_fun(this,
			&GscInfoWindow::on_drive_changed));
}



void GscInfoWindow::fill_ui_with_info(bool scan, bool clear_ui, bool clear_tests)
{
	debug_out_info("app", DBG_FUNC_MSG << "Scan " << (scan ? "" : "not ") << "requested.\n");

	if (clear_ui)
		clear_ui_info(clear_tests);

	if (!drive->get_is_virtual()) {
		// fetch all smartctl info, even if it already has it (to refresh it).
		if (scan) {
			std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
			ex->create_running_dialog(this, Glib::ustring::compose(_("Running {command} on %1..."), drive->get_device_with_type()));
			std::string error_msg = drive->fetch_data_and_parse(ex);  // run it with GUI support

			if (!error_msg.empty()) {
				gsc_executor_error_dialog_show(_("Cannot retrieve SMART data"), error_msg, this);
				return;
			}
		}
	}

	// disable refresh button if virtual
	if (drive->get_is_virtual()) {
		auto* b = lookup_widget<Gtk::Button*>("refresh_info_button");
		if (b) {
			b->set_sensitive(false);
			app_gtkmm_set_widget_tooltip(*b, _("Cannot re-read information from virtual drive"));
		}
	}


	// hide all tabs except the first if smart is disabled, because they may contain
	// completely random data (smartctl does that sometimes).
	if (get_startup_settings().hide_tabs_on_smart_disabled) {
		bool smart_enabled = (drive->get_smart_status() == StorageDevice::Status::enabled);
		Gtk::Widget* note_page_box = nullptr;

		if ((note_page_box = lookup_widget("attributes_tab_vbox")) != nullptr) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("statistics_tab_vbox")) != nullptr) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("test_tab_vbox")) != nullptr) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("error_log_tab_vbox")) != nullptr) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("temperature_log_tab_vbox")) != nullptr) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("advanced_tab_vbox")) != nullptr) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if (auto* notebook = lookup_widget<Gtk::Notebook*>("main_notebook")) {
			notebook->set_show_tabs(smart_enabled);
		}
	}

	// Top label - short device information
	{
		std::string device = Glib::Markup::escape_text(drive->get_device_with_type());
		std::string model = Glib::Markup::escape_text(drive->get_model_name().empty() ? _("Unknown model") : drive->get_model_name());
		std::string drive_letters = Glib::Markup::escape_text(drive->format_drive_letters(false));

		/// Translators: %1 is device name, %2 is device model.
		this->set_title(Glib::ustring::compose(_("Device Information - %1: %2 - GSmartControl"), device, model));

		// Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label");
		if (device_name_label) {
			/// Translators: %1 is device name, %2 is drive letters (if not empty), %3 is device model.
			device_name_label->set_markup(Glib::ustring::compose(_("<b>Device:</b> %1%2  <b>Model:</b> %3"),
					device, (drive_letters.empty() ? "" : (" (<b>" + drive_letters + "</b>)")), model));
		}
	}


	// Fill the tabs with info

	// we need reference here - we take addresses of the elements
	const auto& props = drive->get_properties();  // it's a vector

	fill_ui_general(props);
	fill_ui_attributes(props);
	fill_ui_statistics(props);
	if (clear_tests) {
		fill_ui_self_test_info();
	}
	fill_ui_self_test_log(props);
	fill_ui_error_log(props);
	fill_ui_temperature_log(props);

	// Advanced tab
	WarningLevel caps_warning_level = fill_ui_capabilities(props);
	WarningLevel errc_warning_level = fill_ui_error_recovery(props);
	WarningLevel selective_warning_level = fill_ui_selective_self_test_log(props);
	WarningLevel dir_warning_level = fill_ui_directory(props);
	WarningLevel phy_warning_level = fill_ui_physical(props);

	WarningLevel max_advanced_tab_warning = std::max({
		caps_warning_level,
		errc_warning_level,
		selective_warning_level,
		dir_warning_level,
		phy_warning_level
	});

	// Advanced tab label
	app_highlight_tab_label(lookup_widget("advanced_tab_label"), max_advanced_tab_warning, tab_advanced_name);
}



void GscInfoWindow::clear_ui_info(bool clear_tests_too)
{
	// Note: We do NOT show/hide the notebook tabs here.
	// fill_ui_with_info() will do it all by itself.

	{
		this->set_title(_("Device Information - GSmartControl"));

		// Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label");
		if (device_name_label)
			device_name_label->set_text(_("No data available"));
	}

	{
		auto* identity_table = lookup_widget<Gtk::Grid*>("identity_table");
		if (identity_table) {
			// manually remove all children. without this visual corruption occurs.
			std::vector<Gtk::Widget*> children = identity_table->get_children();
			for (auto& widget : children) {
				identity_table->remove(*widget);
			}
		}

		// tab label
		app_highlight_tab_label(lookup_widget("general_tab_label"), WarningLevel::none, tab_identity_name);
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
		app_highlight_tab_label(lookup_widget("attributes_tab_label"), WarningLevel::none, tab_attributes_name);
	}

	{
		auto* label_vbox = lookup_widget<Gtk::Box*>("statistics_label_vbox");
		app_set_top_labels(label_vbox, std::vector<PropertyLabel>());

		if (auto* treeview = lookup_widget<Gtk::TreeView*>("statistics_treeview")) {
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("statistics_tab_label"), WarningLevel::none, tab_statistics_name);
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
				if (test_combo_model)
					test_combo_model->clear();
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
		app_highlight_tab_label(lookup_widget("test_tab_label"), WarningLevel::none, tab_test_name);
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
		app_highlight_tab_label(lookup_widget("error_log_tab_label"), WarningLevel::none, tab_error_log_name);
	}

	{
		auto* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("temperature_log_tab_label"), WarningLevel::none, tab_temperature_name);
	}

	// tab label
	app_highlight_tab_label(lookup_widget("advanced_tab_label"), WarningLevel::none, tab_advanced_name);

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
		app_highlight_tab_label(lookup_widget("capabilities_tab_label"), WarningLevel::none, tab_capabilities_name);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("erc_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("erc_tab_label"), WarningLevel::none, tab_erc_name);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("selective_selftest_tab_label"), WarningLevel::none, tab_selective_selftest_name);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("phy_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("phy_tab_label"), WarningLevel::none, tab_phy_name);
	}

	{
		if (auto* textview = lookup_widget<Gtk::TextView*>("directory_log_textview")) {
			textview->get_buffer()->set_text("\n"s + _("No data available"));
		}

		// tab label
		app_highlight_tab_label(lookup_widget("directory_tab_label"), WarningLevel::none, tab_directory_name);
	}
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
	if (auto* book = lookup_widget<Gtk::Notebook*>("main_notebook"))
		book->set_current_page(3);  // the Tests tab
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
	GscTextWindow<SmartctlOutputInstance>* win = GscTextWindow<SmartctlOutputInstance>::create();
	// make save visible and enable monospace font

	std::string output = this->drive->get_full_output();
	if (output.empty()) {
		output = this->drive->get_info_output();
	}

	win->set_text_from_command(_("Smartctl Output"), output);

	std::string filename = drive->get_save_filename();
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

	std::string filename = drive->get_save_filename();

	Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
	specific_filter->set_name(_("Text Files"));
	specific_filter->add_pattern("*.txt");

	Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
	all_filter->set_name(_("All Files"));
	all_filter->add_pattern("*");

#if GTK_CHECK_VERSION(3, 20, 0)
	std::unique_ptr<GtkFileChooserNative, decltype(&g_object_unref)> dialog(gtk_file_chooser_native_new(
			_("Save Data As..."), this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr),
			&g_object_unref);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), TRUE);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
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
			std::string file;
#if GTK_CHECK_VERSION(3, 20, 0)
			file = app_ustring_from_gchar(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog.get())));
			last_dir = hz::fs::u8path(file).parent_path().u8string();
#else
			file = dialog.get_filename();  // in fs encoding
			last_dir = dialog.get_current_folder();  // save for the future
#endif
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

			if (file.rfind(".txt") != (file.size() - std::strlen(".txt"))) {
				file += ".txt";
			}

			std::string data = this->drive->get_full_output();
			if (data.empty()) {
				data = this->drive->get_info_output();
			}
			std::error_code ec = hz::fs_file_put_contents(hz::fs::u8path(file), data);
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
	if (drive && drive->get_test_is_active()) {  // disallow close if test is active.
		gui_show_warn_dialog(_("Please wait until all tests are finished."), this);
	} else {
		delete this;  // deletes this object and nullifies instance
	}
}



void GscInfoWindow::on_test_type_combo_changed()
{
	auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");

	Gtk::TreeRow row = *(test_type_combo->get_active());
	if (row) {
		std::shared_ptr<SelfTest> test = row[test_combo_col_self_test];

		//debug_out_error("app", test->get_min_duration_seconds() << "\n");
		if (auto* min_duration_label = lookup_widget<Gtk::Label*>("min_duration_label")) {
			auto duration = test->get_min_duration_seconds();
			min_duration_label->set_text(duration == std::chrono::seconds(-1) ? C_("duration", "N/A")
					: (duration.count() == 0 ? C_("duration", "Unknown") : hz::format_time_length(duration)));
		}

		auto* test_description_textview = lookup_widget<Gtk::TextView*>("test_description_textview");
		if (test_description_textview != nullptr && test_description_textview->get_buffer())
			test_description_textview->get_buffer()->set_text(row[test_combo_col_description]);
	}
}



void GscInfoWindow::fill_ui_general(const std::vector<StorageProperty>& props)
{
	// filter out some properties
	std::vector<StorageProperty> id_props, version_props, health_props;

	for (auto&& p : props) {
		if (p.section == StorageProperty::Section::info) {
			if (p.generic_name == "smartctl_version_full") {
				version_props.push_back(p);
			} else if (p.generic_name == "smartctl_version") {
				continue;  // we use the full version string instead.
			} else {
				id_props.push_back(p);
			}
		} else if (p.section == StorageProperty::Section::data && p.subsection == StorageProperty::SubSection::health) {
			health_props.push_back(p);
		}
	}

	// put version after all the info
	for (auto&& p : version_props)
		id_props.push_back(p);

	// health is the last one
	for (auto&& p : health_props)
		id_props.push_back(p);


	auto* identity_table = lookup_widget<Gtk::Grid*>("identity_table");

	identity_table->hide();

	WarningLevel max_tab_warning = WarningLevel::none;
	int row = 0;

	for (auto&& p : id_props) {
		if (!p.show_in_ui) {
			continue;  // hide debug messages from smartctl
		}

		if (p.generic_name == "overall_health") {  // a little distance for this one
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
		if (app_property_get_label_highlight_color(p.warning, fg)) {
			name->set_markup("<span color=\"" + fg + "\">" + name->get_label() + "</span>");
			value->set_markup("<span color=\"" + fg + "\">" + value->get_label() + "</span>");
		}

		identity_table->attach(*name, 0, row, 1, 1);
		identity_table->attach(*value, 1, row, 1, 1);

		app_gtkmm_set_widget_tooltip(*name, p.get_description(), true);
		app_gtkmm_set_widget_tooltip(*value, // value->get_label() + "\n\n" +
				p.get_description(), true);

		if (int(p.warning) > int(max_tab_warning))
			max_tab_warning = p.warning;

		++row;
	}

	identity_table->show_all();

	// tab label
	app_highlight_tab_label(lookup_widget("general_tab_label"), max_tab_warning, tab_identity_name);
}



void GscInfoWindow::fill_ui_attributes(const std::vector<StorageProperty>& props)
{
	auto* treeview = lookup_widget<Gtk::TreeView*>("attributes_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// ID (int), Name, Flag (hex), Normalized Value (uint8), Worst (uint8), Thresh (uint8), Raw (int64), Type (string),
	// Updated (string), When Failed (string)

	Gtk::TreeModelColumn<int32_t> col_id;
	model_columns.add(col_id);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(col_id, *treeview, _("ID"), _("Attribute ID"), true);

	Gtk::TreeModelColumn<Glib::ustring> col_name;
	model_columns.add(col_name);
	num_tree_col = app_gtkmm_create_tree_view_column(col_name, *treeview,
			_("Name"), _("Attribute name (this is deduced from ID by smartctl and may be incorrect, as it's highly vendor-specific)"), true);
	treeview->set_search_column(col_name.index());
	auto* cr_name = dynamic_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_col));
	if (cr_name)
		cr_name->property_weight() = Pango::WEIGHT_BOLD;

	Gtk::TreeModelColumn<Glib::ustring> col_failed;
	model_columns.add(col_failed);
	num_tree_col = app_gtkmm_create_tree_view_column(col_failed, *treeview,
			_("Failed"), _("When failed (that is, the normalized value became equal to or less than threshold)"), true, true);

	Gtk::TreeModelColumn<std::string> col_value;
	model_columns.add(col_value);
	num_tree_col = app_gtkmm_create_tree_view_column(col_value, *treeview,
			C_("value", "Normalized"), _("Normalized value (highly vendor-specific; converted from Raw value by the drive's firmware)"), false);

	Gtk::TreeModelColumn<std::string> col_worst;
	model_columns.add(col_worst);
	num_tree_col = app_gtkmm_create_tree_view_column(col_worst, *treeview,
			C_("value", "Worst"), _("The worst normalized value recorded for this attribute during the drive's lifetime (with SMART enabled)"), false);

	Gtk::TreeModelColumn<std::string> col_threshold;
	model_columns.add(col_threshold);
	num_tree_col = app_gtkmm_create_tree_view_column(col_threshold, *treeview,
			C_("value", "Threshold"), _("Threshold for normalized value. Normalized value should be greater than threshold (unless vendor thinks otherwise)."), false);

	Gtk::TreeModelColumn<std::string> col_raw;
	model_columns.add(col_raw);
	num_tree_col = app_gtkmm_create_tree_view_column(col_raw, *treeview,
			_("Raw value"), _("Raw value as reported by drive. May or may not be sensible."), false);

	Gtk::TreeModelColumn<Glib::ustring> col_type;
	model_columns.add(col_type);
	num_tree_col = app_gtkmm_create_tree_view_column(col_type, *treeview,
			_("Type"), _("Alarm condition is reached when normalized value becomes less than or equal to threshold. Type indicates whether it's a signal of drive's pre-failure time or just an old age."), false, true);

	// Doesn't carry that much info. Advanced users can look at the flags.
// 		Gtk::TreeModelColumn<Glib::ustring> col_updated;
// 		model_columns.add(col_updated);
// 		tree_col = app_gtkmm_create_tree_view_column(col_updated, *treeview,
// 				"Updated", "The attribute is usually updated continuously, or during Offline Data Collection only. This column indicates that.", true);

	Gtk::TreeModelColumn<std::string> col_flag_value;
	model_columns.add(col_flag_value);
	num_tree_col = app_gtkmm_create_tree_view_column(col_flag_value, *treeview,
			_("Flags"), _("Flags") + "\n\n"s
					+ Glib::ustring::compose(_("If given in %1 format, the presence of each letter indicates that the flag is on."), "POSRCK+") + "\n"
					+ _("P: pre-failure attribute (if the attribute failed, the drive is failing)") + "\n"
					+ _("O: updated continuously (as opposed to updated on offline data collection)") + "\n"
					+ _("S: speed / performance attribute") + "\n"
					+ _("R: error rate") + "\n"
					+ _("C: event count") + "\n"
					+ _("K: auto-keep") + "\n"
					+ _("+: undocumented bits present"), false);

	Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
	model_columns.add(col_tooltip);
	treeview->set_tooltip_column(col_tooltip.index());


	Gtk::TreeModelColumn<const StorageProperty*> col_storage;
	model_columns.add(col_storage);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(col_id, Gtk::SORT_ASCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
	}


	WarningLevel max_tab_warning = WarningLevel::none;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (const auto& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::attributes)
			continue;

		// add non-attribute-type properties to label above
		if (!p.is_value_type<StorageAttribute>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);

			if (int(p.warning) > int(max_tab_warning))
				max_tab_warning = p.warning;
			continue;
		}

		const auto& attr = p.get_value<StorageAttribute>();

		std::string attr_type = StorageAttribute::get_attr_type_name(attr.attr_type);
		if (attr.attr_type == StorageAttribute::AttributeType::prefail)
			attr_type.append("<b>").append(attr_type).append("</b>");

		std::string fail_time = StorageAttribute::get_fail_time_name(attr.when_failed);
		if (attr.when_failed != StorageAttribute::FailTime::none)
			fail_time.append("<b>").append(fail_time).append("</b>");

		Gtk::TreeRow row = *(list_store->append());

		row[col_id] = attr.id;
		row[col_name] = p.displayable_name;
		row[col_flag_value] = attr.flag;  // it's a string, not int.
		row[col_value] = (attr.value.has_value() ? hz::number_to_string_locale(attr.value.value()) : "-");
		row[col_worst] = (attr.worst.has_value() ? hz::number_to_string_locale(attr.worst.value()) : "-");
		row[col_threshold] = (attr.threshold.has_value() ? hz::number_to_string_locale(attr.threshold.value()) : "-");
		row[col_raw] = attr.format_raw_value();
		row[col_type] = attr_type;
// 			row[col_updated] = StorageAttribute::get_update_type_name(attr.update_type);
		row[col_failed] = fail_time;
		row[col_tooltip] = p.get_description();
		row[col_storage] = &p;

		if (int(p.warning) > int(max_tab_warning))
			max_tab_warning = p.warning;
	}


	auto* label_vbox = lookup_widget<Gtk::Box*>("attributes_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("attributes_tab_label"), max_tab_warning, tab_attributes_name);
}



void GscInfoWindow::fill_ui_statistics(const std::vector<StorageProperty>& props)
{
	auto* treeview = lookup_widget<Gtk::TreeView*>("statistics_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	Gtk::TreeModelColumn<Glib::ustring> col_description;
	model_columns.add(col_description);
	num_tree_col = app_gtkmm_create_tree_view_column(col_description, *treeview,
			_("Description"), _("Entry description"), true);
	treeview->set_search_column(col_description.index());
// 		Gtk::CellRendererText* cr_name = dynamic_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_col));
// 		if (cr_name)
// 			cr_name->property_weight() = Pango::WEIGHT_BOLD ;

	Gtk::TreeModelColumn<std::string> col_value;
	model_columns.add(col_value);
	num_tree_col = app_gtkmm_create_tree_view_column(col_value, *treeview,
			_("Value"), Glib::ustring::compose(_("Value (can be normalized if '%1' flag is present)"), "N"), false);

	Gtk::TreeModelColumn<std::string> col_flags;
	model_columns.add(col_flags);
	num_tree_col = app_gtkmm_create_tree_view_column(col_flags, *treeview,
			_("Flags"), _("Flags") + "\n\n"s
					+ _("N: value is normalized") + "\n"
					+ _("D: supports Device Statistics Notification (DSN)") + "\n"
					+ _("C: monitored condition met") + "\n"  // Related to DSN? From the specification, it looks like something user-controllable.
					+ _("+: undocumented bits present"), false);

	Gtk::TreeModelColumn<std::string> col_page_offset;
	model_columns.add(col_page_offset);
	num_tree_col = app_gtkmm_create_tree_view_column(col_page_offset, *treeview,
			_("Page, Offset"), _("Page and offset of the entry"), false);

	Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
	model_columns.add(col_tooltip);
	treeview->set_tooltip_column(col_tooltip.index());

	Gtk::TreeModelColumn<const StorageProperty*> col_storage;
	model_columns.add(col_storage);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	treeview->set_model(list_store);
	// No sorting (we don't want to screw up the headers).

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
	}

	WarningLevel max_tab_warning = WarningLevel::none;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (const auto& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::devstat)
			continue;

		// add non-entry-type properties to label above
		if (!p.is_value_type<StorageStatistic>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);

			if (int(p.warning) > int(max_tab_warning))
				max_tab_warning = p.warning;
			continue;
		}

		Gtk::TreeRow row = *(list_store->append());

		const auto& st = p.get_value<StorageStatistic>();
		row[col_description] = (st.is_header ? p.displayable_name : ("    " + p.displayable_name));
		row[col_value] = st.format_value();
		row[col_flags] = st.flags;  // it's a string, not int.
		row[col_page_offset] = (st.is_header ? std::string()
				: hz::string_sprintf("0x%02x, 0x%03x", int(st.page), int(st.offset)));
		row[col_tooltip] = p.get_description();
		row[col_storage] = &p;

		if (int(p.warning) > int(max_tab_warning))
			max_tab_warning = p.warning;
	}

	auto* label_vbox = lookup_widget<Gtk::Box*>("statistics_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("statistics_tab_label"), max_tab_warning, tab_statistics_name);
}



void GscInfoWindow::fill_ui_self_test_info()
{
	auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");

	// don't check with get_model(), it comes pre-modeled from glade.
	if (!test_combo_model) {
		Gtk::TreeModelColumnRecord model_columns;

		// Test name, [description], [selftest_obj]
		model_columns.add(test_combo_col_name);  // we can use the column variable by value after this.
		model_columns.add(test_combo_col_description);
		model_columns.add(test_combo_col_self_test);

		test_combo_model = Gtk::ListStore::create(model_columns);
		test_type_combo->set_model(test_combo_model);

		// visible columns
		test_type_combo->clear();  // clear old (glade) cellrenderers

		test_type_combo->pack_start(test_combo_col_name);
	}

	// add possible tests

	Gtk::TreeModel::Row row;

	auto test_ioffline = std::make_shared<SelfTest>(drive, SelfTest::TestType::immediate_offline);
	if (test_ioffline->is_supported()) {
		row = *(test_combo_model->append());
		row[test_combo_col_name] = SelfTest::get_test_displayable_name(SelfTest::TestType::immediate_offline);
		row[test_combo_col_description] =
				_("Immediate Offline Test (also known as Immediate Offline Data Collection)"
				" is the manual version of Automatic Offline Data Collection, which, if enabled, is automatically run"
				" every four hours. If an error occurs during this test, it will be reported in Error Log. Besides that,"
				" its effects are visible only in that it updates the \"Offline\" Attribute values.");
		row[test_combo_col_self_test] = test_ioffline;
	}

	auto test_short = std::make_shared<SelfTest>(drive, SelfTest::TestType::short_test);
	if (test_short->is_supported()) {
		row = *(test_combo_model->append());
		row[test_combo_col_name] = SelfTest::get_test_displayable_name(SelfTest::TestType::short_test);
		row[test_combo_col_description] =
				_("Short self-test consists of a collection of test routines that have the highest chance"
				" of detecting drive problems. Its result is reported in the Self-Test Log."
				" Note that this test is in no way comprehensive. Its main purpose is to detect totally damaged"
				" drives without running a full surface scan."
				"\nNote: On some drives this actually runs several consequent tests, which may"
				" cause the program to display the test progress incorrectly.");  // seagate multi-pass test on 7200.11.
		row[test_combo_col_self_test] = test_short;
	}

	auto test_long = std::make_shared<SelfTest>(drive, SelfTest::TestType::long_test);
	if (test_long->is_supported()) {
		row = *(test_combo_model->append());
		row[test_combo_col_name] = SelfTest::get_test_displayable_name(SelfTest::TestType::long_test);
		row[test_combo_col_description] =
				_("Extended self-test examines complete disk surface and performs various test routines"
				" built into the drive. Its result is reported in the Self-Test Log.");
		row[test_combo_col_self_test] = test_long;
	}

	auto test_conveyance = std::make_shared<SelfTest>(drive, SelfTest::TestType::conveyance);
	if (test_conveyance->is_supported()) {
		row = *(test_combo_model->append());
		row[test_combo_col_name] = SelfTest::get_test_displayable_name(SelfTest::TestType::conveyance);
		row[test_combo_col_description] =
				_("Conveyance self-test is intended to identify damage incurred during transporting of the drive.");
		row[test_combo_col_self_test] = test_conveyance;
	}

	if (!test_combo_model->children().empty()) {
		test_type_combo->set_sensitive(true);
		test_type_combo->set_active(test_combo_model->children().begin());  // select first entry

		// At least one test is possible, so enable test button.
		// Note: we disable only Execute button on virtual drives. The combo is left
		// sensitive so that the user can see which tests are supported by the drive.
		auto* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button");
		if (test_execute_button)
			test_execute_button->set_sensitive(!drive->get_is_virtual());
	}
}



void GscInfoWindow::fill_ui_self_test_log(const std::vector<StorageProperty>& props)
{
	auto* treeview = lookup_widget<Gtk::TreeView*>("selftest_log_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// Test num., Type, Status, % Completed, Lifetime hours, LBA of the first error

	Gtk::TreeModelColumn<uint32_t> col_num;
	model_columns.add(col_num);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(col_num, *treeview,
			_("Test #"), _("Test # (greater may mean newer or older depending on drive model)"), true);
	auto* cr_test_num = dynamic_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_col));
	if (cr_test_num)
		cr_test_num->property_weight() = Pango::WEIGHT_BOLD ;

	Gtk::TreeModelColumn<std::string> col_type;
	model_columns.add(col_type);
	num_tree_col = app_gtkmm_create_tree_view_column(col_type, *treeview,
			_("Type"), _("Type of the test performed"), true);
	treeview->set_search_column(col_type.index());

	Gtk::TreeModelColumn<std::string> col_status;
	model_columns.add(col_status);
	num_tree_col = app_gtkmm_create_tree_view_column(col_status, *treeview,
			_("Status"), _("Test completion status"), true);

	Gtk::TreeModelColumn<std::string> col_percent;
	model_columns.add(col_percent);
	num_tree_col = app_gtkmm_create_tree_view_column(col_percent, *treeview,
			_("% Completed"), _("Percentage of the test completed. Instantly-aborted tests have 10%, while unsupported ones <i>may</i> have 100%."), true, false, true);

	Gtk::TreeModelColumn<std::string> col_hours;
	model_columns.add(col_hours);
	num_tree_col = app_gtkmm_create_tree_view_column(col_hours, *treeview,
			_("Lifetime hours"), _("During which hour of the drive's (powered on) lifetime did the test complete (or abort)"), true);

	Gtk::TreeModelColumn<std::string> col_lba;
	model_columns.add(col_lba);
	num_tree_col = app_gtkmm_create_tree_view_column(col_lba, *treeview,
			_("LBA of the first error"), _("LBA of the first error (if an LBA-related error happened)"), true);

	Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
	model_columns.add(col_tooltip);
	treeview->set_tooltip_column(col_tooltip.index());

	Gtk::TreeModelColumn<const StorageProperty*> col_storage;
	model_columns.add(col_storage);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(col_num, Gtk::SORT_ASCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
	}


	WarningLevel max_tab_warning = WarningLevel::none;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::selftest_log)
			continue;

		if (p.generic_name == "selftest_log")  // the whole section, we don't need it
			continue;

		// add non-entry properties to label above
		if (!p.is_value_type<StorageSelftestEntry>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);

			if (int(p.warning) > int(max_tab_warning))
				max_tab_warning = p.warning;
			continue;
		}

		Gtk::TreeRow row = *(list_store->append());

		const auto& sse = p.get_value<StorageSelftestEntry>();

		row[col_num] = sse.test_num;
		row[col_type] = sse.type;
		row[col_status] = sse.get_status_str();
		row[col_percent] = hz::number_to_string_locale(100 - sse.remaining_percent) + "%";
		row[col_hours] = sse.format_lifetime_hours();
		row[col_lba] = sse.lba_of_first_error;
		// There are no descriptions in self-test log entries, so don't display
		// "No description available" for all of them.
		// row[col_tooltip] = p.get_description();
		row[col_storage] = &p;

		if (int(p.warning) > int(max_tab_warning))
			max_tab_warning = p.warning;
	}


	auto* label_vbox = lookup_widget<Gtk::Box*>("selftest_log_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("test_tab_label"), max_tab_warning, tab_test_name);
}



void GscInfoWindow::fill_ui_error_log(const std::vector<StorageProperty>& props)
{
	auto* treeview = lookup_widget<Gtk::TreeView*>("error_log_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// Error Number, Lifetime Hours, State, Type, Details, [tooltips]

	Gtk::TreeModelColumn<uint32_t> col_num;
	model_columns.add(col_num);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(col_num, *treeview,
			_("Error #"), _("Error # in the error log (greater means newer)"), true);
	if (auto* cr_name = dynamic_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_col)))
		cr_name->property_weight() = Pango::WEIGHT_BOLD ;

	Gtk::TreeModelColumn<std::string> col_hours;
	model_columns.add(col_hours);
	num_tree_col = app_gtkmm_create_tree_view_column(col_hours, *treeview,
			_("Lifetime hours"), _("During which hour of the drive's (powered on) lifetime did the error happen."), true);

	Gtk::TreeModelColumn<std::string> col_state;
	model_columns.add(col_state);
	num_tree_col = app_gtkmm_create_tree_view_column(col_state, *treeview,
			C_("power", "State"), _("Power state of the drive when the error occurred"), false);

	Gtk::TreeModelColumn<Glib::ustring> col_type;
	model_columns.add(col_type);
	num_tree_col = app_gtkmm_create_tree_view_column(col_type, *treeview,
			_("Type"), _("Type of error"), true);

	Gtk::TreeModelColumn<std::string> col_details;
	model_columns.add(col_details);
	num_tree_col = app_gtkmm_create_tree_view_column(col_details, *treeview,
			_("Details"), _("Additional details (e.g. LBA where the error occurred, etc...)"), true);

	Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
	model_columns.add(col_tooltip);
	treeview->set_tooltip_column(col_tooltip.index());

	Gtk::TreeModelColumn<const StorageProperty*> col_storage;
	model_columns.add(col_storage);

	Gtk::TreeModelColumn<Glib::ustring> col_mark_name;
	model_columns.add(col_mark_name);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(col_num, Gtk::SORT_DESCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
	}


	WarningLevel max_tab_warning = WarningLevel::none;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::error_log)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "error_log") {
			if (auto* textview = lookup_widget<Gtk::TextView*>("error_log_textview")) {
				// Add complete error log to textview window.
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				if (buffer) {
					buffer->set_text("\n" + Glib::ustring::compose(_("Complete error log: %1"), "\n\n" + p.get_value<std::string>()));

					// set marks so we can scroll to them

					if (!error_log_row_selected_conn.connected()) {  // avoid double-connect
						error_log_row_selected_conn = treeview->get_selection()->signal_changed().connect(
								sigc::bind(sigc::bind(sigc::ptr_fun(on_error_log_treeview_row_selected), col_mark_name), this));
					}

					Gtk::TextIter titer = buffer->begin();
					Gtk::TextIter match_start, match_end;
					// TODO Change this for json
					while (titer.forward_search("\nError ", Gtk::TEXT_SEARCH_TEXT_ONLY, match_start, match_end)) {
						match_start.forward_char();  // place after newline
						match_end.forward_word_end();  // include error number
						titer = match_end;  // continue searching from here

						Glib::ustring mark_name = match_start.get_slice(match_end);  // e.g. "Error 3"
						buffer->create_mark(mark_name, titer);
					}
				}
			}


			// add non-tree properties to label above
		} else if (!p.is_value_type<StorageErrorBlock>()) {
			label_strings.emplace_back(p.displayable_name + ": " + p.format_value(), &p);
			if (p.generic_name == "error_log_error_count")
				label_strings.back().label += " "s + _("(Note: The number of entries may be limited to the newest ones)");

		} else {
			const auto& eb = p.get_value<StorageErrorBlock>();

			std::string type_details = eb.type_more_info;

			Gtk::TreeRow row = *(list_store->append());
			row[col_num] = eb.error_num;
			row[col_hours] = eb.format_lifetime_hours();
			row[col_state] = eb.device_state;
			row[col_type] = StorageErrorBlock::get_displayable_error_types(eb.reported_types);
			row[col_details] = (type_details.empty() ? "-" : type_details);  // e.g. OBS has no details
			// There are no descriptions in self-test log entries, so don't display
			// "No description available" for all of them.
			row[col_tooltip] = p.get_description();
			row[col_storage] = &p;
			row[col_mark_name] = Glib::ustring::compose(_("Error %1"), eb.error_num);
		}

		if (int(p.warning) > int(max_tab_warning))
			max_tab_warning = p.warning;
	}

	auto* label_vbox = lookup_widget<Gtk::Box*>("error_log_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// inner tab label
	app_highlight_tab_label(lookup_widget("error_log_tab_label"), max_tab_warning, tab_error_log_name);
}



void GscInfoWindow::fill_ui_temperature_log(const std::vector<StorageProperty>& props)
{
	auto* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");

	WarningLevel max_tab_warning = WarningLevel::none;
	std::vector<PropertyLabel> label_strings;  // outside-of-tree properties

	std::string temperature;
	StorageProperty temp_property;
	enum { temp_attr2 = 1, temp_attr1, temp_stat, temp_sct };  // less important to more important
	int temp_prop_source = 0;

	for (const auto& p : props) {
		// Find temperature
		if (temp_prop_source < temp_sct && p.generic_name == "sct_temperature_celsius") {
			temperature = hz::number_to_string_locale(p.get_value<int64_t>());
			temp_property = p;
			temp_prop_source = temp_sct;
		}
		if (temp_prop_source < temp_stat && p.generic_name == "stat_temperature_celsius") {
			temperature = hz::number_to_string_locale(p.get_value<StorageStatistic>().value_int);
			temp_property = p;
			temp_prop_source = temp_stat;
		}
		if (temp_prop_source < temp_attr1 && p.generic_name == "attr_temperature_celsius") {
			temperature = hz::number_to_string_locale(p.get_value<StorageAttribute>().raw_value_int);
			temp_property = p;
			temp_prop_source = temp_attr1;
		}
		if (temp_prop_source < temp_attr2 && p.generic_name == "attr_temperature_celsius_x10") {
			temperature = hz::number_to_string_locale(p.get_value<StorageAttribute>().raw_value_int / 10);
			temp_property = p;
			temp_prop_source = temp_attr2;
		}

		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::temperature_log)
			continue;

		if (p.generic_name == "sct_unsupported" && p.get_value<bool>()) {  // only show if unsupported
			label_strings.emplace_back(_("SCT temperature commands not supported."), &p);
			if (int(p.warning) > int(max_tab_warning))
				max_tab_warning = p.warning;
			continue;
		}

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "scttemp_log") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete SCT temperature log: %1"), "\n\n" + p.get_value<std::string>()));
		}
	}

	if (temperature.empty()) {
		temperature = C_("value", "Unknown");
	} else {
		temperature = Glib::ustring::compose(C_("temperature", "%1 C"), temperature);
	}
	temp_property.set_description(_("Current drive temperature in Celsius."));  // overrides attribute description
	label_strings.emplace_back(Glib::ustring::compose(_("Current temperature: %1"), "<b>" + temperature + "</b>"), &temp_property, true);
	if (int(temp_property.warning) > int(max_tab_warning))
		max_tab_warning = temp_property.warning;


	auto* label_vbox = lookup_widget<Gtk::Box*>("temperature_log_label_vbox");
	app_set_top_labels(label_vbox, label_strings);

	// tab label
	app_highlight_tab_label(lookup_widget("temperature_log_tab_label"), max_tab_warning, tab_temperature_name);
}



WarningLevel GscInfoWindow::fill_ui_capabilities(const std::vector<StorageProperty>& props)
{
	auto* treeview = lookup_widget<Gtk::TreeView*>("capabilities_treeview");

	Gtk::TreeModelColumnRecord model_columns;
	[[maybe_unused]] int num_tree_col = 0;

	// N, Name, Flag, Capabilities, [tooltips]

	Gtk::TreeModelColumn<int> col_index;
	model_columns.add(col_index);  // we can use the column variable by value after this.
	num_tree_col = app_gtkmm_create_tree_view_column(col_index, *treeview, _("#"), _("Entry #"), true);

	Gtk::TreeModelColumn<Glib::ustring> col_name;
	model_columns.add(col_name);
	num_tree_col = app_gtkmm_create_tree_view_column(col_name, *treeview, _("Name"), _("Name"), true);
	treeview->set_search_column(col_name.index());
	auto* cr_name = dynamic_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_col));
	if (cr_name)
		cr_name->property_weight() = Pango::WEIGHT_BOLD ;

	Gtk::TreeModelColumn<std::string> col_flag_value;
	model_columns.add(col_flag_value);
	num_tree_col = app_gtkmm_create_tree_view_column(col_flag_value, *treeview, _("Flags"), _("Flags"), false);

	Gtk::TreeModelColumn<Glib::ustring> col_str_values;
	model_columns.add(col_str_values);
	num_tree_col = app_gtkmm_create_tree_view_column(col_str_values, *treeview, _("Capabilities"), _("Capabilities"), false);

	Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
	model_columns.add(col_tooltip);
	treeview->set_tooltip_column(col_tooltip.index());

	Gtk::TreeModelColumn<const StorageProperty*> col_storage;
	model_columns.add(col_storage);


	// create a TreeModel (ListStore)
	Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
	list_store->set_sort_column(col_index, Gtk::SORT_ASCENDING);  // default sort
	treeview->set_model(list_store);

	for (int i = 0; i < int(treeview->get_n_columns()); ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		tcol->set_cell_data_func(*(tcol->get_first_cell()),
				sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
	}


	WarningLevel max_tab_warning = WarningLevel::none;
	int index = 1;

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::capabilities)
			continue;

		Glib::ustring name = p.displayable_name;
		std::string flag_value;
		Glib::ustring str_value;

		if (p.is_value_type<StorageCapability>()) {
			flag_value = hz::number_to_string_nolocale(p.get_value<StorageCapability>().flag_value, 16);  // 0xXX
			str_value = hz::string_join(p.get_value<StorageCapability>().strvalues, "\n");
		} else {
			// no flag value here
			str_value = p.format_value();
		}

		Gtk::TreeRow row = *(list_store->append());
		row[col_index] = index;
		row[col_name] = name;
		row[col_flag_value] = (flag_value.empty() ? "-" : flag_value);
		row[col_str_values] = str_value;
		row[col_tooltip] = p.get_description();
		row[col_storage] = &p;

		if (int(p.warning) > int(max_tab_warning))
			max_tab_warning = p.warning;

		++index;
	}

	// tab label
	app_highlight_tab_label(lookup_widget("capabilities_tab_label"), max_tab_warning, tab_capabilities_name);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_error_recovery(const std::vector<StorageProperty>& props)
{
	auto* textview = lookup_widget<Gtk::TextView*>("erc_log_textview");

	WarningLevel max_tab_warning = WarningLevel::none;

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::erc_log)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "scterc_log") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete SCT Error Recovery Control settings: %1"), "\n\n" + p.get_value<std::string>()));
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("erc_tab_label"), max_tab_warning, tab_erc_name);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_selective_self_test_log(const std::vector<StorageProperty>& props)
{
	auto* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview");

	WarningLevel max_tab_warning = WarningLevel::none;

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::selective_selftest_log)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "SubSection::selective_selftest_log") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete selective self-test log: %1"), "\n\n" + p.get_value<std::string>()));
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("selective_selftest_tab_label"), max_tab_warning, tab_selective_selftest_name);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_physical(const std::vector<StorageProperty>& props)
{
	auto* textview = lookup_widget<Gtk::TextView*>("phy_log_textview");

	WarningLevel max_tab_warning = WarningLevel::none;

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::phy_log)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "sataphy_log") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete phy log: %1"), "\n\n" + p.get_value<std::string>()));
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("phy_tab_label"), max_tab_warning, tab_phy_name);

	return max_tab_warning;
}



WarningLevel GscInfoWindow::fill_ui_directory(const std::vector<StorageProperty>& props)
{
	auto* textview = lookup_widget<Gtk::TextView*>("directory_log_textview");

	WarningLevel max_tab_warning = WarningLevel::none;

	for (auto&& p : props) {
		if (p.section != StorageProperty::Section::data || p.subsection != StorageProperty::SubSection::directory_log)
			continue;

		// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
		if (p.generic_name == "directory_log") {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\n" + Glib::ustring::compose(_("Complete directory log: %1"), "\n\n" + p.get_value<std::string>()));
		}
	}

	// tab label
	app_highlight_tab_label(lookup_widget("directory_tab_label"), max_tab_warning, tab_directory_name);

	return max_tab_warning;
}



// Note: Another loop like this may run inside it for another drive.
gboolean GscInfoWindow::test_idle_callback(void* data)
{
	auto* self = static_cast<GscInfoWindow*>(data);
	DBG_ASSERT(self);

	if (!self->current_test)  // shouldn't happen
		return FALSE;  // stop

	auto* test_completion_progressbar =
			self->lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar");


	bool active = true;

	do {  // goto
		if (!self->current_test->is_active()) {  // check status
			active = false;
			break;
		}

		int8_t rem_percent = self->current_test->get_remaining_percent();
		std::string rem_percent_str = (rem_percent == -1 ? C_("value", "Unknown") : hz::number_to_string_locale(100 - rem_percent));

		auto poll_in = self->current_test->get_poll_in_seconds();  // sec


		// One update() is performed by start(), so do the timeout first.

		// Wait until next poll (up to several minutes). Meanwhile, interpolate
		// the remaining time, update the progressbar, etc...
		if (self->test_timer_poll.elapsed() < static_cast<double>(poll_in.count())) {  // elapsed() is seconds in double.

			// Update progress bar right after poll, plus every 5 seconds.
			if (self->test_force_bar_update || self->test_timer_bar.elapsed() >= 5.) {

				auto rem_seconds = self->current_test->get_remaining_seconds();

				if (test_completion_progressbar) {
					std::string rem_seconds_str = (rem_seconds == std::chrono::seconds(-1) ? C_("duration", "Unknown") : hz::format_time_length(rem_seconds));

					Glib::ustring bar_str;

					if (self->test_error_msg.empty()) {
						bar_str = Glib::ustring::compose(_("Test completion: %1%%; ETA: %2"), rem_percent_str, rem_seconds_str);
					} else {
						bar_str = self->test_error_msg;  // better than popup every few seconds
					}

					test_completion_progressbar->set_text(bar_str);
					test_completion_progressbar->set_fraction(std::max(0., std::min(1., 1. - (rem_percent / 100.))));
				}

				self->test_force_bar_update = false;
				self->test_timer_bar.start();  // restart
			}

			if (!self->current_test->is_active()) {  // check status
				active = false;
				break;
			}


		} else {  // it's poll time

			if (!self->current_test->is_active()) {  // the inner loop stopped, stop this one too
				active = false;
				break;
			}

			std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
			ex->create_running_dialog(self);

			self->test_error_msg = self->current_test->update(ex);
			if (!self->test_error_msg.empty()) {
// 				gui_show_error_dialog("Cannot monitor test progress", self->test_error_msg, this);  // better show in progressbar.
				self->current_test->force_stop(ex);  // what else can we do?
				active = false;
				break;
			}

			self->test_timer_poll.start();  // restart it
			self->test_force_bar_update = true;  // force progressbar / ETA update on the next tick
		}


	} while (false);


	if (active) {
		return TRUE;  // continue the idle callback
	}


	// Test is finished, clean up

	self->test_timer_poll.stop();  // just in case
	self->test_timer_bar.stop();  // just in case

	StorageSelftestEntry::Status status = self->current_test->get_status();

	bool aborted = false;
	StorageSelftestEntry::StatusSeverity severity = StorageSelftestEntry::StatusSeverity::none;
	std::string result_msg;

	if (!self->test_error_msg.empty()) {
		aborted = true;
		severity = StorageSelftestEntry::StatusSeverity::error;
		result_msg = Glib::ustring::compose(_("<b>Test aborted:</b> %1"), self->test_error_msg);

	} else {
		severity = StorageSelftestEntry::get_status_severity(status);
		if (status == StorageSelftestEntry::Status::aborted_by_host) {
			aborted = true;
			result_msg = "<b>"s + _("Test was manually aborted.") + "</b>";  // it's a StatusSeverity::none message

		} else {
			result_msg = Glib::ustring::compose(_("<b>Test result:</b> %1."), StorageSelftestEntry::get_status_displayable_name(status));

			// It may not reach 100% somehow, so do it manually.
			if (test_completion_progressbar)
				test_completion_progressbar->set_fraction(1.);  // yes, we're at 100% anyway (at least logically).
		}
	}

	if (severity != StorageSelftestEntry::StatusSeverity::none) {
		result_msg += "\n"s + _("Check the Self-Test Log for more information.");
	}


	if (auto* test_type_combo = self->lookup_widget<Gtk::ComboBox*>("test_type_combo"))
		test_type_combo->set_sensitive(true);

	if (auto* test_execute_button = self->lookup_widget<Gtk::Button*>("test_execute_button"))
		test_execute_button->set_sensitive(true);

	if (test_completion_progressbar)
		test_completion_progressbar->set_text(aborted ? _("Test aborted") : _("Test completed"));

	if (auto* test_stop_button = self->lookup_widget<Gtk::Button*>("test_stop_button"))
		test_stop_button->set_sensitive(false);

	Gtk::StockID stock_id = Gtk::Stock::DIALOG_ERROR;
	if (severity == StorageSelftestEntry::StatusSeverity::none) {
		stock_id = Gtk::Stock::DIALOG_INFO;
	} else if (severity == StorageSelftestEntry::StatusSeverity::warning) {
		stock_id = Gtk::Stock::DIALOG_WARNING;
	}

	// we use large icon size here because the icons we use are from dialogs.
	// unfortunately, there are no non-dialog icons of this sort.
	if (auto* test_result_image = self->lookup_widget<Gtk::Image*>("test_result_image"))
		test_result_image->set(stock_id, Gtk::ICON_SIZE_DND);


	if (auto* test_result_label = self->lookup_widget<Gtk::Label*>("test_result_label"))
		test_result_label->set_markup(result_msg);

	if (auto* test_result_hbox = self->lookup_widget<Gtk::Box*>("test_result_hbox"))
		test_result_hbox->show();


	self->refresh_info(false);  // don't clear the tests tab

	return FALSE;  // stop idle callback
}





void GscInfoWindow::on_test_execute_button_clicked()
{
	auto* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");

	Gtk::TreeRow row = *(test_type_combo->get_active());
	if (!row)
		return;

	std::shared_ptr<SelfTest> test_from_combo = row[test_combo_col_self_test];
	auto test = std::make_shared<SelfTest>(drive, test_from_combo->get_test_type());
	if (!test)
		return;

	// hide previous test results from GUI
	if (auto* test_result_hbox = this->lookup_widget<Gtk::Box*>("test_result_hbox"))
		test_result_hbox->hide();

	std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
	ex->create_running_dialog(this);

	std::string error_msg = test->start(ex);  // this runs update() too.
	if (!error_msg.empty()) {
		/// Translators: %1 is test name
		gui_show_error_dialog(Glib::ustring::compose(_("Cannot run %1"), SelfTest::get_test_displayable_name(test->get_test_type())), error_msg, this);
		return;
	}

	current_test = test;


	// Switch GUI to "running test" mode

	if (test_type_combo)
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
	test_error_msg.clear();
	test_timer_poll.start();
	test_timer_bar.start();
	test_force_bar_update = true;


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
	if (!current_test)
		return;

	std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
	ex->create_running_dialog(this);

	std::string error_msg = current_test->force_stop(ex);
	if (!error_msg.empty()) {
		/// Translators: %1 is test name
		gui_show_error_dialog(Glib::ustring::compose(_("Cannot stop %1"), SelfTest::get_test_displayable_name(current_test->get_test_type())), error_msg, this);
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
	if (!drive)
		return;
	bool test_active = drive->get_test_is_active();

	// disable refresh button if test is active or if it's a virtual drive
	if (auto* refresh_info_button = lookup_widget<Gtk::Button*>("refresh_info_button"))
		refresh_info_button->set_sensitive(!test_active && !drive->get_is_virtual());

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
		bool selection_empty = treeview->get_selection()->get_selected_rows().empty();
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

	int num_cols = static_cast<int>(treeview->get_n_columns());
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
			GType type = list_store->get_column_type(j);
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
