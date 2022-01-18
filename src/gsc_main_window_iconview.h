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

#ifndef GSC_MAIN_WINDOW_ICONVIEW_H
#define GSC_MAIN_WINDOW_ICONVIEW_H

#include "local_glibmm.h"
#include <gtkmm.h>
#include <vector>
#include <cmath>  // std::floor
#include <unordered_map>
#include <cairomm/cairomm.h>

#include "hz/string_algo.h"  // string_join
#include "hz/debug.h"
#include "hz/data_file.h"  // data_file_find
#include "applib/app_gtkmm_tools.h"
#include "applib/warning_colors.h"

#include "gsc_main_window.h"
#include "rconfig/rconfig.h"
#include "build_config.h"



/// The icon view of the main window (shows a drive list).
/// Note: The IconView must have a fixed icon width set (e.g. in .ui file).
/// Otherwise, it doesn't re-compute it when clearing and adding new icons.
class GscMainWindowIconView : public Gtk::IconView {
	public:

		/// Message type to show
		enum class Message {
			none,  ///< No message
			scan_disabled,  ///< Scanning is disabled
			scanning,  ///< Scanning drives...
			no_drives_found,  ///< No drives found
			no_smartctl,  ///< No smartctl installed
			please_rescan,  ///< Re-scan to see the drives
		};


		/// Get message string
		static std::string get_message_string(Message type)
		{
			static const std::unordered_map<Message, std::string> m {
					{Message::none, _("[error - invalid message]")},
					{Message::scan_disabled, _("Automatic scanning is disabled.\nPress Ctrl+R to scan manually.")},
					{Message::scanning, _("Scanning system, please wait...")},
					{Message::no_drives_found, _("No drives found.")},
					{Message::no_smartctl, _("Please specify the correct smartctl binary in\nPreferences and press Ctrl-R to re-scan.")},
					{Message::please_rescan, _("Preferences changed.\nPress Ctrl-R to re-scan.")},
			};
			if (auto iter = m.find(type); iter != m.end()) {
				return iter->second;
			}
			return "[internal_error]";
		}


		/// Constructor, GtkBuilder needs this.
		[[maybe_unused]] GscMainWindowIconView(BaseObjectType* gtkcobj, [[maybe_unused]] const Glib::RefPtr<Gtk::Builder>& ref_ui)
				: Gtk::IconView(gtkcobj)
		{
			columns.add(col_name);  // we can use the col_name variable by value after this.
			this->set_markup_column(col_name);

			columns.add(col_description);

			columns.add(col_pixbuf);

			// For high quality rendering with GDK_SCALE=2
			this->pack_start(cell_renderer_pixbuf, false);
			this->set_cell_data_func(cell_renderer_pixbuf,
					sigc::mem_fun(this, &GscMainWindowIconView::on_cell_data_render));

			columns.add(col_drive_ptr);

			columns.add(col_populated);

			// create a Tree Model
			ref_list_model = Gtk::ListStore::create(columns);
// 			ref_list_model->set_sort_column(col_name, Gtk::SORT_ASCENDING);
			this->set_model(ref_list_model);

			// we add it here because list model must be created already
			set_tooltip_column(col_description.index());


			// icons
			Glib::RefPtr<Gtk::IconTheme> default_icon_theme;
			try {
				default_icon_theme = Gtk::IconTheme::get_default();
			} catch (...) {
				// nothing
			}

			// Adwaita's drive-harddisk icons are really small at 48, so 64 is better.
			// Plus, it scales well to 128 and 256 (if using GDK_SCALE).
			const int icon_size = 64;

			// Try XDG version first
			try {
				hd_icon = default_icon_theme->load_icon("drive-harddisk", icon_size, get_scale_factor(), Gtk::IconLookupFlags(0));
			} catch (...) { }  // ignore exceptions

			if (!hd_icon) {
				// Still no luck, use bundled ones.
				auto icon_file = hz::data_file_find("icons", "icon_hdd.png");
				if (!icon_file.empty()) {
					try {
						hd_icon = Gdk::Pixbuf::create_from_file(icon_file.u8string());
					} catch (...) { }  // ignore exceptions
				}
			}

			// Try XDG version first
			try {
				cddvd_icon = default_icon_theme->load_icon("media-optical", icon_size, get_scale_factor(), Gtk::IconLookupFlags(0));
			} catch (...) { }  // ignore exceptions

			if (!cddvd_icon) {
				// Still no luck, use bundled ones.
				auto icon_file = hz::data_file_find("icons", "icon_cddvd.png");
				if (!icon_file.empty()) {
					try {
						cddvd_icon = Gdk::Pixbuf::create_from_file(icon_file.u8string());
					} catch (...) { }  // ignore exceptions
				}
			}

			this->signal_item_activated().connect(sigc::mem_fun(*this,
					&GscMainWindowIconView::on_iconview_item_activated) );

			this->signal_selection_changed().connect(sigc::mem_fun(*this,
					&GscMainWindowIconView::on_iconview_selection_changed) );

			this->signal_button_press_event().connect(sigc::mem_fun(*this,
					&GscMainWindowIconView::on_iconview_button_press_event) );
		}


