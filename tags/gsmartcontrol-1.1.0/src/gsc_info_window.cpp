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

#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape
#include <vector>  // better use vector, it's needed by others too
#include <algorithm>  // std::min, std::max

#include "hz/scoped_ptr.h"
#include "hz/down_cast.h"
#include "hz/string_num.h"  // number_to_string
#include "hz/string_sprintf.h"  // string_sprintf
#include "hz/string_algo.h"  // string_join
#include "hz/fs_file.h"  // hz::File
#include "hz/format_unit.h"  // format_time_length
#include "rconfig/rconfig_mini.h"  // rconfig::*

#include "applib/app_gtkmm_utils.h"  // app_gtkmm_*
#include "applib/storage_property_colors.h"
#include "applib/gui_utils.h"  // gui_show_error_dialog
#include "applib/smartctl_executor_gui.h"

#include "gsc_text_window.h"
#include "gsc_info_window.h"
#include "gsc_executor_error_dialog.h"



/// A label for StorageProperty
struct PropertyLabel {
	/// Constructor
	PropertyLabel(const std::string& label_, const StorageProperty* prop, bool markup_ = false) :
		label(label_), property(prop), markup(markup_)
	{ }

	std::string label;  ///< Label text
	const StorageProperty* property;  ///< Storage property
	bool markup;  ///< Whether the label text uses markup
};


/// A vector of PropertyLabel objects
typedef std::vector<PropertyLabel> label_list_t;




namespace {


	/// Set "top" labels - the generic text at the top of each tab page.
	inline void app_set_top_labels(Gtk::Box* vbox, const label_list_t& label_strings)
	{
		if (!vbox)
			return;

		// remove all first
		std::vector<Gtk::Widget*> wv = vbox->get_children();
		for (unsigned int i = 0; i < wv.size(); ++i) {
			vbox->remove(*(wv[i]));
			delete wv[i];  // since it's without parent anymore, it won't be auto-deleted.
		}

		vbox->set_visible(!label_strings.empty());

		if (label_strings.empty()) {
			// add one label only
// 			Gtk::Label* label = Gtk::manage(new Gtk::Label("No data available", Gtk::ALIGN_START));
// 			label->set_padding(6, 0);
// 			vbox->pack_start(*label, false, false);

		} else {

			// add one label per element
			for (label_list_t::const_iterator iter = label_strings.begin(); iter != label_strings.end(); ++iter) {
				std::string label_text = (iter->markup ? Glib::ustring(iter->label) : Glib::Markup::escape_text(iter->label));
				Gtk::Label* label = Gtk::manage(new Gtk::Label());
				label->set_markup(label_text);
				label->set_padding(6, 0);
				label->set_alignment(Gtk::ALIGN_START);
				// label->set_ellipsize(Pango::ELLIPSIZE_END);
				label->set_selectable(true);
				label->set_can_focus(false);

				std::string fg;
				if (app_property_get_label_highlight_color(iter->property->warning, fg))
					label->set_markup("<span color=\"" + fg + "\">" + label_text + "</span>");
				vbox->pack_start(*label, false, false);

				// set it after packing, else the old tooltips api won't have anything to attach them to.
				app_gtkmm_set_widget_tooltip(*label, // label_text + "\n\n" +  // add label text too, in case it's ellipsized
						iter->property->get_description(), true);  // already markupped

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
		Gtk::CellRendererText* crt = hz::down_cast<Gtk::CellRendererText*>(cr);
		if (crt) {
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
			if (p->value_statistic.is_header) {
				crt->property_weight() = Pango::WEIGHT_BOLD;
			} else {
				crt->property_weight().reset_value();
			}
		}
	}



	/// Highlight a tab label according to \c warning
	inline void app_highlight_tab_label(Gtk::Widget* label_widget,
			StorageProperty::warning_t warning, const Glib::ustring& original_label)
	{
		Gtk::Label* label = hz::down_cast<Gtk::Label*>(label_widget);
		if (!label)
			return;

		if (warning == StorageProperty::warning_none) {
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
		Gtk::TreeView* treeview = window->lookup_widget<Gtk::TreeView*>("error_log_treeview");
		Gtk::TextView* textview = window->lookup_widget<Gtk::TextView*>("error_log_textview");
		Glib::RefPtr<Gtk::TextBuffer> buffer;
		if (treeview && textview && (buffer = textview->get_buffer())) {
			Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
			if (iter) {
				Glib::RefPtr<Gtk::TextMark> mark = buffer->get_mark((*iter)[mark_name_column]);
				if (mark)
					textview->scroll_to(mark, 0., 0., 0.);
			}
		}
	}



}




GscInfoWindow::GscInfoWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
		: AppUIResWidget<GscInfoWindow, true>(gtkcobj, ref_ui),
		device_name_label(0), test_force_bar_update(true)
{
	// Size
	{
		int def_size_w = 0, def_size_h = 0;
		rconfig::get_data("gui/info_window/default_size_w", def_size_w);
		rconfig::get_data("gui/info_window/default_size_h", def_size_h);
		if (def_size_w > 0 && def_size_h > 0) {
			set_default_size(def_size_w, def_size_h);
		}
	}

	// Create missing widgets
	Gtk::Box* device_name_hbox = lookup_widget<Gtk::Box*>("device_name_label_hbox");
	if (device_name_hbox) {
		device_name_label = Gtk::manage(new Gtk::Label("No data available", Gtk::ALIGN_START));
		device_name_label->set_selectable(true);
		device_name_label->show();
		device_name_hbox->pack_start(*device_name_label, true, true);
	}


	// Connect callbacks

	APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	Gtk::Button* refresh_info_button = 0;
	APP_UI_RES_AUTO_CONNECT(refresh_info_button, clicked);

	Gtk::Button* view_output_button = 0;
	APP_UI_RES_AUTO_CONNECT(view_output_button, clicked);

	Gtk::Button* save_info_button = 0;
	APP_UI_RES_AUTO_CONNECT(save_info_button, clicked);

	Gtk::Button* close_window_button = 0;
	APP_UI_RES_AUTO_CONNECT(close_window_button, clicked);

	Gtk::ComboBox* test_type_combo = 0;
	APP_UI_RES_AUTO_CONNECT(test_type_combo, changed);

	Gtk::Button* test_execute_button = 0;
	APP_UI_RES_AUTO_CONNECT(test_execute_button, clicked);

	Gtk::Button* test_stop_button = 0;
	APP_UI_RES_AUTO_CONNECT(test_stop_button, clicked);


	// Accelerators
	if (close_window_button) {
		close_window_button->add_accelerator("clicked", this->get_accel_group(), GDK_KEY_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}

	// Context menu in treeviews
	{
		std::vector<std::string> treeview_names;
		treeview_names.push_back("attributes_treeview");
		treeview_names.push_back("statistics_treeview");
		treeview_names.push_back("selftest_log_treeview");

		for (std::size_t i = 0; i < treeview_names.size(); ++i) {
			std::string treeview_name = treeview_names[i];
			Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>(treeview_name);
			treeview_menus[treeview_name] = new Gtk::Menu();  // deleted in window destructor

			treeview->signal_button_press_event().connect(
					sigc::bind(sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::on_treeview_button_press_event), treeview), treeview_menus[treeview_name]), false);  // before

			Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem("Copy Selected Data", true));
			item->signal_activate().connect(
					sigc::bind(sigc::mem_fun(*this, &GscInfoWindow::on_treeview_menu_copy_clicked), treeview) );
			treeview_menus[treeview_name]->append(*item);

			treeview_menus[treeview_name]->show_all();  // Show all menu items when the menu pops up
		}
	}


	// ---------------


	// Set default texts on TextView-s, because glade's "text" property doesn't work
	// on them in gtkbuilder.
	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("error_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}
	}
	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}
	}
	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}
	}
	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("erc_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}
	}
	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("phy_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}
	}
	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("directory_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}
	}


	// Save their original texts so that we can apply markup to them.
	Gtk::Label* tab_label = 0;

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
	for (std::map<std::string, Gtk::Menu*>::iterator iter = treeview_menus.begin(); iter != treeview_menus.end(); ++iter) {
		delete iter->second;
	}
}



