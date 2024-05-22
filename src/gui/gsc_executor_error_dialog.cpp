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

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "gsc_executor_log_window.h"
#include "gsc_executor_error_dialog.h"
#include "gsc_text_window.h"




namespace {

	/// Helper function
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
		ok_button.set_can_default(true);
		dialog.add_action_widget(ok_button, Gtk::RESPONSE_OK);


		Gtk::Button output_button(_("_Show Output"), true);  // don't put this inside if, it needs to live beyond it.
		if (show_output_button) {
			output_button.show_all();
			dialog.add_action_widget(output_button, Gtk::RESPONSE_HELP);
		}

		dialog.set_default_response(Gtk::RESPONSE_OK);


		const int response = dialog.run();  // blocks until the dialog is closed

		return response;
	}

}





void gsc_executor_error_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup, bool show_output_button)
{
	const int response = show_executor_dialog(Gtk::MESSAGE_ERROR, message, sec_message,
			parent, sec_msg_markup, show_output_button);

	if (response == Gtk::RESPONSE_HELP) {
		// this one will only hide on close.
		auto win = GscExecutorLogWindow::create();  // probably already created
		// win->set_transient_for(*this);  // don't do this - it will make it always-on-top of this.
		win->show_last();  // show the window and select last entry
	}
}



void gsc_no_info_dialog_show(const std::string& message, const std::string& sec_message,
		Gtk::Window* parent, bool sec_msg_markup, const std::string& output,
		const std::string& output_window_title, const std::string& default_save_filename)
{
	const int response = show_executor_dialog(Gtk::MESSAGE_WARNING, message, sec_message,
			parent, sec_msg_markup, !output.empty());

	if (response == Gtk::RESPONSE_HELP) {
		auto win = GscTextWindow<SmartctlOutputInstance>::create();
		win->set_text_from_command(output_window_title, output);

		if (!default_save_filename.empty())
			win->set_save_filename(default_save_filename);

		win->show();
	}

}





/// @}
