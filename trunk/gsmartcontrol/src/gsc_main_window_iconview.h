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

#ifndef GSC_MAIN_WINDOW_ICONVIEW_H
#define GSC_MAIN_WINDOW_ICONVIEW_H

#include <gtkmm.h>
#include <vector>
#include <cmath>  // std::floor
#include <cairomm/cairomm.h>

#include "hz/string_algo.h"  // string_join
#include "hz/debug.h"
#include "hz/data_file.h"  // data_file_find
#include "applib/app_gtkmm_utils.h"
#include "applib/storage_property_colors.h"

#include "gsc_main_window.h"
#include "rconfig/rconfig_mini.h"



/// The icon view of the main window (shows a drive list).
/// Note: The IconView must have a fixed icon width set (e.g. in .ui file).
/// Otherwise, it doesn't re-compute it when clearing and adding new icons.
class GscMainWindowIconView : public Gtk::IconView {
	public:

		typedef GscMainWindowIconView self_type;  ///< Self type, needed for CONNECT_VIRTUAL

		/// Message type to show
		enum message_t {
			message_none,  ///< No message
			message_scan_disabled,  ///< Scanning is disabled
			message_scanning,  ///< Scanning drives...
			message_no_drives_found,  ///< No drives found
			message_no_smartctl,  ///< No smartctl installed
			message_please_rescan,  ///< Re-scan to see the drives
		};


