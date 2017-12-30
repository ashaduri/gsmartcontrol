/**************************************************************************
 Copyright:
      (C) 2011 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/comboboxentry.h>
#include <gtkmm/treemodelcolumn.h>
#include <gdk/gdkkeysyms.h>  // GDK_Escape

#include "hz/fs_path.h"
#include "hz/string_sprintf.h"
#include "applib/app_gtkmm_utils.h"

#include "gsc_add_device_window.h"
#include "gsc_main_window.h"




GscAddDeviceWindow::GscAddDeviceWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
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
	if (Gtk::Entry* device_type_entry = lookup_widget<Gtk::Entry*>("device_type_entry")) {
		app_gtkmm_set_widget_tooltip(*device_type_entry, device_type_tooltip);
	}


	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_cancel_button) {
		window_cancel_button->add_accelerator("clicked", accel_group, GDK_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


#ifdef _WIN32
	// "Browse" doesn't make sense in win32, hide it.
	if (device_name_browse_button) {
		device_name_browse_button->hide();
	}
#endif


	// Populate type combo with common types
	Gtk::ComboBoxEntry* type_combo = lookup_widget<Gtk::ComboBoxEntry*>("device_type_combo");
	if (type_combo) {
		Gtk::TreeModelColumnRecord column_record; ///< Columns for type combo
		Gtk::TreeModelColumn<Glib::ustring> col_type; ///< Model column

		column_record.add(col_type);

		Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(column_record);
		type_combo->set_model(model);
		type_combo->set_text_column(col_type);

		(*(model->append()))[col_type] = "sat,12";
		(*(model->append()))[col_type] = "sat,16";
		(*(model->append()))[col_type] = "usbcypress";
		(*(model->append()))[col_type] = "usbjmicron";
		(*(model->append()))[col_type] = "usbsunplus";
		(*(model->append()))[col_type] = "ata";
		(*(model->append()))[col_type] = "scsi";
#if defined CONFIG_KERNEL_LINUX
		(*(model->append()))[col_type] = "marvell";
		(*(model->append()))[col_type] = "megaraid,N";
		(*(model->append()))[col_type] = "areca,N";
		(*(model->append()))[col_type] = "areca,N/E";
#endif
#if defined CONFIG_KERNEL_LINUX || defined CONFIG_KERNEL_FREEBSD || defined CONFIG_KERNEL_DRAGONFLY
		(*(model->append()))[col_type] = "3ware,N";  // this option is not needed in windows
		(*(model->append()))[col_type] = "cciss,N";
		(*(model->append()))[col_type] = "hpt,L/M";
		(*(model->append()))[col_type] = "hpt,L/M/N";
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



bool GscAddDeviceWindow::on_delete_event_before(GdkEventAny* e)
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
	if (Gtk::Entry* entry = lookup_widget<Gtk::Entry*>("device_type_entry")) {
		type = entry->get_text();
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

	Gtk::FileChooserDialog dialog(*this, "Choose Device...",
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

	// Note: This works on absolute paths only (otherwise it's gtk warning).
	hz::FsPath p(entry->get_text());
	if (p.is_absolute())
		dialog.set_filename(p.str());  // change to its dir and select it if exists.

	// Show the dialog and wait for a user response
	int result = dialog.run();  // the main cycle blocks here

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			entry->set_text(std::string(dialog.get_filename()));
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