void GscInfoWindow::obj_destroy()
{
	// Main window size. We don't store position to avoid overlaps
	{
		int window_w = 0, window_h = 0;
		get_size(window_w, window_h);
		rconfig::set_data("gui/info_window/default_size_w", window_w);
		rconfig::set_data("gui/info_window/default_size_h", window_h);
	}

}



void GscInfoWindow::set_drive(StorageDeviceRefPtr d)
{
	if (drive)  // if an old drive is present, disconnect our callback from it.
		drive_changed_connection.disconnect();
	drive = d;
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
			SmartctlExecutorGuiRefPtr ex(new SmartctlExecutorGui());
			ex->create_running_dialog(this, "Running %s on " + drive->get_device_with_type() + "...");
			std::string error_msg = drive->fetch_data_and_parse(ex);  // run it with GUI support

			if (!error_msg.empty()) {
				gsc_executor_error_dialog_show("Cannot retrieve SMART data", error_msg, this);
				return;
			}

		// it's already parsed
// 		} else {
// 			std::string error_msg = drive->parse_data();
// 			if (!error_msg.empty()) {
// 				gui_show_error_dialog("Cannot interpret SMART data", error_msg, this);
// 				return;
// 			}
		}
	}

	// disable refresh button if virtual
	if (drive->get_is_virtual()) {
		Gtk::Button* b = lookup_widget<Gtk::Button*>("refresh_info_button");
		if (b) {
			b->set_sensitive(false);
			app_gtkmm_set_widget_tooltip(*b, "Cannot re-read information from virtual drive");
		}
	}


	// hide all tabs except the first if smart is disabled, because they may contain
	// completely random data (smartctl does that sometimes).
	bool hide_tabs = true;
	rconfig::get_data("/runtime/gui/hide_tabs_on_smart_disabled", hide_tabs);

	if (hide_tabs) {
		bool smart_enabled = (drive->get_smart_status() == StorageDevice::status_enabled);
		Gtk::Widget* note_page_box = 0;

		if ((note_page_box = lookup_widget("attributes_tab_vbox")) != 0) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("statistics_tab_vbox")) != 0) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("test_tab_vbox")) != 0) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("error_log_tab_vbox")) != 0) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("temperature_log_tab_vbox")) != 0) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if ((note_page_box = lookup_widget("advanced_tab_vbox")) != 0) {
			if (smart_enabled) { note_page_box->show(); } else { note_page_box->hide(); }
		}
		if (Gtk::Notebook* notebook = lookup_widget<Gtk::Notebook*>("main_notebook")) {
			notebook->set_show_tabs(smart_enabled);
		}
	}


	{
		std::string device = Glib::Markup::escape_text(drive->get_device_with_type());
		std::string model = Glib::Markup::escape_text(drive->get_model_name().empty() ? "Unknown model" : drive->get_model_name());
		std::string drive_letters = Glib::Markup::escape_text(drive->format_drive_letters());

		this->set_title("Device Information - " + device + ": " + model + " - GSmartControl");

		// Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label");
		if (device_name_label) {
			device_name_label->set_markup(
					"<b>Device: </b>" + device + (drive_letters.empty() ? "" : (" (<b>" + drive_letters + "</b>)"))
					+ "  <b>Model: </b>" + model);
		}
	}


	// Fill the tabs with info

	// we need reference here - we take addresses of the elements
	const SmartctlParser::prop_list_t& props = drive->get_properties();  // it's a vector
	typedef SmartctlParser::prop_list_t::const_iterator prop_iterator;



	// ------------------------------------------- Identity, version, overall health

	do {

		// filter out some properties
		SmartctlParser::prop_list_t id_props, version_props, health_props;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section == StorageProperty::section_info) {
				if (iter->generic_name == "smartctl_version_full") {
					version_props.push_back(*iter);
				} else if (iter->generic_name == "smartctl_version") {
					continue;  // we use the full version string instead.
				} else {
					id_props.push_back(*iter);
				}
			} else if (iter->section == StorageProperty::section_data && iter->subsection == StorageProperty::subsection_health) {
				health_props.push_back(*iter);
			}
		}

		// put version after all the info
		for (prop_iterator iter = version_props.begin(); iter != version_props.end(); ++iter)
			id_props.push_back(*iter);

		// health is the last one
		for (prop_iterator iter = health_props.begin(); iter != health_props.end(); ++iter)
			id_props.push_back(*iter);


		Gtk::Grid* identity_table = lookup_widget<Gtk::Grid*>("identity_table");
		if (!identity_table)
			break;

		identity_table->hide();

		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		int row = 0;

		for (prop_iterator iter = id_props.begin(); iter != id_props.end(); ++iter) {
			if (!iter->show_in_ui) {
				continue;  // hide debug messages from smartctl
			}

			if (iter->generic_name == "overall_health") {  // a little distance for this one
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
			name->set_markup("<b>" + Glib::Markup::escape_text(iter->readable_name) + "</b>");

			// If the above is Label, then this has to be Label too, else it will shrink
			// and "name" will take most of the horizontal space. If "name" is set to shrink,
			// then it stops being right-aligned.
			Gtk::Label* value = Gtk::manage(new Gtk::Label());
			// value->set_ellipsize(Pango::ELLIPSIZE_END);
			value->set_alignment(Gtk::ALIGN_START);  // left-align
			value->set_selectable(true);
			value->set_can_focus(false);
			value->set_markup(Glib::Markup::escape_text(iter->format_value()));

			std::string fg;
			if (app_property_get_label_highlight_color(iter->warning, fg)) {
				name->set_markup("<span color=\"" + fg + "\">"+ name->get_label() + "</span>");
				value->set_markup("<span color=\"" + fg + "\">"+ value->get_label() + "</span>");
			}

			identity_table->attach(*name, 0, row, 1, 1);
			identity_table->attach(*value, 1, row, 1, 1);

			app_gtkmm_set_widget_tooltip(*name, iter->get_description(), true);
			app_gtkmm_set_widget_tooltip(*value, // value->get_label() + "\n\n" +
					iter->get_description(), true);

			if (int(iter->warning) > int(max_tab_warning))
				max_tab_warning = iter->warning;

			++row;
		}

		identity_table->show_all();

		// tab label
		app_highlight_tab_label(lookup_widget("general_tab_label"), max_tab_warning, tab_identity_name);

	} while (false);





	// ------------------------------------------- Attributes


	do {

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("attributes_treeview");
		if (!treeview)
			break;

		Gtk::TreeModelColumnRecord model_columns;
		unsigned int num_tree_cols = 0;

		// ID (int), Name, Flag (hex), Normalized Value (uint8), Worst (uint8), Thresh (uint8), Raw (int64), Type (string),
		// Updated (string), When Failed (string)

		Gtk::TreeModelColumn<int32_t> col_id;
		model_columns.add(col_id);  // we can use the column variable by value after this.
		num_tree_cols = app_gtkmm_create_tree_view_column(col_id, *treeview, "ID", "Attribute ID", true);

		Gtk::TreeModelColumn<Glib::ustring> col_name;
		model_columns.add(col_name);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_name, *treeview,
				"Name", "Attribute name (this is deduced from ID by smartctl and may be incorrect, as it's highly vendor-specific)", true);
		treeview->set_search_column(col_name.index());
		Gtk::CellRendererText* cr_name = hz::down_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_cols - 1));
		if (cr_name)
			cr_name->property_weight() = Pango::WEIGHT_BOLD;

		Gtk::TreeModelColumn<Glib::ustring> col_failed;
		model_columns.add(col_failed);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_failed, *treeview,
				"Failed", "When failed (that is, the normalized value became equal to or less than threshold)", true, true);

		Gtk::TreeModelColumn<std::string> col_value;
		model_columns.add(col_value);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_value, *treeview,
				"Norm-ed value", "Normalized value (highly vendor-specific; converted from Raw value by the drive's firmware)", false);

		Gtk::TreeModelColumn<std::string> col_worst;
		model_columns.add(col_worst);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_worst, *treeview,
				"Worst", "The worst normalized value recorded for this attribute during the drive's lifetime (with SMART enabled)", false);

		Gtk::TreeModelColumn<std::string> col_threshold;
		model_columns.add(col_threshold);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_threshold, *treeview,
				"Threshold", "Threshold for normalized value. Normalized value should be greater than threshold (unless vendor thinks otherwise).", false);

		Gtk::TreeModelColumn<std::string> col_raw;
		model_columns.add(col_raw);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_raw, *treeview,
				"Raw value", "Raw value as reported by drive. May or may not be sensible.", false);

		Gtk::TreeModelColumn<Glib::ustring> col_type;
		model_columns.add(col_type);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_type, *treeview,
				"Type", "Alarm condition is reached when if normalized value becomes less than or equal to threshold. Type indicates whether it's a signal of drive's pre-failure time or just an old age.", false, true);

		// Doesn't carry that much info. Advanced users can look at the flags.
