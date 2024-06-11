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

#include <cstddef>
#include <cstdint>
#include <glibmm.h>
#include <gtkmm.h>
#include <vector>
#include <cmath>  // std::floor
#include <unordered_map>
#include <cairomm/cairomm.h>

#include "applib/warning_level.h"
#include "hz/string_algo.h"  // string_join
#include "hz/debug.h"
#include "hz/data_file.h"  // data_file_find
#include "applib/app_gtkmm_tools.h"
#include "applib/warning_colors.h"

#include "gsc_main_window.h"
#include "rconfig/rconfig.h"
#include "build_config.h"

#include "gsc_main_window_iconview.h"




std::string GscMainWindowIconView::get_message_string(Message type)
{
	static const std::unordered_map<Message, std::string> m {
			{Message::None,          _("[error - invalid message]")},
			{Message::ScanDisabled,  _("Automatic scanning is disabled.\nPress Ctrl+R to scan manually.")},
			{Message::Scanning,      _("Scanning system, please wait...")},
			{Message::NoDrivesFound, _("No drives found.")},
			{Message::NoSmartctl,    _("Please specify the correct smartctl binary in\nPreferences and press Ctrl-R to re-scan.")},
			{Message::PleaseRescan,  _("Preferences changed.\nPress Ctrl-R to re-scan.")},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



GscMainWindowIconView::GscMainWindowIconView(BaseObjectType* gtkcobj, [[maybe_unused]] const Glib::RefPtr<Gtk::Builder>& ref_ui)
		: Gtk::IconView(gtkcobj)
{
	columns_.add(col_name_);  // we can use the col_name variable by value after this.
	this->set_markup_column(col_name_);

	columns_.add(col_description_);

	columns_.add(col_pixbuf_);

	// For high quality rendering with GDK_SCALE=2
	this->pack_start(cell_renderer_pixbuf_, false);
	this->set_cell_data_func(cell_renderer_pixbuf_,
			sigc::mem_fun(this, &GscMainWindowIconView::on_cell_data_render));

	columns_.add(col_drive_ptr_);

	columns_.add(col_populated_);

	// create a Tree Model
	ref_list_model_ = Gtk::ListStore::create(columns_);
// 			ref_list_model->set_sort_column(col_name, Gtk::SORT_ASCENDING);
	this->set_model(ref_list_model_);

	// we add it here because list model must be created already
	set_tooltip_column(col_description_.index());

	this->load_icon_pixbufs();

	this->signal_item_activated().connect(sigc::mem_fun(*this,
			&GscMainWindowIconView::on_iconview_item_activated) );

	this->signal_selection_changed().connect(sigc::mem_fun(*this,
			&GscMainWindowIconView::on_iconview_selection_changed) );

	this->signal_button_press_event().connect(sigc::mem_fun(*this,
			&GscMainWindowIconView::on_iconview_button_press_event) );
}



void GscMainWindowIconView::set_main_window(GscMainWindow* w)
{
	main_window_ = w;
}



void GscMainWindowIconView::set_empty_view_message(Message message)
{
	empty_view_message_ = message;
}



int GscMainWindowIconView::get_num_icons() const
{
	return num_icons_;
}



bool GscMainWindowIconView::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	if (in_destruction()) {
		return true;
	}
	if (empty_view_message_ != Message::None && this->num_icons_ == 0) {  // no icons
		Glib::RefPtr<Pango::Layout> layout = this->create_pango_layout("");
		layout->set_alignment(Pango::ALIGN_CENTER);
		layout->set_markup(get_message_string(empty_view_message_));

		int layout_w = 0, layout_h = 0;
		layout->get_pixel_size(layout_w, layout_h);

		const int pos_x = (get_allocation().get_width() - layout_w) / 2;
		const int pos_y = (get_allocation().get_height() - layout_h) / 2;
		cr->move_to(pos_x, pos_y);

		layout->show_in_cairo_context(cr);

		return true;
	}

	return Gtk::IconView::on_draw(cr);
}



void GscMainWindowIconView::on_cell_data_render(const Gtk::TreeModel::const_iterator& iter)
{
	Gtk::TreeRow row = *iter;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf = row[col_pixbuf_];
	if (!pixbuf) {
		return;
	}

	// Gtkmm property_surface() doesn't work, so use plain C.
	// https://bugzilla.gnome.org/show_bug.cgi?id=788513
	// Also, Gtkmm doesn't have gdk_cairo_surface_create_from_pixbuf() wrapper.
	// https://bugzilla.gnome.org/show_bug.cgi?id=788533

// 			Cairo::Format format = Cairo::FORMAT_ARGB32;
// 			if (pixbuf->get_n_channels() == 3) {
// 				format = Cairo::FORMAT_RGB24;
// 			}
// 			Cairo::RefPtr<Cairo::Surface> surface = get_window()->create_similar_image_surface(
// 					format, pixbuf->get_width(), pixbuf->get_height(), get_scale_factor());
// 			cell_renderer_pixbuf.property_surface().set_value(surface);

	// gdk_cairo_surface_create_from_pixbuf() (and create_similar_image_surface()) from gtk 3.10.
	cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(pixbuf->gobj(), get_scale_factor(), get_window()->gobj());
	g_object_set(G_OBJECT(cell_renderer_pixbuf_.gobj()), "surface", surface, nullptr);
	cairo_surface_destroy(surface);
}



void GscMainWindowIconView::add_entry(StorageDevicePtr drive, bool scroll_to_it)
{
	if (!drive)
		return;

	Gtk::TreeModel::Row row = *(ref_list_model_->append());
	row[col_drive_ptr_] = drive;

	this->decorate_entry(row);

	row[col_populated_] = true;  // triggers rendering

	drive->signal_changed().connect(sigc::mem_fun(this, &GscMainWindowIconView::on_drive_changed));

	if (scroll_to_it) {
		const Gtk::TreeModel::Path tpath(row);
		// scroll_to_path() and set/get_cursor() are since gtkmm 2.8.

		this->scroll_to_path(tpath, true, 0.5, 0.5);
		// select it (keyboard & selection)
		Gtk::CellRenderer* cell = nullptr;
		if (this->get_cursor(cell) && cell) {
			this->set_cursor(tpath, *cell, false);
		}
		this->select_path(tpath);  // highlight it
	}

	++num_icons_;
}



void GscMainWindowIconView::decorate_entry(const Gtk::TreePath& model_path)
{
	if (model_path.empty())
		return;

	Gtk::TreeModel::Row row = *(ref_list_model_->get_iter(model_path));
	this->decorate_entry(row);
}



void GscMainWindowIconView::decorate_entry(Gtk::TreeModel::Row& row)
{
	StorageDevicePtr drive = row[col_drive_ptr_];
	if (!drive) {
		return;
	}

	// it needs this space to be symmetric (why?);
	std::string name;  // = "<big>" + drive->get_device_with_type() + " </big>\n";
	Glib::ustring drive_letters = Glib::Markup::escape_text(drive->format_drive_letters(false));
	if (drive_letters.empty()) {
		drive_letters = C_("media", "not mounted");
	}
	Glib::ustring drive_letters_with_volname = Glib::Markup::escape_text(drive->format_drive_letters(true));
	if (drive_letters_with_volname.empty()) {
		drive_letters_with_volname = C_("media", "not mounted");
	}

	// note: if this wraps, it becomes left-aligned in gtk <= 2.10.
	name += (drive->get_model_name().empty() ? Glib::ustring("Unknown model") : Glib::Markup::escape_text(drive->get_model_name()));
	if (rconfig::get_data<bool>("gui/icons_show_device_name")) {
		if (!drive->get_is_virtual()) {
			const std::string dev = Glib::Markup::escape_text(drive->get_device_with_type());
			if constexpr(BuildEnv::is_kernel_family_windows()) {
				name += "\n" + Glib::ustring::compose(_("%1 (%2)"), dev, drive_letters);
			} else {
				name += "\n" + dev;
			}
		} else if (!drive->get_virtual_filename().empty()) {
			name += "\n" + Glib::Markup::escape_text(drive->get_virtual_filename());
		}
	}
	if (rconfig::get_data<bool>("gui/icons_show_serial_number") && !drive->get_serial_number().empty()) {
		name += "\n" + Glib::Markup::escape_text(drive->get_serial_number());
	}
	StorageProperty scan_time_prop;
	if (drive->get_is_virtual()) {
		scan_time_prop = drive->get_property_repository().lookup_property("local_time/asctime");
		if (!scan_time_prop.empty() && !scan_time_prop.get_value<std::string>().empty()) {
			name += "\n" + Glib::Markup::escape_text(scan_time_prop.get_value<std::string>());
		}
	}

	std::vector<std::string> tooltip_strs;

	if (drive->get_is_virtual()) {
		const std::string vfile = drive->get_virtual_filename();
		tooltip_strs.push_back(Glib::ustring::compose(_("Loaded from: %1"), (vfile.empty() ? (Glib::ustring("[") + C_("name", "empty") + "]") : Glib::Markup::escape_text(vfile))));
		if (!scan_time_prop.empty() && !scan_time_prop.get_value<std::string>().empty()) {
			tooltip_strs.push_back(Glib::ustring::compose(_("Scanned on: "), Glib::Markup::escape_text(scan_time_prop.get_value<std::string>())));
		}
	} else {
		tooltip_strs.push_back(Glib::ustring::compose(_("Device: %1"), "<b>" + Glib::Markup::escape_text(drive->get_device_with_type()) + "</b>"));
	}

	if constexpr(BuildEnv::is_kernel_family_windows()) {
		tooltip_strs.push_back(Glib::ustring::compose(_("Drive letters: %1"), "<b>" + drive_letters_with_volname + "</b>"));
	}

	if (!drive->get_serial_number().empty()) {
		tooltip_strs.push_back(Glib::ustring::compose(_("Serial number: %1"), "<b>" + Glib::Markup::escape_text(drive->get_serial_number()) + "</b>"));
	}
	tooltip_strs.push_back(Glib::ustring::compose(_("SMART status: %1"),
			"<b>" + Glib::Markup::escape_text(StorageDevice::get_status_displayable_name(drive->get_smart_status())) + "</b>"));

	std::string tooltip_str = hz::string_join(tooltip_strs, '\n');


	Glib::RefPtr<Gdk::Pixbuf> icon;
	if (icon_pixbufs_.contains(drive->get_detected_type())) {
		icon = icon_pixbufs_[drive->get_detected_type()];
	} else {
		icon = default_icon_;
	}

	const StorageProperty health_prop = drive->get_health_property();
	if (health_prop.warning_level != WarningLevel::None && health_prop.generic_name == "smart_status/passed") {
		if (icon) {
			icon = icon->copy();  // work on a copy
			if (icon->get_colorspace() == Gdk::COLORSPACE_RGB && icon->get_bits_per_sample() == 8) {
				const std::ptrdiff_t n_channels = icon->get_n_channels();
				const std::ptrdiff_t icon_width = icon->get_width();
				const std::ptrdiff_t icon_height = icon->get_height();
				const std::ptrdiff_t rowstride = icon->get_rowstride();
				guint8* pixels = icon->get_pixels();

				for (std::ptrdiff_t y = 0; y < icon_height; ++y) {
					for (std::ptrdiff_t x = 0; x < icon_width; ++x) {
						guint8* p = pixels + y * rowstride + x * n_channels;
						auto avg = static_cast<uint8_t>(std::floor((p[0] * 0.30) + (p[1] * 0.59) + (p[2] * 0.11) + 0.001 + 0.5));
						p[0] = avg;  // R
						p[1] = 0;  // G
						p[2] = 0;  // B
					}
				}
			}
		}

		tooltip_str += "\n\n" + storage_property_get_warning_reason(health_prop)
				+ "\n\n" + _("View details for more information.");
	}


	// we use all these if-s because changing the data (without actually changing it)
	// sometimes leads to screwed up icons in iconview (blame gtk).

	if (row.get_value(col_name_) != name) {
		row[col_name_] = name;  // markup
	}
	if (row.get_value(col_description_) != tooltip_str) {
		row[col_description_] = tooltip_str;  // markup
	}

	if (row.get_value(col_pixbuf_) != icon) {
		row[col_pixbuf_] = icon;
	}
}



void GscMainWindowIconView::remove_entry(const Gtk::TreePath& model_path)
{
	const Gtk::TreeModel::Row row = *(ref_list_model_->get_iter(model_path));
	ref_list_model_->erase(row);
}



void GscMainWindowIconView::remove_selected_drive()
{
	const auto& selected_items = this->get_selected_items();
	if (!selected_items.empty()) {
		const Gtk::TreePath model_path = *(selected_items.begin());
		this->remove_entry(model_path);
	}
}



void GscMainWindowIconView::clear_all()
{
	num_icons_ = 0;
	ref_list_model_->clear();

	// this is needed to update the label from "disabled" to "scanning"
	if (this->get_realized()) {
		const Gdk::Rectangle rect = this->get_allocation();
		Glib::RefPtr<Gdk::Window> win = this->get_window();
		win->invalidate_rect(rect, true);  // force expose event
		win->process_updates(false);  // update immediately
	}
}



StorageDevicePtr GscMainWindowIconView::get_selected_drive()
{
	StorageDevicePtr drive;
	const auto& selected_items = this->get_selected_items();
	if (!selected_items.empty()) {
		const Gtk::TreePath model_path = *(selected_items.begin());
		const Gtk::TreeModel::Row row = *(ref_list_model_->get_iter(model_path));
		drive = row[col_drive_ptr_];
	}
	return drive;
}



Gtk::TreePath GscMainWindowIconView::get_path_by_drive(StorageDevice* drive)
{
	const Gtk::TreeNodeChildren children = ref_list_model_->children();
	for (const auto& row : children) {
		// convert iter to row (iter is row's base, but can we cast it?)
		if (drive == row.get_value(col_drive_ptr_).get())
			return ref_list_model_->get_path(row);
	}
	return {};  // check with .empty()
}



void GscMainWindowIconView::update_menu_actions()
{
	// if there's nothing selected, disable items from "Drives" menu
	if (this->get_selected_items().empty()) {
		main_window_->set_drive_menu_status(nullptr);

	} else {  // enable drives menu, set proper smart toggles
		const Gtk::TreePath model_path = *(this->get_selected_items().begin());
		const Gtk::TreeModel::Row row = *(ref_list_model_->get_iter(model_path));
		if (!row[col_populated_]) {  // protect against using incomplete model entry
			return;
		}

		StorageDevicePtr drive = row[col_drive_ptr_];
		main_window_->set_drive_menu_status(drive);
	}
}


void GscMainWindowIconView::on_iconview_item_activated(const Gtk::TreePath& model_path)
{
	debug_out_info("app", DBG_FUNC << "\n");
	if (!main_window_)
		return;

	const Gtk::TreeModel::Row row = *(ref_list_model_->get_iter(model_path));
	if (!row[col_populated_]) {  // protect against using incomplete model entry
		return;
	}

	StorageDevicePtr drive = row[col_drive_ptr_];
	main_window_->show_device_info_window(drive);
}



void GscMainWindowIconView::on_iconview_selection_changed()
{
	// Must do it here - if done during menu activation, the actions won't work
	// properly before that.
	this->update_menu_actions();

	main_window_->update_status_widgets();  // status area, etc.
}



bool GscMainWindowIconView::on_iconview_button_press_event(GdkEventButton* event_button)
{
	// select and show popup menu on right-click
	if (event_button->type == GDK_BUTTON_PRESS && event_button->button == 3) {
		StorageDevicePtr drive;  // clicked drive (if any)

		// don't use get_item_at_pos() - it's not available in gtkmm < 2.8.
		Gtk::TreePath tpath = this->get_path_at_pos(static_cast<int>(event_button->x),
				static_cast<int>(event_button->y));
		// if (this->get_item_at_pos(static_cast<int>(event_button->x), static_cast<int>(event_button->y), model_path)) {

		if (tpath.gobj() && !tpath.empty()) {  // without gobj() check gtkmm 2.6 (but not 2.12) prints lots of errors
			// move keyboard focus to the icon (just as left-click does)

			Gtk::CellRenderer* cell = nullptr;
			if (this->get_cursor(cell) && cell) {
				// gtkmm's set_cursor() is undefined (but declared) in 2.8, so use gtk variant.
				gtk_icon_view_set_cursor(GTK_ICON_VIEW(this->gobj()), tpath.gobj(), cell->gobj(), FALSE);
			}

			// select the icon
			this->select_path(tpath);

			const Gtk::TreeModel::Row row = *(ref_list_model_->get_iter(tpath));
			drive = row[col_drive_ptr_];

		} else {
			this->unselect_all();  // unselect on empty area right-click
		}

		Gtk::Menu* menu = main_window_->get_popup_menu(drive);
		if (menu)
			menu->popup(event_button->button, event_button->time);

		return true;  // stop handling
	}

	// left click and everything else - continue handling.
	// left click selects the icon by default, and allows "activated" signal on double-click.
	return false;
}



void GscMainWindowIconView::on_drive_changed(StorageDevice* drive)
{
	const Gtk::TreePath model_path = this->get_path_by_drive(drive);
	this->decorate_entry(model_path);
	this->update_menu_actions();
	main_window_->update_status_widgets();
}



void GscMainWindowIconView::load_icon_pixbufs()
{
	Glib::RefPtr<Gtk::IconTheme> default_icon_theme;
	try {
		default_icon_theme = Gtk::IconTheme::get_default();
	} catch (...) {
		// nothing
	}

	default_icon_ = load_icon_pixbuf(default_icon_theme, "drive-harddisk", "icon_harddisk.png");

	for (auto type : StorageDeviceDetectedTypeExt::get_all_values()) {
		Glib::RefPtr<Gdk::Pixbuf> type_icon;
		switch(type) {
			case StorageDeviceDetectedType::Unknown:
			case StorageDeviceDetectedType::NeedsExplicitType:
				break;
			case StorageDeviceDetectedType::AtaAny:
			case StorageDeviceDetectedType::AtaHdd:
				type_icon = load_icon_pixbuf(default_icon_theme, "drive-harddisk", "icon_harddisk.png");
				break;
			case StorageDeviceDetectedType::AtaSsd:
			case StorageDeviceDetectedType::Nvme:
				// Not in XDG, but available in some icon themes
				type_icon = load_icon_pixbuf(default_icon_theme, "drive-harddisk-solidstate", "");
				break;
			case StorageDeviceDetectedType::BasicScsi:
				// Most likely to be a flash drive
				type_icon = load_icon_pixbuf(default_icon_theme, "drive-removable-media", "");
				break;
			case StorageDeviceDetectedType::CdDvd:
				type_icon = load_icon_pixbuf(default_icon_theme, "drive-optical", "icon_optical.png");
				break;
			case StorageDeviceDetectedType::UnsupportedRaid:
				// Not in XDG, but available in some icon themes
				type_icon = load_icon_pixbuf(default_icon_theme, "drive-multidisk", "");
				break;
		}
		if (!type_icon) {
			type_icon = default_icon_;
		}
		icon_pixbufs_[type] = type_icon;
	}
}



Glib::RefPtr<Gdk::Pixbuf> GscMainWindowIconView::load_icon_pixbuf(Glib::RefPtr<Gtk::IconTheme> default_icon_theme,
		const std::string& xdg_icon_name, const std::string& bundled_icon_filename)
{
	Glib::RefPtr<Gdk::Pixbuf> icon;

	// Try XDG version first
	try {
		if (default_icon_theme && !xdg_icon_name.empty()) {
			icon = default_icon_theme->load_icon(xdg_icon_name, icon_size_, get_scale_factor(), Gtk::IconLookupFlags(0));
		}
	} catch (...) { }  // ignore exceptions

	if (!icon && !bundled_icon_filename.empty()) {
		// Still no luck, use bundled ones.
		if (auto icon_file = hz::data_file_find("icons", bundled_icon_filename); !icon_file.empty()) {
			try {
				icon = Gdk::Pixbuf::create_from_file(hz::fs_path_to_string(icon_file));
			} catch (...) { }  // ignore exceptions
		}
	}

	return icon;
}




/// @}
