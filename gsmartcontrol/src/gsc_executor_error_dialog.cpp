/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>

#include "gsc_executor_log_window.h"
#include "gsc_executor_error_dialog.h"




void gsc_executor_error_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool show_output_button, bool sec_msg_markup)
{
	// no markup, modal
	Gtk::MessageDialog dialog("\n" + message + (sec_message.empty() ? "\n" : ""),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE, true);

	if (!sec_message.empty())
		dialog.set_secondary_text(sec_message, sec_msg_markup);

	if (parent) {
		dialog.set_transient_for(*parent);
		dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	} else {
		dialog.set_position(Gtk::WIN_POS_MOUSE);
	}


	Gtk::Button ok_button(Gtk::Stock::OK);
	ok_button.show_all();
	ok_button.set_flags(ok_button.get_flags() | Gtk::CAN_DEFAULT);
	dialog.add_action_widget(ok_button, Gtk::RESPONSE_OK);


	Gtk::Button output_button("_Show Output", true);  // don't put this inside if, it needs to live beyond it.
	if (show_output_button) {
		output_button.show_all();
		dialog.add_action_widget(output_button, Gtk::RESPONSE_HELP);
	}

	dialog.set_default_response(Gtk::RESPONSE_OK);


	int response = dialog.run();  // blocks until the dialog is closed

	if (response == Gtk::RESPONSE_HELP) {
		// this one will only hide on close.
		GscExecutorLogWindow* win = GscExecutorLogWindow::create();  // probably already created
		// win->set_transient_for(*this);  // don't do this - it will make it always-on-top of this.
		win->show_last();  // show the window and select last entry
	}

}





