/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <gtkmm/messagedialog.h>

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