		/// Constructor, GtkBuilder needs this.
		GscMainWindowIconView(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
				: Gtk::IconView(gtkcobj), num_icons(0), main_window(0), empty_view_message(message_none)
		{
			columns.add(col_name);  // we can use the col_name variable by value after this.
			this->set_markup_column(col_name);

			columns.add(col_description);

			columns.add(col_pixbuf);

#if GTK_CHECK_VERSION(3, 10, 0)
			// For high quality rendering with GDK_SCALE=2
			this->pack_start(cell_renderer_pixbuf, false);
			this->set_cell_data_func(cell_renderer_pixbuf,
					sigc::mem_fun(this, &GscMainWindowIconView::on_cell_data_render));
#else
			this->set_pixbuf_column(col_pixbuf);
#endif

			columns.add(col_drive_ptr);

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
#if APP_GTKMM_CHECK_VERSION(3, 10, 0)
			try {
				hd_icon = default_icon_theme->load_icon("drive-harddisk", icon_size, get_scale_factor(), Gtk::IconLookupFlags(0));
			} catch (...) { }  // ignore exceptions
#endif
			if (!hd_icon) {
				// Before gtk 3.10 it was called gtk-harddisk.
				try {
					hd_icon = default_icon_theme->load_icon("gtk-harddisk", icon_size, Gtk::IconLookupFlags(0));
				} catch (...) { }  // ignore exceptions
			}
			if (!hd_icon) {
				// Still no luck, use bundled ones.
				std::string icon_file = hz::data_file_find("icon_hdd.png");
				if (!icon_file.empty()) {
					try {
						hd_icon = Gdk::Pixbuf::create_from_file(icon_file);
					} catch (...) { }  // ignore exceptions
				}
			}

			// Try XDG version first
#if APP_GTKMM_CHECK_VERSION(3, 10, 0)
			try {
				cddvd_icon = default_icon_theme->load_icon("media-optical", icon_size, get_scale_factor(), Gtk::IconLookupFlags(0));
			} catch (...) { }  // ignore exceptions
#endif
			if (!cddvd_icon) {
				// Before gtk 3.10 it was called gtk-cdrom.
				try {
					cddvd_icon = default_icon_theme->load_icon("gtk-cdrom", icon_size, Gtk::IconLookupFlags(0));
				} catch (...) { }  // ignore exceptions
			}
			if (!cddvd_icon) {
				// Still no luck, use bundled ones.
				std::string icon_file = hz::data_file_find("icon_cddvd.png");
				if (!icon_file.empty()) {
					try {
						cddvd_icon = Gdk::Pixbuf::create_from_file(icon_file);
					} catch (...) { }  // ignore exceptions
				}
			}

			this->signal_item_activated().connect(sigc::mem_fun(*this,
					&self_type::on_iconview_item_activated) );

			this->signal_selection_changed().connect(sigc::mem_fun(*this,
					&self_type::on_iconview_selection_changed) );

			this->signal_button_press_event().connect(sigc::mem_fun(*this,
					&self_type::on_iconview_button_press_event) );
		}


		/// Set the parent window
		void set_main_window(GscMainWindow* w)
		{
			main_window = w;
		}


		/// Set the message type to display when there are no icons to show
		void set_empty_view_message(message_t message)
		{
			empty_view_message = message;
		}


		/// Get the number of icons currently displayed
		unsigned int get_num_icons() const
		{
			return num_icons;
		}


		// Overridden from Gtk::Widget
		bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
		{
			if (empty_view_message != message_none && this->num_icons == 0) {  // no icons
				std::string msg;
				switch(empty_view_message) {
					case message_scan_disabled: msg = "Automatic scanning is disabled.\nPress Ctrl+R to scan manually."; break;
					case message_scanning: msg = "Scanning system, please wait..."; break;
					case message_no_drives_found: msg = "No drives found."; break;
					case message_no_smartctl: msg = "Please specify the correct smartctl binary in\nPreferences and press Ctrl-R to re-scan."; break;
					case message_please_rescan: msg = "Preferences changed.\nPress Ctrl-R to re-scan."; break;
					default: msg = "[error - invalid message]";
				}

				Glib::RefPtr<Pango::Layout> layout = this->create_pango_layout("");
				layout->set_alignment(Pango::ALIGN_CENTER);
				layout->set_markup(msg);

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



#if GTK_CHECK_VERSION(3, 10, 0)
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
			g_object_set(G_OBJECT(cell_renderer_pixbuf.gobj()), "surface", surface, NULL);
			cairo_surface_destroy(surface);
		}
#endif


		/// Add a drive entry to the icon view
		void add_entry(StorageDeviceRefPtr drive, bool scroll_to_it = false)
		{
			if (!drive)
				return;

			Gtk::TreeModel::Row row = *(ref_list_model->append());
			row[col_drive_ptr] = drive;

			this->decorate_entry(row);

			drive->signal_changed.connect(sigc::mem_fun(this, &GscMainWindowIconView::on_drive_changed));

			if (scroll_to_it) {
				Gtk::TreeModel::Path tpath(row);
				// scroll_to_path() and set/get_cursor() are since gtkmm 2.8.

				this->scroll_to_path(tpath, true, 0.5, 0.5);
				// select it (keyboard & selection)
				Gtk::CellRenderer* cell = 0;
				if (this->get_cursor(cell) && cell) {
					this->set_cursor(tpath, *cell, false);
				}
				this->select_path(tpath);  // highlight it
			}

			++num_icons;
		}



		/// Decorate a drive entry (colorize it if it has errors, etc...).
		/// This should be called to update the icon of already refreshed drive.
		void decorate_entry(Gtk::TreePath model_path)
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
			StorageDeviceRefPtr drive = row[col_drive_ptr];

			// it needs this space to be symmetric (why?);
			std::string name;  // = "<big>" + drive->get_device_with_type() + " </big>\n";
			Glib::ustring drive_letters = Glib::Markup::escape_text(drive->format_drive_letters(false));
			if (drive_letters.empty()) {
				drive_letters = "not mounted";
			}
			Glib::ustring drive_letters_with_volname = Glib::Markup::escape_text(drive->format_drive_letters(true));
			if (drive_letters_with_volname.empty()) {
				drive_letters_with_volname = "not mounted";
			}

			// note: if this wraps, it becomes left-aligned in gtk <= 2.10.
			name += (drive->get_model_name().empty() ? Glib::ustring("Unknown model") : Glib::Markup::escape_text(drive->get_model_name()));
			if (rconfig::get_data<bool>("gui/icons_show_device_name")) {
				if (!drive->get_is_virtual()) {
					name += "\n" + Glib::Markup::escape_text(drive->get_device_with_type());
				#ifdef _WIN32
					name += " (" + drive_letters + ")";
				#endif
				} else if (!drive->get_virtual_filename().empty()) {
					name += "\n" + Glib::Markup::escape_text(drive->get_virtual_filename());
				}
			}
			if (rconfig::get_data<bool>("gui/icons_show_serial_number") && !drive->get_serial_number().empty()) {
				name += "\n" + Glib::Markup::escape_text(drive->get_serial_number());
			}
			StorageProperty scan_time_prop;
			if (drive->get_is_virtual()) {
				scan_time_prop = drive->lookup_property("scan_time");
				if (!scan_time_prop.value_string.empty()) {
					name += "\n" + Glib::Markup::escape_text(scan_time_prop.value_string);
				}
			}

			std::vector<std::string> tooltip_strs;

			if (drive->get_is_virtual()) {
				std::string vfile = drive->get_virtual_filename();
				tooltip_strs.push_back("Loaded from: " + (vfile.empty() ? "[empty]" : Glib::Markup::escape_text(vfile)));
				if (!scan_time_prop.value_string.empty()) {
					tooltip_strs.push_back("Scanned on: " + Glib::Markup::escape_text(scan_time_prop.value_string));
				}
			} else {
				tooltip_strs.push_back("Device: <b>" + Glib::Markup::escape_text(drive->get_device_with_type()) + "</b>");
			}

		#ifdef _WIN32
			tooltip_strs.push_back("Drive letters: <b>" + drive_letters_with_volname + "</b>");
		#endif

			if (!drive->get_serial_number().empty()) {
				tooltip_strs.push_back("Serial number: <b>" + Glib::Markup::escape_text(drive->get_serial_number()) + "</b>");
			}
			tooltip_strs.push_back("SMART status: <b>"
					+ StorageDevice::get_status_name(drive->get_smart_status(), false) + "</b>");
			tooltip_strs.push_back("Automatic Offline Data Collection status: <b>"
					+ StorageDevice::get_status_name(drive->get_aodc_status(), false) + "</b>");

			std::string tooltip_str = hz::string_join(tooltip_strs, '\n');


			Glib::RefPtr<Gdk::Pixbuf> icon;
			switch(drive->get_detected_type()) {
				case StorageDevice::detected_type_cddvd:
					icon = cddvd_icon;
					break;
				case StorageDevice::detected_type_unknown:  // standard HD icon
				case StorageDevice::detected_type_invalid:
				case StorageDevice::detected_type_raid:  // TODO a separate icon for this
					icon = hd_icon;
					break;
			}

			StorageProperty health_prop = drive->get_health_property();
			if (health_prop.warning != StorageProperty::warning_none && health_prop.generic_name == "overall_health") {
				if (icon) {
					icon = icon->copy();  // work on a copy
					if (icon->get_colorspace() == Gdk::COLORSPACE_RGB && icon->get_bits_per_sample() == 8) {
						int n_channels = icon->get_n_channels();
						int icon_width = icon->get_width();
						int icon_height = icon->get_height();
						int rowstride = icon->get_rowstride();
						guint8* pixels = icon->get_pixels();

						guint8* p = 0;
						for (int y = 0; y < icon_height; ++y) {
							for (int x = 0; x < icon_width; ++x) {
								p = pixels + y * rowstride + x * n_channels;
								uint8_t avg = static_cast<uint8_t>(std::floor((p[0] * 0.30) + (p[1] * 0.59) + (p[2] * 0.11) + 0.001 + 0.5));
								p[0] = avg;  // R
								p[1] = 0;  // G
								p[2] = 0;  // B
							}
						}
					}
				}

				tooltip_str += "\n\n" + storage_property_get_warning_reason(health_prop)
						+ "\n\nView details for more information.";
			}


			// we use all these if-s because changing the data (without actually changing it)
			// sometimes leads to screwed up icons in iconview (blame gtk).

			if (row.get_value(col_name) != name)
				row[col_name] = name;
			if (row.get_value(col_description) != tooltip_str)
				row[col_description] = tooltip_str;

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
			if (this->get_selected_items().size()) {
				Gtk::TreePath model_path = *(this->get_selected_items().begin());
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
		StorageDeviceRefPtr get_selected_drive()
		{
			StorageDeviceRefPtr drive = 0;
			if (this->get_selected_items().size()) {
				Gtk::TreePath model_path = *(this->get_selected_items().begin());
				Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
				drive = row[col_drive_ptr];
			}
			return drive;
		}



		/// Get tree path by a drive
		Gtk::TreePath get_path_by_drive(StorageDevice* drive)
		{
			Gtk::TreeNodeChildren children = ref_list_model->children();
			for (Gtk::TreeNodeChildren::const_iterator iter = children.begin(); iter != children.end(); ++iter) {
				// convert iter to row (iter is row's base, but can we cast it?)
				Gtk::TreeModel::Row row = *iter;
				if (drive == row.get_value(col_drive_ptr))
					return ref_list_model->get_path(row);
			}
			return Gtk::TreePath();  // check with .empty()
		}



		/// Update menu actions in the Drives menu
		void update_menu_actions()
		{
			// if there's nothing selected, disable items from "Drives" menu
			if (!this->get_selected_items().size()) {
				main_window->set_drive_menu_status(NULL);

			} else {  // enable drives menu, set proper smart toggles
				Gtk::TreePath model_path = *(this->get_selected_items().begin());
				Gtk::TreeModel::Row row = *(ref_list_model->get_iter(model_path));
				StorageDeviceRefPtr drive = row[col_drive_ptr];

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
			StorageDeviceRefPtr drive = row[col_drive_ptr];

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
				StorageDeviceRefPtr drive = 0;  // clicked drive (if any)

				// don't use get_item_at_pos() - it's not available in gtkmm < 2.8.
				Gtk::TreePath tpath = this->get_path_at_pos(static_cast<int>(event_button->x),
						static_cast<int>(event_button->y));
				// if (this->get_item_at_pos(static_cast<int>(event_button->x), static_cast<int>(event_button->y), model_path)) {

				if (tpath.gobj() && !tpath.empty()) {  // without gobj() check gtkmm 2.6 (but not 2.12) prints lots of errors
					// move keyboard focus to the icon (just as left-click does)

					Gtk::CellRenderer* cell = 0;
					if (this->get_cursor(cell) && cell) {
						// gtkmm's set_cursor() is undefined (but declared) in 2.8, so use gtk variant.
						gtk_icon_view_set_cursor(GTK_ICON_VIEW(this->gobj()), tpath.gobj(), cell->gobj(), false);
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
		Gtk::TreeModelColumn<StorageDeviceRefPtr> col_drive_ptr;  ///< Model column

		Glib::RefPtr<Gtk::ListStore> ref_list_model;  ///< The icon view model
		unsigned int num_icons;  ///< Track the number of icons, because liststore makes it difficult to count them.

		// available icons
		Glib::RefPtr<Gdk::Pixbuf> hd_icon;  ///< Icon pixbuf
		Glib::RefPtr<Gdk::Pixbuf> cddvd_icon;  ///< Icon pixbuf

		GscMainWindow* main_window;  ///< The main window, our parent

		message_t empty_view_message;  ///< Message type to display when not showing any icons

};








#endif

/// @}