// 		Gtk::TreeModelColumn<Glib::ustring> col_updated;
// 		model_columns.add(col_updated);
// 		num_tree_cols = app_gtkmm_create_tree_view_column(col_updated, *treeview,
// 				"Updated", "The attribute is usually updated continuously, or during Offline Data Collection only. This column indicates that.", true);

		Gtk::TreeModelColumn<std::string> col_flag_value;
		model_columns.add(col_flag_value);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_flag_value, *treeview,
				"Flags", "Flags\n\n"
				"If given in POSRCK+ format, the presence of each letter indicates that the flag is on.\n"
				"P: pre-failure attribute (if the attribute failed, the drive is failing)\n"
				"O: updated continuously (as opposed to updated on offline data collection)\n"
				"S: speed / performance attribute\n"
				"R: error rate\n"
				"C: event count\n"
				"K: auto-keep\n"
				"+: undocumented bits present", false);

		Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
		model_columns.add(col_tooltip);
		treeview->set_tooltip_column(col_tooltip.index());


		Gtk::TreeModelColumn<const StorageProperty*> col_storage;
		model_columns.add(col_storage);


		// create a TreeModel (ListStore)
		Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
		list_store->set_sort_column(col_id, Gtk::SORT_ASCENDING);  // default sort
		treeview->set_model(list_store);

		for (unsigned int i = 0; i < num_tree_cols; ++i) {
			Gtk::TreeViewColumn* tcol = treeview->get_column(i);
			tcol->set_cell_data_func(*(tcol->get_first_cell()),
						sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
		}


		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		label_list_t label_strings;  // outside-of-tree properties

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_attributes)
				continue;

			// add non-attribute-type properties to label above
			if (iter->value_type != StorageProperty::value_type_attribute) {
				label_strings.push_back(PropertyLabel(iter->readable_name + ": " + iter->format_value(), &(*iter)));

				if (int(iter->warning) > int(max_tab_warning))
					max_tab_warning = iter->warning;
				continue;
			}

			std::string attr_type = StorageAttribute::get_attr_type_name(iter->value_attribute.attr_type);
			if (iter->value_attribute.attr_type == StorageAttribute::attr_type_prefail)
				attr_type = "<b>" + attr_type + "</b>";

			std::string fail_time = StorageAttribute::get_fail_time_name(iter->value_attribute.when_failed);
			if (iter->value_attribute.when_failed != StorageAttribute::fail_time_none)
				fail_time = "<b>" + fail_time + "</b>";

			Gtk::TreeRow row = *(list_store->append());

			row[col_id] = iter->value_attribute.id;
			row[col_name] = iter->readable_name;
			row[col_flag_value] = iter->value_attribute.flag;  // it's a string, not int.
			row[col_value] = (iter->value_attribute.value.defined() ? hz::number_to_string(iter->value_attribute.value.value()) : "-");
			row[col_worst] = (iter->value_attribute.worst.defined() ? hz::number_to_string(iter->value_attribute.worst.value()) : "-");
			row[col_threshold] = (iter->value_attribute.threshold.defined() ? hz::number_to_string(iter->value_attribute.threshold.value()) : "-");
			row[col_raw] = iter->value_attribute.format_raw_value();
			row[col_type] = attr_type;
// 			row[col_updated] = StorageAttribute::get_update_type_name(iter->value_attribute.update_type);
			row[col_failed] = fail_time;
			row[col_tooltip] = iter->get_description();
			row[col_storage] = &(*iter);

			if (int(iter->warning) > int(max_tab_warning))
				max_tab_warning = iter->warning;
		}


		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("attributes_label_vbox");
		app_set_top_labels(label_vbox, label_strings);

		// tab label
		app_highlight_tab_label(lookup_widget("attributes_tab_label"), max_tab_warning, tab_attributes_name);

	} while (false);



	// ------------------------------------------- Statistics


	do {

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("statistics_treeview");
		if (!treeview)
			break;

		Gtk::TreeModelColumnRecord model_columns;
		unsigned int num_tree_cols = 0;

		Gtk::TreeModelColumn<Glib::ustring> col_description;
		model_columns.add(col_description);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_description, *treeview,
				"Description", "Entry description", true);
		treeview->set_search_column(col_description.index());
