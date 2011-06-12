/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>

#include "gsc_executor_log_window.h"
#include "gsc_executor_error_dialog.h"
#include "gsc_text_window.h"




namespace {

	inline int show_executor_dialog(Gtk::MessageType type,
			const std::string& message, const std::string& sec_message,
			Gtk::Window* parent, bool sec_msg_markup, bool show_output_button)
	{
		// no markup, modal
		Gtk::MessageDialog dialog("\n" + message + (sec_message.empty() ? "\n" : ""),
				false, type, Gtk::BUTTONS_NONE, true);

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

		return response;
	}

}





void gsc_executor_error_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup, bool show_output_button)
{
	int response = show_executor_dialog(Gtk::MESSAGE_ERROR, message, sec_message,
			parent, sec_msg_markup, show_output_button);

	if (response == Gtk::RESPONSE_HELP) {
		// this one will only hide on close.
		GscExecutorLogWindow* win = GscExecutorLogWindow::create();  // probably already created
		// win->set_transient_for(*this);  // don't do this - it will make it always-on-top of this.
		win->show_last();  // show the window and select last entry
	}
}



void gsc_no_info_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup, const std::string& output,
		const std::string& output_window_title, const std::string& default_save_filename)
{
	int response = show_executor_dialog(Gtk::MESSAGE_WARNING, message, sec_message,
			parent, sec_msg_markup, !output.empty());

	if (response == Gtk::RESPONSE_HELP) {
		GscTextWindow<SmartctlOutputInstance>* win = GscTextWindow<SmartctlOutputInstance>::create();
		// make save visible and enable monospace font

		std::string buf_text = output;
		// We receive locale'd thousands separators in win32, so convert them.
		#ifdef _WIN32
		try {
			buf_text = Glib::locale_to_utf8(buf_text);
		} catch (Glib::ConvertError& e) {
			buf_text = "";  // inserting invalid utf8 may trigger a segfault, so empty is better.
		}
		#endif
		win->set_text(output_window_title, buf_text, true, true);

		if (!default_save_filename.empty())
			win->set_save_filename(default_save_filename);

		win->show();
	}

}