		/// Set the parent window
		void set_main_window(GscMainWindow* w)
		{
			main_window = w;
		}


		/// Set the message type to display when there are no icons to show
		void set_empty_view_message(Message message)
		{
			empty_view_message = message;
		}


		/// Get the number of icons currently displayed
		int get_num_icons() const
		{
			return num_icons;
		}


		// Overridden from Gtk::Widget
		bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override
		{
			if (in_destruction()) {
				return true;
			}
			if (empty_view_message != Message::none && this->num_icons == 0) {  // no icons
				Glib::RefPtr<Pango::Layout> layout = this->create_pango_layout("");
				layout->set_alignment(Pango::ALIGN_CENTER);
				layout->set_markup(get_message_string(empty_view_message));

				int layout_w = 0, layout_h = 0;
				layout->get_pixel_size(layout_w, layout_h);

				int pos_x = (get_allocation().get_width() - layout_w) / 2;
				int pos_y = (get_allocation().get_height() - layout_h) / 2;
				cr->move_to(pos_x, pos_y);

				layout->show_in_cairo_context(cr);

				return true;
			}

			return Gtk::IconView::on_draw(cr);
		}



		/// Cell data renderer (needed for high quality icons in GDK_SCALE=2).
		/// We have to use Cairo surfaces, because pixbufs are scaled by GtkIconView.
		void on_cell_data_render(const Gtk::TreeModel::const_iterator& iter)
		{
			Gtk::TreeRow row = *iter;
			Glib::RefPtr<Gdk::Pixbuf> pixbuf = row[col_pixbuf];

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
			g_object_set(G_OBJECT(cell_renderer_pixbuf.gobj()), "surface", surface, nullptr);
			cairo_surface_destroy(surface);
		}


		/// Add a drive entry to the icon view
		void add_entry(StorageDevicePtr drive, bool scroll_to_it = false)
		{
			if (!drive)
				return;

			Gtk::TreeModel::Row row = *(ref_list_model->append());
			row[col_drive_ptr] = drive;

			this->decorate_entry(row);

			row[col_populated] = true;  // triggers rendering

			drive->signal_changed().connect(sigc::mem_fun(this, &GscMainWindowIconView::on_drive_changed));

			if (scroll_to_it) {
				Gtk::TreeModel::Path tpath(row);
				// scroll_to_path() and set/get_cursor() are since gtkmm 2.8.

				this->scroll_to_path(tpath, true, 0.5, 0.5);
				// select it (keyboard & selection)
				Gtk::CellRenderer* cell = nullptr;
				if (this->get_cursor(cell) && cell) {
					this->set_cursor(tpath, *cell, false);
				}
				this->select_path(tpath);  // highlight it
			}

			++num_icons;
		}