// 		Gtk::CellRendererText* cr_name = hz::down_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_cols - 1));
// 		if (cr_name)
// 			cr_name->property_weight() = Pango::WEIGHT_BOLD ;

		Gtk::TreeModelColumn<std::string> col_value;
		model_columns.add(col_value);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_value, *treeview,
				"Value", "Value (can be normalized if 'N' flag is present)", false);

		Gtk::TreeModelColumn<std::string> col_flags;
		model_columns.add(col_flags);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_flags, *treeview,
				"Flags", "Flags\n\n"
				"N: value is normalized\n"
				"D: supports Device Statistics Notification (DSN)\n"
				"C: monitored condition met\n"  // Related to DSN? From the specification, it looks like something user-controllable.
				"+: undocumented bits present", false);

		Gtk::TreeModelColumn<std::string> col_page_offset;
		model_columns.add(col_page_offset);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_page_offset, *treeview,
				"Page, Offset", "Page and offset of the entry", false);

		Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
		model_columns.add(col_tooltip);
		treeview->set_tooltip_column(col_tooltip.index());


		Gtk::TreeModelColumn<const StorageProperty*> col_storage;
		model_columns.add(col_storage);


		// create a TreeModel (ListStore)
		Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
		treeview->set_model(list_store);
		// No sorting (we don't want to screw up the headers).

		for (unsigned int i = 0; i < num_tree_cols; ++i) {
			Gtk::TreeViewColumn* tcol = treeview->get_column(i);
			tcol->set_cell_data_func(*(tcol->get_first_cell()),
						sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
		}



		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		label_list_t label_strings;  // outside-of-tree properties

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_devstat)
				continue;

			// add non-entry-type properties to label above
			if (iter->value_type != StorageProperty::value_type_statistic) {
				label_strings.push_back(PropertyLabel(iter->readable_name + ": " + iter->format_value(), &(*iter)));

				if (int(iter->warning) > int(max_tab_warning))
					max_tab_warning = iter->warning;
				continue;
			}

			Gtk::TreeRow row = *(list_store->append());

			row[col_description] = (iter->value_statistic.is_header ? iter->readable_name : ("    " + iter->readable_name));
			row[col_value] = iter->value_statistic.value;
			row[col_flags] = iter->value_statistic.flags;  // it's a string, not int.
			row[col_page_offset] = (iter->value_statistic.is_header ? std::string()
					: hz::string_sprintf("0x%02x, 0x%03x", int(iter->value_statistic.page), int(iter->value_statistic.offset)));
			row[col_tooltip] = iter->get_description();
			row[col_storage] = &(*iter);

			if (int(iter->warning) > int(max_tab_warning))
				max_tab_warning = iter->warning;
		}


		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("statistics_label_vbox");
		app_set_top_labels(label_vbox, label_strings);

		// tab label
		app_highlight_tab_label(lookup_widget("statistics_tab_label"), max_tab_warning, tab_statistics_name);

	} while (false);




	// ------------------------------------------- Tests


	if (clear_tests) { do {  // Modify only after clearing tests!

		Gtk::ComboBox* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");
		if (!test_type_combo)  // huh?
			break;

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

		SelfTestPtr test_ioffline(new SelfTest(drive, SelfTest::type_ioffline));
		if (test_ioffline->is_supported()) {
			row = *(test_combo_model->append());
			row[test_combo_col_name] = SelfTest::get_test_name(SelfTest::type_ioffline);
			row[test_combo_col_description] =
				"Immediate Offline Test (also known as Immediate Offline Data Collection)"
				" is the manual version of Automatic Offline Data Collection, which, if enabled, is automatically run"
				" every four hours. If an error occurs during this test, it will be reported in Error Log. Besides that,"
				" its effects are visible only in that it updates the \"Offline\" Attribute values.";
			row[test_combo_col_self_test] = test_ioffline;
		}

		SelfTestPtr test_short(new SelfTest(drive, SelfTest::type_short));
		if (test_short->is_supported()) {
			row = *(test_combo_model->append());
			row[test_combo_col_name] = SelfTest::get_test_name(SelfTest::type_short);
			row[test_combo_col_description] =
				"Short self-test consists of a collection of test routines that have the highest chance"
				" of detecting drive problems. Its result is reported in the Self-Test Log."
				" Note that this test is in no way comprehensive. Its main purpose is to detect totally damaged"
				" drives without running the full surface scan."
				"\nNote: On some drives this actually runs several consequent tests, which may"
				" cause the program to display the test progress incorrectly.";  // seagate multi-pass test on 7200.11.
			row[test_combo_col_self_test] = test_short;
		}

		SelfTestPtr test_long(new SelfTest(drive, SelfTest::type_long));
		if (test_long->is_supported()) {
			row = *(test_combo_model->append());
			row[test_combo_col_name] = SelfTest::get_test_name(SelfTest::type_long);
			row[test_combo_col_description] =
				"Extended self-test examines complete disk surface and performs various test routines"
				" built into the drive. Its result is reported in the Self-Test Log.";
			row[test_combo_col_self_test] = test_long;
		}

		SelfTestPtr test_conveyance(new SelfTest(drive, SelfTest::type_conveyance));
		if (test_conveyance->is_supported()) {
			row = *(test_combo_model->append());
			row[test_combo_col_name] = SelfTest::get_test_name(SelfTest::type_conveyance);
			row[test_combo_col_description] =
				"Conveyance self-test is intended to identify damage incurred during transporting of the drive.";
			row[test_combo_col_self_test] = test_conveyance;
		}


		if (!test_combo_model->children().empty()) {
			test_type_combo->set_sensitive(true);
			test_type_combo->set_active(test_combo_model->children().begin());  // select first entry

			// At least one test is possible, so enable test button.
			// Note: we disable only Execute button on virtual drives. The combo is left
			// sensitive so that the user can see which tests are supported by the drive.
			Gtk::Button* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button");
			if (test_execute_button)
				test_execute_button->set_sensitive(!drive->get_is_virtual());
		}

	} while (false); }  // end if (clear_tests)




	// ------------------------------------------- Selftest Log (Tests tab)


	do {
		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("selftest_log_treeview");
		if (!treeview)
			break;

		Gtk::TreeModelColumnRecord model_columns;
		unsigned int num_tree_cols = 0;

		// Test num., Type, Status, % Completed, Lifetime hours, LBA of the first error

		Gtk::TreeModelColumn<int32_t> col_num;
		model_columns.add(col_num);  // we can use the column variable by value after this.
		num_tree_cols = app_gtkmm_create_tree_view_column(col_num, *treeview,
				"Test #", "Test # (greater may mean newer or older depending on drive model)", true);
		Gtk::CellRendererText* cr_test_num = hz::down_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_cols - 1));
		if (cr_test_num)
			cr_test_num->property_weight() = Pango::WEIGHT_BOLD ;

		Gtk::TreeModelColumn<std::string> col_type;
		model_columns.add(col_type);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_type, *treeview,
				"Type", "Type of the test performed", true);
		treeview->set_search_column(col_type.index());

		Gtk::TreeModelColumn<std::string> col_status;
		model_columns.add(col_status);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_status, *treeview,
				"Status", "Test completion status", true);

		Gtk::TreeModelColumn<std::string> col_percent;
		model_columns.add(col_percent);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_percent, *treeview,
				"% Completed", "Percentage of the test completed. Instantly-aborted tests have 10%, while unsupported ones _may_ have 100%.", true);

		Gtk::TreeModelColumn<std::string> col_hours;
		model_columns.add(col_hours);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_hours, *treeview,
				"Lifetime hours", "During which hour of the drive's (powered on) lifetime did the test complete (or abort)", true);

		Gtk::TreeModelColumn<std::string> col_lba;
		model_columns.add(col_lba);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_lba, *treeview,
				"LBA of the first error", "LBA of the first error (if an LBA-related error happened)", true);

		Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
		model_columns.add(col_tooltip);
		treeview->set_tooltip_column(col_tooltip.index());

		Gtk::TreeModelColumn<const StorageProperty*> col_storage;
		model_columns.add(col_storage);


		// create a TreeModel (ListStore)
		Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
		list_store->set_sort_column(col_num, Gtk::SORT_ASCENDING);  // default sort
		treeview->set_model(list_store);

		for (unsigned int i = 0; i < num_tree_cols; ++i) {
			Gtk::TreeViewColumn* tcol = treeview->get_column(i);
			tcol->set_cell_data_func(*(tcol->get_first_cell()),
						sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
		}


		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		label_list_t label_strings;  // outside-of-tree properties

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_selftest_log)
				continue;

			if (iter->generic_name == "selftest_log")  // the whole section, we don't need it
				continue;

			// add non-entry properties to label above
			if (iter->value_type != StorageProperty::value_type_selftest_entry) {
				label_strings.push_back(PropertyLabel(iter->readable_name + ": " + iter->format_value(), &(*iter)));

				if (int(iter->warning) > int(max_tab_warning))
					max_tab_warning = iter->warning;
				continue;
			}

			Gtk::TreeRow row = *(list_store->append());

			row[col_num] = iter->value_selftest_entry.test_num;
			row[col_type] = iter->value_selftest_entry.type;
			row[col_status] = iter->value_selftest_entry.get_status_str();
			row[col_percent] = hz::number_to_string(100 - iter->value_selftest_entry.remaining_percent) + "%";
			row[col_hours] = iter->value_selftest_entry.format_lifetime_hours();
			row[col_lba] = iter->value_selftest_entry.lba_of_first_error;
			// There are no descriptions in self-test log entries, so don't display
			// "No description available" for all of them.
			// row[col_tooltip] = iter->get_description();
			row[col_storage] = &(*iter);

			if (int(iter->warning) > int(max_tab_warning))
				max_tab_warning = iter->warning;
		}


		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("selftest_log_label_vbox");
		app_set_top_labels(label_vbox, label_strings);

		// tab label
		app_highlight_tab_label(lookup_widget("test_tab_label"), max_tab_warning, tab_test_name);

	} while (false);



	// ------------------------------------------- Error Log


	do {
		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("error_log_treeview");
		if (!treeview)
			break;

		Gtk::TreeModelColumnRecord model_columns;
		unsigned int num_tree_cols = 0;

		// Error Number, Lifetime Hours, State, Type, Details, [tooltips]

		Gtk::TreeModelColumn<uint32_t> col_num;
		model_columns.add(col_num);  // we can use the column variable by value after this.
		num_tree_cols = app_gtkmm_create_tree_view_column(col_num, *treeview,
				"Error #", "Error # in the error log (greater means newer)", true);
		Gtk::CellRendererText* cr_name = hz::down_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_cols - 1));
		if (cr_name)
			cr_name->property_weight() = Pango::WEIGHT_BOLD ;

		Gtk::TreeModelColumn<std::string> col_hours;
		model_columns.add(col_hours);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_hours, *treeview,
				"Lifetime hours", "During which hour of the drive's (powered on) lifetime did the error happen.", true);

		Gtk::TreeModelColumn<std::string> col_state;
		model_columns.add(col_state);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_state, *treeview,
				"State", "Power state of the drive when the error occurred", false);

		Gtk::TreeModelColumn<Glib::ustring> col_type;
		model_columns.add(col_type);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_type, *treeview,
				"Type", "Type of error", true);

		Gtk::TreeModelColumn<std::string> col_details;
		model_columns.add(col_details);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_details, *treeview,
				"Details", "Additional details (e.g. LBA where the error occurred, etc...)", true);

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

		for (unsigned int i = 0; i < num_tree_cols; ++i) {
			Gtk::TreeViewColumn* tcol = treeview->get_column(i);
			tcol->set_cell_data_func(*(tcol->get_first_cell()),
						sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
		}


		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		label_list_t label_strings;  // outside-of-tree properties

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_error_log)
				continue;

			// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
			if (iter->generic_name == "error_log") {
				Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("error_log_textview");
				if (textview) {
					// Add complete error log to textview window.
					Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
					if (buffer) {
						buffer->set_text("\nComplete error log:\n\n" + iter->value_string);

						// set marks so we can scroll to them

						if (!error_log_row_selected_conn.connected()) {  // avoid double-connect
							error_log_row_selected_conn = treeview->get_selection()->signal_changed().connect(
								sigc::bind(sigc::bind(sigc::ptr_fun(on_error_log_treeview_row_selected), col_mark_name), this));
						}

						Gtk::TextIter titer = buffer->begin();
						Gtk::TextIter match_start, match_end;
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
			} else if (iter->value_type != StorageProperty::value_type_error_block) {
				label_strings.push_back(PropertyLabel(iter->readable_name + ": " + iter->format_value(), &(*iter)));
				if (iter->generic_name == "error_log_error_count")
					label_strings.back().label += " (Note: The number of entries may be limited to the newest ones)";

			} else {
				std::string type_details = iter->value_error_block.type_more_info;

				Gtk::TreeRow row = *(list_store->append());
				row[col_num] = iter->value_error_block.error_num;
				row[col_hours] = iter->value_error_block.format_lifetime_hours();
				row[col_state] = iter->value_error_block.device_state;
				row[col_type] = StorageErrorBlock::get_readable_error_types(iter->value_error_block.reported_types);
				row[col_details] = (type_details.empty() ? "-" : type_details);  // e.g. OBS has no details
				// There are no descriptions in self-test log entries, so don't display
				// "No description available" for all of them.
				row[col_tooltip] = iter->get_description();
				row[col_storage] = &(*iter);
				row[col_mark_name] = "Error " + hz::number_to_string(iter->value_error_block.error_num);
			}

			if (int(iter->warning) > int(max_tab_warning))
				max_tab_warning = iter->warning;
		}

		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("error_log_label_vbox");
		app_set_top_labels(label_vbox, label_strings);

		// inner tab label
		app_highlight_tab_label(lookup_widget("error_log_tab_label"), max_tab_warning, tab_error_log_name);

	} while (false);




	// ------------------------------------------- Temperature log


	do {

		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");
		if (!textview)
			break;

		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		label_list_t label_strings;  // outside-of-tree properties

		std::string temperature;
		StorageProperty temp_property;
		enum { temp_attr2 = 1, temp_attr1, temp_stat, temp_sct };  // less important to more important
		int temp_prop_source = 0;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			// Find temperature
			if (temp_prop_source < temp_sct && iter->generic_name == "sct_temperature_celsius") {
				temperature = hz::number_to_string(iter->value_integer);
				temp_property = *iter;
				temp_prop_source = temp_sct;
			}
			if (temp_prop_source < temp_stat && iter->generic_name == "stat_temperature_celsius") {
				temperature = hz::number_to_string(iter->value_statistic.value_int);
				temp_property = *iter;
				temp_prop_source = temp_stat;
			}
			if (temp_prop_source < temp_attr1 && iter->generic_name == "attr_temperature_celsius") {
				temperature = hz::number_to_string(iter->value_attribute.raw_value_int);
				temp_property = *iter;
				temp_prop_source = temp_attr1;
			}
			if (temp_prop_source < temp_attr2 && iter->generic_name == "attr_temperature_celsius_x10") {
				temperature = hz::number_to_string(iter->value_attribute.raw_value_int / 10);
				temp_property = *iter;
				temp_prop_source = temp_attr2;
			}

			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_temperature_log)
				continue;

			if (iter->generic_name == "sct_unsupported" && iter->value_bool) {  // only show if unsupported
				label_strings.push_back(PropertyLabel("SCT temperature commands not supported.", &(*iter)));
				if (int(iter->warning) > int(max_tab_warning))
					max_tab_warning = iter->warning;
				continue;
			}

			// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
			if (iter->generic_name == "scttemp_log") {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text("\nComplete SCT temperature log:\n\n" + iter->value_string);
			}
		}

		if (temperature.empty()) {
			temperature = "Unknown";
		} else {
			temperature += " C";
		}
		temp_property.set_description("Current drive temperature in Celsius.");  // overrides attribute description
		label_strings.push_back(PropertyLabel("Current temperature: <b>" + temperature + "</b>", &temp_property, true));
		if (int(temp_property.warning) > int(max_tab_warning))
			max_tab_warning = temp_property.warning;


		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("temperature_log_label_vbox");
		app_set_top_labels(label_vbox, label_strings);

		// tab label
		app_highlight_tab_label(lookup_widget("temperature_log_tab_label"), max_tab_warning, tab_temperature_name);

	} while (false);




	// ------------------------------------------- Advanced tab - Capabilities


	StorageProperty::warning_t max_advanced_tab_warning = StorageProperty::warning_none;


	do {

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("capabilities_treeview");
		if (!treeview)
			break;

		Gtk::TreeModelColumnRecord model_columns;
		unsigned int num_tree_cols = 0;

		// N, Name, Flag, Capabilities, [tooltips]

		Gtk::TreeModelColumn<int> col_index;
		model_columns.add(col_index);  // we can use the column variable by value after this.
		num_tree_cols = app_gtkmm_create_tree_view_column(col_index, *treeview, "#", "Entry #", true);

		Gtk::TreeModelColumn<Glib::ustring> col_name;
		model_columns.add(col_name);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_name, *treeview, "Name", "Name", true);
		treeview->set_search_column(col_name.index());
		Gtk::CellRendererText* cr_name = hz::down_cast<Gtk::CellRendererText*>(treeview->get_column_cell_renderer(num_tree_cols - 1));
		if (cr_name)
			cr_name->property_weight() = Pango::WEIGHT_BOLD ;

		Gtk::TreeModelColumn<std::string> col_flag_value;
		model_columns.add(col_flag_value);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_flag_value, *treeview, "Flags", "Flags", false);

		Gtk::TreeModelColumn<Glib::ustring> col_str_values;
		model_columns.add(col_str_values);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_str_values, *treeview, "Capabilities", "Capabilities", false);

		Gtk::TreeModelColumn<Glib::ustring> col_tooltip;
		model_columns.add(col_tooltip);
		treeview->set_tooltip_column(col_tooltip.index());

		Gtk::TreeModelColumn<const StorageProperty*> col_storage;
		model_columns.add(col_storage);


		// create a TreeModel (ListStore)
		Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(model_columns);
		list_store->set_sort_column(col_index, Gtk::SORT_ASCENDING);  // default sort
		treeview->set_model(list_store);

		for (unsigned int i = 0; i < num_tree_cols; ++i) {
			Gtk::TreeViewColumn* tcol = treeview->get_column(i);
			tcol->set_cell_data_func(*(tcol->get_first_cell()),
						sigc::bind(sigc::ptr_fun(app_list_cell_renderer_func), col_storage));
		}


		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;
		int index = 1;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_capabilities)
				continue;

			Glib::ustring name = iter->readable_name;
			std::string flag_value;
			Glib::ustring str_value;

			if (iter->value_type == StorageProperty::value_type_capability) {
				flag_value = hz::number_to_string(iter->value_capability.flag_value, 16);  // 0xXX
				str_value = hz::string_join(iter->value_capability.strvalues, "\n");
			} else {
				// no flag value here
				str_value = iter->format_value();
			}

			Gtk::TreeRow row = *(list_store->append());
			row[col_index] = index;
			row[col_name] = name;
			row[col_flag_value] = (flag_value.empty() ? "-" : flag_value);
			row[col_str_values] = str_value;
			row[col_tooltip] = iter->get_description();
			row[col_storage] = &(*iter);

			if (int(iter->warning) > int(max_tab_warning))
				max_tab_warning = iter->warning;

			++index;
		}

		if (int(max_tab_warning) > int(max_advanced_tab_warning))
			max_advanced_tab_warning = max_tab_warning;

		// tab label
		app_highlight_tab_label(lookup_widget("capabilities_tab_label"), max_tab_warning, tab_capabilities_name);

	} while (false);




	// ------------------------------------------- Advanced tab - ERC log


	do {

		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("erc_log_textview");
		if (!textview)
			break;

		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_erc_log)
				continue;

			// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
			if (iter->generic_name == "scterc_log") {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text("\nComplete SCT Error Recovery Control settings:\n\n" + iter->value_string);
			}
		}

		if (int(max_tab_warning) > int(max_advanced_tab_warning))
			max_advanced_tab_warning = max_tab_warning;

		// tab label
		app_highlight_tab_label(lookup_widget("erc_tab_label"), max_tab_warning, tab_erc_name);

	} while (false);




	// ------------------------------------------- Advanced tab - Selective self-test log


	do {

		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview");
		if (!textview)
			break;

		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_selective_selftest_log)
				continue;

			// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
			if (iter->generic_name == "selective_selftest_log") {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text("\nComplete selective self-test log:\n\n" + iter->value_string);
			}
		}

		if (int(max_tab_warning) > int(max_advanced_tab_warning))
			max_advanced_tab_warning = max_tab_warning;

		// tab label
		app_highlight_tab_label(lookup_widget("selective_selftest_tab_label"), max_tab_warning, tab_selective_selftest_name);

	} while (false);




	// ------------------------------------------- Advanced tab - Phy log


	do {

		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("phy_log_textview");
		if (!textview)
			break;

		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_phy_log)
				continue;

			// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
			if (iter->generic_name == "sataphy_log") {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text("\nComplete phy log:\n\n" + iter->value_string);
			}
		}

		if (int(max_tab_warning) > int(max_advanced_tab_warning))
			max_advanced_tab_warning = max_tab_warning;

		// tab label
		app_highlight_tab_label(lookup_widget("phy_tab_label"), max_tab_warning, tab_phy_name);

	} while (false);




	// ------------------------------------------- Advanced tab - Directory log


	do {

		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("directory_log_textview");
		if (!textview)
			break;

		StorageProperty::warning_t max_tab_warning = StorageProperty::warning_none;

		for (prop_iterator iter = props.begin(); iter != props.end(); ++iter) {
			if (iter->section != StorageProperty::section_data || iter->subsection != StorageProperty::subsection_directory_log)
				continue;

			// Note: Don't use property description as a tooltip here. It won't be available if there's no property.
			if (iter->generic_name == "directory_log") {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text("\nComplete directory log:\n\n" + iter->value_string);
			}
		}

		if (int(max_tab_warning) > int(max_advanced_tab_warning))
			max_advanced_tab_warning = max_tab_warning;

		// tab label
		app_highlight_tab_label(lookup_widget("directory_tab_label"), max_tab_warning, tab_directory_name);

	} while (false);


	// Advanced tab label
	app_highlight_tab_label(lookup_widget("advanced_tab_label"), max_advanced_tab_warning, tab_advanced_name);

}





