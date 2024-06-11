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

#ifndef GSC_MAIN_WINDOW_ICONVIEW_H
#define GSC_MAIN_WINDOW_ICONVIEW_H

#include <glibmm.h>
#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <string>

#include "gsc_main_window.h"
#include "applib/storage_device.h"



/// The icon view of the main window (shows a drive list).
/// Note: The IconView must have a fixed icon width set (e.g. in .ui file).
/// Otherwise, it doesn't re-compute it when clearing and adding new icons.
class GscMainWindowIconView : public Gtk::IconView {
	public:

		/// Message type to show
		enum class Message {
			None,  ///< No message
			ScanDisabled,  ///< Scanning is disabled
			Scanning,  ///< Scanning drives...
			NoDrivesFound,  ///< No drives found
			NoSmartctl,  ///< No smartctl installed
			PleaseRescan,  ///< Re-scan to see the drives
		};


		/// Get message string
		[[nodiscard]] static std::string get_message_string(Message type);


		/// Constructor, GtkBuilder needs this.
		[[maybe_unused]] GscMainWindowIconView(BaseObjectType* gtkcobj, [[maybe_unused]] const Glib::RefPtr<Gtk::Builder>& ref_ui);


		/// Set the parent window
		void set_main_window(GscMainWindow* w);


		/// Set the message type to display when there are no icons to show
		void set_empty_view_message(Message message);


		/// Get the number of icons currently displayed
		[[nodiscard]] int get_num_icons() const;


		// Overridden from Gtk::Widget
		bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;


		/// Cell data renderer (needed for high quality icons in GDK_SCALE=2).
		/// We have to use Cairo surfaces, because pixbufs are scaled by GtkIconView.
		void on_cell_data_render(const Gtk::TreeModel::const_iterator& iter);


		/// Add a drive entry to the icon view
		void add_entry(StorageDevicePtr drive, bool scroll_to_it = false);


		/// Decorate a drive entry (colorize it if it has errors, etc.).
		/// This should be called to update the icon of already refreshed drive.
		void decorate_entry(const Gtk::TreePath& model_path);


		/// Decorate a drive entry (colorize it if it has errors, etc.).
		/// This should be called to update the icon of already refreshed drive.
		void decorate_entry(Gtk::TreeModel::Row& row);


		/// Remove drive entry
		void remove_entry(const Gtk::TreePath& model_path);


		/// Remove selected drive entry
		void remove_selected_drive();


		/// Remove all entries
		void clear_all();


		/// Get selected drive
		[[nodiscard]] StorageDevicePtr get_selected_drive();


		/// Get tree path by a drive
		[[nodiscard]] Gtk::TreePath get_path_by_drive(StorageDevice* drive);


		/// Update menu actions in the Drives menu
		void update_menu_actions();


	protected:

		/// Callback
		void on_iconview_item_activated(const Gtk::TreePath& model_path);


		/// Callback
		void on_iconview_selection_changed();


		/// Callback
		bool on_iconview_button_press_event(GdkEventButton* event_button);


		/// Callback attached to StorageDevice, updates its view.
		void on_drive_changed(StorageDevice* drive);


		/// Load drive-type-specific icons
		void load_icon_pixbufs();


		/// Load drive-type-specific icons
		[[nodiscard]] Glib::RefPtr<Gdk::Pixbuf> load_icon_pixbuf(Glib::RefPtr<Gtk::IconTheme> default_icon_theme,
				const std::string& xdg_icon_name, const std::string& bundled_icon_filename);


	private:

		Gtk::TreeModel::ColumnRecord columns_;  ///< Model columns
		Gtk::CellRendererPixbuf cell_renderer_pixbuf_;  ///< Cell renderer for icons.

		Gtk::TreeModelColumn<std::string> col_name_;  ///< Model column
		Gtk::TreeModelColumn<Glib::ustring> col_description_;  ///< Model column
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > col_pixbuf_;  ///< Model column
		Gtk::TreeModelColumn<StorageDevicePtr> col_drive_ptr_;  ///< Model column
		Gtk::TreeModelColumn<bool> col_populated_;  ///< Model column, indicates whether the model entry has been fully populated.

		Glib::RefPtr<Gtk::ListStore> ref_list_model_;  ///< The icon view model
		int num_icons_ = 0;  ///< Track the number of icons, because liststore makes it difficult to count them.

		/// Adwaita's drive-harddisk icons are tiny at 48, so 64 is better.
		/// Plus, 64 scales well to 128 and 256 (if using GDK_SCALE).
		const int icon_size_ = 64;

		Glib::RefPtr<Gdk::Pixbuf> default_icon_;  ///< Icon pixbuf, used when type-specific icon is missing
		std::unordered_map<StorageDeviceDetectedType, Glib::RefPtr<Gdk::Pixbuf>> icon_pixbufs_;  ///< Icons for different drive types

		GscMainWindow* main_window_ = nullptr;  ///< The main window, our parent

		Message empty_view_message_ = Message::None;  ///< Message type to display when not showing any icons

};








#endif

/// @}
