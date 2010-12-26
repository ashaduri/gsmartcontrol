/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <gtkmm/messagedialog.h>
#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>

#include "app_pango_utils.h"  // app_pango_strip_markup()
#include "gui_utils.h"


namespace
{

	inline void show_dialog(const std::string& message, const std::string& sec_message,
			Gtk::Window* parent, Gtk::MessageType type, bool sec_msg_markup)
	{
		// no markup, modal
		Gtk::MessageDialog dialog("\n" + message + (sec_message.empty() ? "\n" : ""),
				false, type, Gtk::BUTTONS_OK, true);

		if (!sec_message.empty())
			dialog.set_secondary_text(sec_message, sec_msg_markup);

		if (parent) {
			dialog.set_transient_for(*parent);
			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
		} else {
			dialog.set_position(Gtk::WIN_POS_MOUSE);
		}

		dialog.run();  // blocks until the dialog is closed
	}

}



void gui_show_error_dialog(const std::string& message, Gtk::Window* parent)
{
	show_dialog(message, "", parent, Gtk::MESSAGE_ERROR, false);
}

void gui_show_error_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup)
{
	show_dialog(message, sec_message, parent, Gtk::MESSAGE_ERROR, sec_msg_markup);
}



void gui_show_warn_dialog(const std::string& message, Gtk::Window* parent)
{
	show_dialog(message, "", parent, Gtk::MESSAGE_WARNING, false);
}

void gui_show_warn_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup)
{
	show_dialog(message, sec_message, parent, Gtk::MESSAGE_WARNING, sec_msg_markup);
}



void gui_show_info_dialog(const std::string& message, Gtk::Window* parent)
{
	show_dialog(message, "", parent, Gtk::MESSAGE_INFO, false);
}

void gui_show_info_dialog(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup)
{
	show_dialog(message, sec_message, parent, Gtk::MESSAGE_INFO, sec_msg_markup);
}



// Returns false if Cancel was clicked. Puts the user-entered string into result otherwise.
bool gui_show_text_entry_dialog(const std::string& title, const std::string& message,
		std::string& result, const std::string& default_str, Gtk::Window* parent)
{
	return gui_show_text_entry_dialog(title, message, "", result, default_str, parent, false);
}


bool gui_show_text_entry_dialog(const std::string& title, const std::string& message, const std::string& sec_message,
		std::string& result, const std::string& default_str, Gtk::Window* parent, bool sec_msg_markup)
{
	int response = 0;
	std::string input_str;

	{  // the dialog hides at the end of scope
		Gtk::Dialog dialog(title, true);  // modal

		dialog.set_resizable(false);
		dialog.set_skip_taskbar_hint(true);
		dialog.set_border_width(5);

		if (parent) {
			dialog.set_transient_for(*parent);
			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
		} else {
			dialog.set_position(Gtk::WIN_POS_MOUSE);
		}


		Gtk::Label main_label;
		main_label.set_markup("<big><b>" + Glib::Markup::escape_text(message)
				+ (sec_message.empty() ? "\n" : "") + "</b></big>");
		main_label.set_line_wrap(true);
		main_label.set_selectable(true);
		main_label.set_alignment(0.0, 0.0);

		Gtk::Label sec_label;
		if (sec_msg_markup) {
			sec_label.set_markup(sec_message);
		} else {
			sec_label.set_text(sec_message);
		}
		sec_label.set_line_wrap(true);
		sec_label.set_selectable(true);
		sec_label.set_alignment(0.0, 0.0);

		Gtk::Entry input_entry;
		input_entry.set_activates_default(true);


		Gtk::VBox vbox;
		vbox.set_spacing(12);
		vbox.pack_start(main_label, false, false, 0);
		vbox.pack_start(sec_label, true, true, 0);
		vbox.pack_start(input_entry, true, true, 0);
		vbox.show_all();

		dialog.get_action_area()->set_border_width(5);
		dialog.get_action_area()->set_spacing(6);

		dialog.get_vbox()->set_spacing(14);  // as in MessageDialog
		dialog.get_vbox()->pack_start(vbox, false, false, 0);


		dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

		Gtk::Button ok_button(Gtk::Stock::OK);
		ok_button.set_flags(ok_button.get_flags() | Gtk::CAN_DEFAULT);
		ok_button.show_all();
		dialog.add_action_widget(ok_button, Gtk::RESPONSE_OK);
		ok_button.grab_default();  // make it the default widget

		response = dialog.run();  // blocks until the dialog is closed

		input_str = input_entry.get_text();
	}

	if (response == Gtk::RESPONSE_OK) {
		result = input_str;
		return true;
	}

	return false;
}