void GscInfoWindow::clear_ui_info(bool clear_tests_too)
{
	// Note: We do NOT show/hide the notebook tabs here.
	// fill_ui_with_info() will do it all by itself.

	{
		this->set_title("Device Information - GSmartControl");

		// Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label");
		if (device_name_label)
			device_name_label->set_text("No data available");
	}

	{
		Gtk::Grid* identity_table = lookup_widget<Gtk::Grid*>("identity_table");
		if (identity_table) {
			// manually remove all children. without this visual corruption occurs.
			std::vector<Gtk::Widget*> children = identity_table->get_children();
			for (std::vector<Gtk::Widget*>::iterator iter = children.begin(); iter != children.end(); ++iter) {
				identity_table->remove(*(*iter));
			}
		}

		// tab label
		app_highlight_tab_label(lookup_widget("general_tab_label"), StorageProperty::warning_none, tab_identity_name);
	}

	{
		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("attributes_label_vbox");
		app_set_top_labels(label_vbox, label_list_t());

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("attributes_treeview");
		if (treeview) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("attributes_tab_label"), StorageProperty::warning_none, tab_attributes_name);
	}

	{
		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("statistics_label_vbox");
		app_set_top_labels(label_vbox, label_list_t());

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("statistics_treeview");
		if (treeview) {
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("statistics_tab_label"), StorageProperty::warning_none, tab_statistics_name);
	}

	{
		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("selftest_log_label_vbox");
		app_set_top_labels(label_vbox, label_list_t());

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("selftest_log_treeview");
		if (treeview) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		if (clear_tests_too) {
			Gtk::ComboBox* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");
			if (test_type_combo) {
				test_type_combo->set_sensitive(false);  // true if testing is possible and not active.
				// test_type_combo->clear();  // clear cellrenderers
				if (test_combo_model)
					test_combo_model->clear();
			}

			Gtk::Label* min_duration_label = lookup_widget<Gtk::Label*>("min_duration_label");
			if (min_duration_label)
				min_duration_label->set_text("N/A");  // set on test selection

			Gtk::Button* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button");
			if (test_execute_button)
				test_execute_button->set_sensitive(false);  // true if testing is possible and not active


			Gtk::TextView* test_description_textview = lookup_widget<Gtk::TextView*>("test_description_textview");
			if (test_description_textview && test_description_textview->get_buffer())
				test_description_textview->get_buffer()->set_text("");  // set on test selection



			Gtk::ProgressBar* test_completion_progressbar = lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar");
			if (test_completion_progressbar) {
				test_completion_progressbar->set_text("");  // set when test is run or completed
				test_completion_progressbar->set_sensitive(false);  // set when test is run or completed
				test_completion_progressbar->hide();
			}

			Gtk::Button* test_stop_button = lookup_widget<Gtk::Button*>("test_stop_button");
			if (test_stop_button) {
				test_stop_button->set_sensitive(false);  // true when test is active
				test_stop_button->hide();
			}


			Gtk::Box* test_result_hbox = lookup_widget<Gtk::Box*>("test_result_hbox");
			if (test_result_hbox)
				test_result_hbox->hide();  // hide by default. show when test is completed.
		}

		// tab label
		app_highlight_tab_label(lookup_widget("test_tab_label"), StorageProperty::warning_none, tab_test_name);
	}

	{
		Gtk::Box* label_vbox = lookup_widget<Gtk::Box*>("error_log_label_vbox");
		app_set_top_labels(label_vbox, label_list_t());

		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("error_log_treeview");
		if (treeview) {
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("error_log_textview");
		if (textview) {
			// we re-create the buffer to get rid of all the Marks
			textview->set_buffer(Gtk::TextBuffer::create());
			textview->get_buffer()->set_text("\nNo data available");
		}

		// tab label
		app_highlight_tab_label(lookup_widget("error_log_tab_label"), StorageProperty::warning_none, tab_error_log_name);
	}

	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("temperature_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}

		// tab label
		app_highlight_tab_label(lookup_widget("temperature_log_tab_label"), StorageProperty::warning_none, tab_temperature_name);
	}

	// tab label
	app_highlight_tab_label(lookup_widget("advanced_tab_label"), StorageProperty::warning_none, tab_advanced_name);

	{
		Gtk::TreeView* treeview = lookup_widget<Gtk::TreeView*>("capabilities_treeview");
		if (treeview) {
			// It's better to clear the model rather than unset it. If we unset it, we'll have
			// to deattach the callbacks too. But if we clear it, we have to remember column vars.
// 			Glib::RefPtr<Gtk::ListStore> model = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
// 			if (model)
// 				model->clear();
			treeview->remove_all_columns();
			treeview->unset_model();
		}

		// tab label
		app_highlight_tab_label(lookup_widget("capabilities_tab_label"), StorageProperty::warning_none, tab_capabilities_name);
	}

	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("erc_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}

		// tab label
		app_highlight_tab_label(lookup_widget("erc_tab_label"), StorageProperty::warning_none, tab_erc_name);
	}

	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("selective_selftest_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}

		// tab label
		app_highlight_tab_label(lookup_widget("selective_selftest_tab_label"), StorageProperty::warning_none, tab_selective_selftest_name);
	}

	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("phy_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}

		// tab label
		app_highlight_tab_label(lookup_widget("phy_tab_label"), StorageProperty::warning_none, tab_phy_name);
	}

	{
		Gtk::TextView* textview = lookup_widget<Gtk::TextView*>("directory_log_textview");
		if (textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
			buffer->set_text("\nNo data available");
		}

		// tab label
		app_highlight_tab_label(lookup_widget("directory_tab_label"), StorageProperty::warning_none, tab_directory_name);
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
	Gtk::Notebook* book = lookup_widget<Gtk::Notebook*>("main_notebook");
	if (book)
		book->set_current_page(3);  // the Tests tab
}



bool GscInfoWindow::on_delete_event_before(GdkEventAny* e)
{
	if (drive && drive->get_test_is_active()) {  // disallow close if test is active.
		gui_show_warn_dialog("Please wait until all tests are finished.", this);
		return true;  // handled
	}
	destroy(this);  // deletes this object and nullifies instance
	return true;  // event handled, don't call default virtual handler
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

	win->set_text("Smartctl Output", output, true, true);

	std::string filename = drive->get_save_filename();
	if (!filename.empty())
		win->set_save_filename(filename);

	win->show();
}



void GscInfoWindow::on_save_info_button_clicked()
{
	static std::string last_dir;
	if (last_dir.empty()) {
		rconfig::get_data("gui/drive_data_open_save_dir", last_dir);
	}
	int result = 0;

	std::string filename = drive->get_save_filename();

	Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
	specific_filter->set_name("Text Files");
	specific_filter->add_pattern("*.txt");

	Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
	all_filter->set_name("All Files");
	all_filter->add_pattern("*");

#if GTK_CHECK_VERSION(3, 20, 0)
	hz::scoped_ptr<GtkFileChooserNative> dialog(gtk_file_chooser_native_new(
			"Save Data As...", this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, NULL, NULL), g_object_unref);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), true);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

	if (!last_dir.empty())
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());

	if (!filename.empty())
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog.get()), filename.c_str());

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
	Gtk::FileChooserDialog dialog(*this, "Save Data As...",
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
			last_dir = hz::path_get_dirname(file);
#else
			file = dialog.get_filename();  // in fs encoding
			last_dir = dialog.get_current_folder();  // save for the future
#endif
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

			if (file.rfind(".txt") != (file.size() - std::strlen(".txt"))) {
				file += ".txt";
			}

			hz::File f(file);
			std::string data = this->drive->get_full_output();
			if (data.empty()) {
				data = this->drive->get_info_output();
			}
			if (!f.put_contents(data)) {  // this will send to debug_ too.
				gui_show_error_dialog("Cannot save SMART data to file", f.get_error_utf8(), this);
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
	on_delete_event_before(0);
}




void GscInfoWindow::on_test_type_combo_changed()
{
	Gtk::ComboBox* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");
	if (!test_type_combo)
		return;

	Gtk::TreeRow row = *(test_type_combo->get_active());
	if (row) {
		SelfTestPtr test = row[test_combo_col_self_test];

		//debug_out_error("app", test->get_min_duration_seconds() << "\n");
		Gtk::Label* min_duration_label = lookup_widget<Gtk::Label*>("min_duration_label");
		if (min_duration_label) {
			int64_t duration = test->get_min_duration_seconds();
			min_duration_label->set_text(duration == -1 ? "N/A"
					: (duration == 0 ? "Unknown" : hz::format_time_length(duration)));
		}

		Gtk::TextView* test_description_textview = lookup_widget<Gtk::TextView*>("test_description_textview");
		if (test_description_textview && test_description_textview->get_buffer())
			test_description_textview->get_buffer()->set_text(row[test_combo_col_description]);
	}
}





// Note: Another loop like this may run inside it for another drive.
gboolean GscInfoWindow::test_idle_callback(void* data)
{
	GscInfoWindow* self = static_cast<GscInfoWindow*>(data);
	DBG_ASSERT(self);

	if (!self->current_test)  // shouldn't happen
		return false;  // stop

	Gtk::ProgressBar* test_completion_progressbar =
			self->lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar");


	bool active = true;

	do {  // goto
		if (!self->current_test->is_active()) {  // check status
			active = false;
			break;
		}

		int8_t rem_percent = self->current_test->get_remaining_percent();
		std::string rem_percent_str = (rem_percent == -1 ? "Unknown" : hz::number_to_string(100 - rem_percent));

		int64_t poll_in = self->current_test->get_poll_in_seconds();  // sec


		// One update() is performed by start(), so do the timeout first.

		// Wait until next poll (up to several minutes). Meanwhile, interpolate
		// the remaining time, update the progressbar, etc...
		if (self->test_timer_poll.elapsed() < static_cast<double>(poll_in)) {  // elapsed() is seconds in double.

			// Update progress bar right after poll, plus every 5 seconds.
			if (self->test_force_bar_update || self->test_timer_bar.elapsed() >= 5.) {

				int64_t rem_seconds = self->current_test->get_remaining_seconds();

				if (test_completion_progressbar) {
					std::string rem_seconds_str = (rem_seconds == -1 ? "Unknown" : hz::format_time_length(rem_seconds));

					Glib::ustring bar_str;

					if (self->test_error_msg.empty()) {
						bar_str = hz::string_sprintf("Test completion: %s%%; ETA: %s",
								rem_percent_str.c_str(), rem_seconds_str.c_str());
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

			SmartctlExecutorGuiRefPtr ex(new SmartctlExecutorGui());
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
		return true;  // continue the idle callback
	}


	// Test is finished, clean up

	self->test_timer_poll.stop();  // just in case
	self->test_timer_bar.stop();  // just in case

	StorageSelftestEntry::status_t status = self->current_test->get_status();

	bool aborted = false;
	StorageSelftestEntry::status_severity_t severity = StorageSelftestEntry::severity_none;
	std::string result_msg;

	if (!self->test_error_msg.empty()) {
		aborted = true;
		severity = StorageSelftestEntry::severity_error;
		result_msg = "<b>Test aborted: </b>" + self->test_error_msg;

	} else {
		severity = StorageSelftestEntry::get_status_severity(status);
		if (status == StorageSelftestEntry::status_aborted_by_host) {
			aborted = true;
			result_msg = "<b>Test was manually aborted.</b>";  // it's a severity_none message

		} else {
			result_msg = "<b>Test result: </b>" + StorageSelftestEntry::get_status_name(status) + ".";

			// It may not reach 100% somehow, so do it manually.
			if (test_completion_progressbar)
				test_completion_progressbar->set_fraction(1.);  // yes, we're at 100% anyway (at least logically).
		}
	}

	if (severity != StorageSelftestEntry::severity_none) {
		result_msg += "\nCheck the Self-Test Log for more information.";
	}


	Gtk::ComboBox* test_type_combo = self->lookup_widget<Gtk::ComboBox*>("test_type_combo");
	if (test_type_combo)
		test_type_combo->set_sensitive(true);

	Gtk::Button* test_execute_button = self->lookup_widget<Gtk::Button*>("test_execute_button");
	if (test_execute_button)
		test_execute_button->set_sensitive(true);

	if (test_completion_progressbar)
		test_completion_progressbar->set_text(aborted ? "Test aborted" : "Test completed");

	Gtk::Button* test_stop_button = self->lookup_widget<Gtk::Button*>("test_stop_button");
	if (test_stop_button)
		test_stop_button->set_sensitive(false);

	Gtk::StockID stock_id = Gtk::Stock::DIALOG_ERROR;
	if (severity == StorageSelftestEntry::severity_none) {
		stock_id = Gtk::Stock::DIALOG_INFO;
	} else if (severity == StorageSelftestEntry::severity_warn) {
		stock_id = Gtk::Stock::DIALOG_WARNING;
	}

	Gtk::Image* test_result_image = self->lookup_widget<Gtk::Image*>("test_result_image");
	// we use large icon size here because the icons we use are from dialogs.
	// unfortunately, there are no non-dialog icons of this sort.
	if (test_result_image)
		test_result_image->set(stock_id, Gtk::ICON_SIZE_DND);


	Gtk::Label* test_result_label = self->lookup_widget<Gtk::Label*>("test_result_label");
	if (test_result_label)
		test_result_label->set_markup(result_msg);

	Gtk::Box* test_result_hbox = self->lookup_widget<Gtk::Box*>("test_result_hbox");
	if (test_result_hbox)
		test_result_hbox->show();


	self->refresh_info(false);  // don't clear the tests tab

	return false;  // stop idle callback
}





void GscInfoWindow::on_test_execute_button_clicked()
{
	Gtk::ComboBox* test_type_combo = lookup_widget<Gtk::ComboBox*>("test_type_combo");
	if (!test_type_combo)
		return;

	Gtk::TreeRow row = *(test_type_combo->get_active());
	if (!row)
		return;

	SelfTestPtr test = row[test_combo_col_self_test];
	if (!test)
		return;

	// hide previous test results from GUI
	Gtk::Box* test_result_hbox = this->lookup_widget<Gtk::Box*>("test_result_hbox");
	if (test_result_hbox)
		test_result_hbox->hide();

	SmartctlExecutorGuiRefPtr ex(new SmartctlExecutorGui());
	ex->create_running_dialog(this);

	std::string error_msg = test->start(ex);  // this runs update() too.
	if (!error_msg.empty()) {
		gui_show_error_dialog("Cannot run " + SelfTest::get_test_name(test->get_test_type()), error_msg, this);
		return;
	}

	current_test = test;


	// Switch GUI to "running test" mode

	if (test_type_combo)
		test_type_combo->set_sensitive(false);

	Gtk::Button* test_execute_button = lookup_widget<Gtk::Button*>("test_execute_button");
	if (test_execute_button)
		test_execute_button->set_sensitive(false);


	Gtk::ProgressBar* test_completion_progressbar = lookup_widget<Gtk::ProgressBar*>("test_completion_progressbar");
	if (test_completion_progressbar) {
		test_completion_progressbar->set_text("");
		test_completion_progressbar->set_sensitive(true);
		test_completion_progressbar->show();
	}

	Gtk::Button* test_stop_button = lookup_widget<Gtk::Button*>("test_stop_button");
	if (test_stop_button) {
		test_stop_button->set_sensitive(true);  // true while test is active
		test_stop_button->show();
	}


	// reset these
	test_error_msg = "";
	test_timer_poll.start();
	test_timer_bar.start();
	test_force_bar_update = true;


	// We don't use idle function here, because it has the following problem:
	// CmdexSync::execute() (which is called on force_stop()) calls g_main_context_pending(),
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

	SmartctlExecutorGuiRefPtr ex(new SmartctlExecutorGui());
	ex->create_running_dialog(this);

	std::string error_msg = current_test->force_stop(ex);
	if (!error_msg.empty()) {
		gui_show_error_dialog("Cannot stop " + SelfTest::get_test_name(current_test->get_test_type()), error_msg, this);
		return;
	}

	// nothing else to do - the cleanup is performed by the idle callback.
}




// Callback attached to StorageDevice.
// We don't refresh automatically (that would make it impossible to do
// several same-drive info window comparisons side by side).
// But we need to look for testing status change, to avoid aborting it.
void GscInfoWindow::on_drive_changed(StorageDevice* pdrive)
{
	if (!drive)
		return;
	bool test_active = drive->get_test_is_active();

	// disable refresh button if test is active or if it's a virtual drive
	Gtk::Button* refresh_info_button = lookup_widget<Gtk::Button*>("refresh_info_button");
	if (refresh_info_button)
		refresh_info_button->set_sensitive(!test_active && !drive->get_is_virtual());

	// disallow close. usually modal dialogs are used for this, but we can't have
	// per-drive modal dialogs.
	Gtk::Button* close_window_button = lookup_widget<Gtk::Button*>("close_window_button");
	if (close_window_button)
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
		for (std::size_t i = 0; i < children.size(); ++i) {
			children[i]->set_sensitive(!selection_empty);
		}
		menu->popup(button_event->button, button_event->time);
		return true;
	}
	return false;
}



void GscInfoWindow::on_treeview_menu_copy_clicked(Gtk::TreeView* treeview)
{
	std::string text;

	guint num_cols = treeview->get_n_columns();
	std::vector<std::string> col_texts;
	for (guint i = 0; i < num_cols; ++i) {
		Gtk::TreeViewColumn* tcol = treeview->get_column(i);
		col_texts.push_back("\"" + hz::string_replace_copy(tcol->get_title(), "\"", "\"\"") + "\"");
	}
	text += hz::string_join(col_texts, ',') + "\n";

	std::vector<Gtk::TreeModel::Path> selection = treeview->get_selection()->get_selected_rows();
	Glib::RefPtr<Gtk::ListStore> list_store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(treeview->get_model());
	for (std::size_t i = 0; i < selection.size(); ++i) {
		std::vector<std::string> cell_texts;
		Gtk::TreeModel::Path path = selection[i];
		Gtk::TreeRow row = *(list_store->get_iter(path));

		for (guint j = 0; j < num_cols; ++j) {  // gather data only from tree columns, not model columns (like tooltips and helper data)
			GType type = list_store->get_column_type(j);
			if (type == G_TYPE_INT) {
				int32_t value = 0;
				row.get_value(j, value);
				cell_texts.push_back(hz::number_to_string(value));
			} else if (type == G_TYPE_STRING) {
				std::string value;
				row.get_value(j, value);
				cell_texts.push_back("\"" + hz::string_replace_copy(value, "\"", "\"\"") + "\"");
			}
		}
		text += hz::string_join(cell_texts, ',') + "\n";
	}

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	if (clipboard) {
		clipboard->set_text(text);
	}
}







/// @}