		/// Decorate a drive entry (colorize it if it has errors, etc...).
		/// This should be called to update the icon of already refreshed drive.
		void decorate_entry(const Gtk::TreePath& model_path)
		{
			if (model_path.empty())
				return;

			Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
			this->decorate_entry(row);
		}



		/// Decorate a drive entry (colorize it if it has errors, etc...).
		/// This should be called to update the icon of already refreshed drive.
		void decorate_entry(Gtk::TreeModel::Row& row)
		{
			StorageDevicePtr drive = row[col_drive_ptr];
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
					std::string dev = Glib::Markup::escape_text(drive->get_device_with_type());
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
			AtaStorageProperty scan_time_prop;
			if (drive->get_is_virtual()) {
				scan_time_prop = drive->lookup_property("scan_time");
				if (!scan_time_prop.empty() && !scan_time_prop.get_value<std::string>().empty()) {
					name += "\n" + Glib::Markup::escape_text(scan_time_prop.get_value<std::string>());
				}
			}

			std::vector<std::string> tooltip_strs;

			if (drive->get_is_virtual()) {
				std::string vfile = drive->get_virtual_filename();
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
			tooltip_strs.push_back(Glib::ustring::compose(_("Automatic Offline Data Collection status: %1"),
					"<b>" + Glib::Markup::escape_text(StorageDevice::get_status_displayable_name(drive->get_aodc_status())) + "</b>"));

			std::string tooltip_str = hz::string_join(tooltip_strs, '\n');


			Glib::RefPtr<Gdk::Pixbuf> icon;
			switch(drive->get_detected_type()) {
				case StorageDevice::DetectedType::cddvd:
					icon = cddvd_icon;
					break;
				case StorageDevice::DetectedType::unknown:  // standard HD icon
				case StorageDevice::DetectedType::invalid:
				case StorageDevice::DetectedType::raid:  // TODO a separate icon for this
					icon = hd_icon;
					break;
			}

			AtaStorageProperty health_prop = drive->get_health_property();
			if (health_prop.warning != WarningLevel::none && health_prop.generic_name == "overall_health") {
				if (icon) {
					icon = icon->copy();  // work on a copy
					if (icon->get_colorspace() == Gdk::COLORSPACE_RGB && icon->get_bits_per_sample() == 8) {
						std::ptrdiff_t n_channels = icon->get_n_channels();
						std::ptrdiff_t icon_width = icon->get_width();
						std::ptrdiff_t icon_height = icon->get_height();
						std::ptrdiff_t rowstride = icon->get_rowstride();
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

			if (row.get_value(col_name) != name)
				row[col_name] = name;  // markup
			if (row.get_value(col_description) != tooltip_str)
				row[col_description] = tooltip_str;  // markup

			if (row.get_value(col_pixbuf) != icon)
				row[col_pixbuf] = icon;

		}



		/// Remove drive entry
		void remove_entry(const Gtk::TreePath& model_path)
		{
			Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
			ref_list_model->erase(row);
		}



		/// Remove selected drive entry
		void remove_selected_drive()
		{
			const auto& selected_items = this->get_selected_items();
			if (!selected_items.empty()) {
				Gtk::TreePath model_path = *(selected_items.begin());
				this->remove_entry(model_path);
			}
		}



		/// Remove all entries
		void clear_all()
		{
			num_icons = 0;
			ref_list_model->clear();

			// this is needed to update the label from "disabled" to "scanning"
			if (this->get_realized()) {
				Gdk::Rectangle rect = this->get_allocation();
				Glib::RefPtr<Gdk::Window> win = this->get_window();
				win->invalidate_rect(rect, true);  // force expose event
				win->process_updates(false);  // update immediately
			}
		}



		/// Get selected drive
		StorageDevicePtr get_selected_drive()
		{
			StorageDevicePtr drive;
			const auto& selected_items = this->get_selected_items();
			if (!selected_items.empty()) {
				Gtk::TreePath model_path = *(selected_items.begin());
				Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
				drive = row[col_drive_ptr];
			}
			return drive;
		}



		/// Get tree path by a drive
		Gtk::TreePath get_path_by_drive(StorageDevice* drive)
		{
			Gtk::TreeNodeChildren children = ref_list_model->children();
			for (const auto& row : children) {
				// convert iter to row (iter is row's base, but can we cast it?)
				if (drive == row.get_value(col_drive_ptr).get())
					return ref_list_model->get_path(row);
			}
			return {};  // check with .empty()
		}



		/// Update menu actions in the Drives menu
		void update_menu_actions()
		{
			// if there's nothing selected, disable items from "Drives" menu
			if (this->get_selected_items().empty()) {
				main_window->set_drive_menu_status(nullptr);

			} else {  // enable drives menu, set proper smart toggles
				Gtk::TreePath model_path = *(this->get_selected_items().begin());
				Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
				if (!row[col_populated]) {  // protect against using incomplete model entry
					return;
				}

				StorageDevicePtr drive = row[col_drive_ptr];
				main_window->set_drive_menu_status(drive);
			}
		}


		/// Callback
		void on_iconview_item_activated(const Gtk::TreePath& model_path)
		{
			debug_out_info("app", DBG_FUNC << "\n");
			if (!main_window)
				return;

			Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
			if (!row[col_populated]) {  // protect against using incomplete model entry
				return;
			}

			StorageDevicePtr drive = row[col_drive_ptr];
			main_window->show_device_info_window(drive);
		}


		/// Callback
		void on_iconview_selection_changed()
		{
			// Must do it here - if done during menu activation, the actions won't work
			// properly before that.
			this->update_menu_actions();

			main_window->update_status_widgets();  // status area, etc...
		}


		/// Callback
		bool on_iconview_button_press_event(GdkEventButton* event_button)
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

					Gtk::TreeModel::Row row = *(ref_list_model->get_iter(tpath));
					drive = row[col_drive_ptr];

				} else {
					this->unselect_all();  // unselect on empty area right-click
				}

				Gtk::Menu* menu = main_window->get_popup_menu(drive);
				if (menu)
					menu->popup(event_button->button, event_button->time);

				return true;  // stop handling
			}

			// left click and everything else - continue handling.
			// left click selects the icon by default, and allows "activated" signal on double-click.
			return false;
		}


		/// Callback attached to StorageDevice, updates its view.
		void on_drive_changed(StorageDevice* drive)
		{
			Gtk::TreePath model_path = this->get_path_by_drive(drive);
			this->decorate_entry(model_path);
			this->update_menu_actions();
			main_window->update_status_widgets();
		}


	private:

		Gtk::TreeModel::ColumnRecord columns;  ///< Model columns
		Gtk::CellRendererPixbuf cell_renderer_pixbuf;  ///< Cell renderer for icons.

		Gtk::TreeModelColumn<std::string> col_name;  ///< Model column
		Gtk::TreeModelColumn<Glib::ustring> col_description;  ///< Model column
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > col_pixbuf;  ///< Model column
		Gtk::TreeModelColumn<StorageDevicePtr> col_drive_ptr;  ///< Model column
		Gtk::TreeModelColumn<bool> col_populated;  ///< Model column, indicates whether the model entry has been fully populated.

		Glib::RefPtr<Gtk::ListStore> ref_list_model;  ///< The icon view model
		int num_icons = 0;  ///< Track the number of icons, because liststore makes it difficult to count them.

		// available icons
		Glib::RefPtr<Gdk::Pixbuf> hd_icon;  ///< Icon pixbuf
		Glib::RefPtr<Gdk::Pixbuf> cddvd_icon;  ///< Icon pixbuf

		GscMainWindow* main_window = nullptr;  ///< The main window, our parent

		Message empty_view_message = Message::none;  ///< Message type to display when not showing any icons

};








#endif

/// @}
