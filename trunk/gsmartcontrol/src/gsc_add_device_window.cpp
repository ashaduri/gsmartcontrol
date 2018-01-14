/**************************************************************************
 Copyright:
      (C) 2011 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // NOT NEEDED
#undef throw

#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape
#include <gtk/gtk.h>

#include "hz/fs_path.h"
#include "hz/string_sprintf.h"
#include "hz/scoped_ptr.h"
#include "applib/app_gtkmm_utils.h"

#include "gsc_add_device_window.h"
#include "gsc_main_window.h"




GscAddDeviceWindow::GscAddDeviceWindow(BaseObjectType* gtkcobj, const Glib::RefPtr<Gtk::Builder>& ref_ui)
		: AppUIResWidget<GscAddDeviceWindow, true>(gtkcobj, ref_ui), main_window_(0)
{
	// Connect callbacks

	APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	Gtk::Button* window_cancel_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_cancel_button, clicked);

	Gtk::Button* window_ok_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_ok_button, clicked);

	Gtk::Button* device_name_browse_button = 0;
	APP_UI_RES_AUTO_CONNECT(device_name_browse_button, clicked);


	Glib::ustring device_name_tooltip = "Device name";
#if defined CONFIG_KERNEL_FAMILY_WINDOWS
	device_name_tooltip = "Device name (for example, use \"pd0\" for the first physical drive)";
#elif defined CONFIG_KERNEL_LINUX
	device_name_tooltip = "Device name (for example, /dev/sda or /dev/twa0)";
#endif
	if (Gtk::Label* device_name_label = lookup_widget<Gtk::Label*>("device_name_label")) {
		app_gtkmm_set_widget_tooltip(*device_name_label, device_name_tooltip);
	}

	Gtk::Entry* device_name_entry = 0;
	APP_UI_RES_AUTO_CONNECT(device_name_entry, changed);
	if (device_name_entry) {
		app_gtkmm_set_widget_tooltip(*device_name_entry, device_name_tooltip);
	}


	Glib::ustring device_type_tooltip = "Smartctl -d option parameter";
#if defined CONFIG_KERNEL_LINUX || defined CONFIG_KERNEL_FAMILY_WINDOWS
	device_type_tooltip = "Smartctl -d option parameter. For example, use areca,1 for the first drive behind Areca RAID controller.";
#endif
	if (Gtk::Label* device_type_label = lookup_widget<Gtk::Label*>("device_type_label")) {
		app_gtkmm_set_widget_tooltip(*device_type_label, device_type_tooltip);
	}
	if (Gtk::ComboBoxText* type_combo = lookup_widget<Gtk::ComboBoxText*>("device_type_combo")) {
		app_gtkmm_set_widget_tooltip(*type_combo, device_type_tooltip);
	}


	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_cancel_button) {
		window_cancel_button->add_accelerator("clicked", accel_group, GDK_KEY_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


#ifdef _WIN32
	// "Browse" doesn't make sense in win32, hide it.
	if (device_name_browse_button) {
		device_name_browse_button->hide();
	}
#endif


	// Populate type combo with common types
	Gtk::ComboBoxText* type_combo = lookup_widget<Gtk::ComboBoxText*>("device_type_combo");
	if (type_combo) {
		type_combo->append("sat,12");
		type_combo->append("sat,16");
		type_combo->append("usbcypress");
		type_combo->append("usbjmicron");
		type_combo->append("usbsunplus");
		type_combo->append("ata");
		type_combo->append("scsi");
#if defined CONFIG_KERNEL_LINUX
		type_combo->append("marvell");
		type_combo->append("megaraid,N");
		type_combo->append("areca,N");
		type_combo->append("areca,N/E");
#endif
#if defined CONFIG_KERNEL_LINUX || defined CONFIG_KERNEL_FREEBSD || defined CONFIG_KERNEL_DRAGONFLY
		type_combo->append("3ware,N");  // this option is not needed in windows
		type_combo->append("cciss,N");
		type_combo->append("hpt,L/M");
		type_combo->append("hpt,L/M/N");
#endif
	}


	// This sets the initial state of OK button
	on_device_name_entry_changed();

	// show();
}



void GscAddDeviceWindow::set_main_window(GscMainWindow* main_window)
{
	main_window_ = main_window;
}



bool GscAddDeviceWindow::on_delete_event_before([[maybe_unused]] GdkEventAny* e)
{
	destroy(this);  // deletes this object and nullifies instance
	return true;  // event handled, don't call default virtual handler
}



void GscAddDeviceWindow::on_window_cancel_button_clicked()
{
	destroy(this);
}



void GscAddDeviceWindow::on_window_ok_button_clicked()
{
	std::string dev, type, params;
	if (Gtk::Entry* entry = lookup_widget<Gtk::Entry*>("device_name_entry")) {
		dev = entry->get_text();
	}
	if (Gtk::ComboBoxText* type_combo = lookup_widget<Gtk::ComboBoxText*>("device_type_combo")) {
		type = type_combo->get_entry_text();
	}
	if (Gtk::Entry* entry = lookup_widget<Gtk::Entry*>("smartctl_params_entry")) {
		params = entry->get_text();
	}
	if (main_window_ && !dev.empty()) {
		main_window_->add_device(dev, type, params);
	}

	destroy(this);
}



void GscAddDeviceWindow::on_device_name_browse_button_clicked()
{
	std::string default_file;

	Gtk::Entry* entry = this->lookup_widget<Gtk::Entry*>("device_name_entry");
	if (!entry)
		return;

	hz::FsPath path(entry->get_text());

	int result = 0;

#if GTK_CHECK_VERSION(3, 20, 0)
	hz::scoped_ptr<GtkFileChooserNative> dialog(gtk_file_chooser_native_new(
			"Choose Device...", this->gobj(), GTK_FILE_CHOOSER_ACTION_OPEN, nullptr, nullptr), g_object_unref);

	if (path.is_absolute())
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog.get()), path.c_str());

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
	Gtk::FileChooserDialog dialog(*this, "Choose Device...",
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

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



void GscAddDeviceWindow::on_device_name_entry_changed()
{
	// Allow OK only if name is not empty
	Gtk::Entry* entry = lookup_widget<Gtk::Entry*>("device_name_entry");
	Gtk::Button* ok_button = lookup_widget<Gtk::Button*>("window_ok_button");
	if (entry && ok_button) {
		ok_button->set_sensitive(!entry->get_text().empty());
	}
}







/// @}
