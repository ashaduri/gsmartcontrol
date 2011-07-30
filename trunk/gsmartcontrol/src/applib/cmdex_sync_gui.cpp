/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <gtkmm/main.h>  // Gtk::Main

#include "hz/fs_path.h"
#include "hz/string_sprintf.h"

#include "app_gtkmm_features.h"  // APP_GTKMM_CHECK_VERSION
#include "cmdex_sync_gui.h"




bool CmdexSyncGui::execute()
{
	this->create_running_dialog();  // create, but don't show.
	this->set_running_dialog_abort_mode(false);  // reset and set the message
	return CmdexSync::execute();
}


#if APP_GTKMM_CHECK_VERSION(2, 10, 0)
	// these are available since gtkmm 2.10
	#define CMDEX_DIALOG_MESSAGE_TYPE Gtk::MESSAGE_OTHER
	// not present in Gdk:: in 2.10 (it's in 2.12), use we have to use GDK_
	#define CMDEX_DIALOG_HINT_TYPE GDK_WINDOW_TYPE_HINT_NOTIFICATION
#else
	#define CMDEX_DIALOG_MESSAGE_TYPE Gtk::MESSAGE_INFO
	#define CMDEX_DIALOG_HINT_TYPE GDK_WINDOW_TYPE_HINT_UTILITY
#endif



Gtk::MessageDialog* CmdexSyncGui::create_running_dialog(Gtk::Window* parent, const Glib::ustring& msg)
{
	if (running_dialog_)
		return running_dialog_;

	if (!msg.empty())
		set_running_msg(msg);

	// Construct the dialog so we can manipulate it before execution
	if (parent) {
		running_dialog_ = new Gtk::MessageDialog(*parent, "", false,
				CMDEX_DIALOG_MESSAGE_TYPE, Gtk::BUTTONS_CANCEL);
	} else {
		running_dialog_ = new Gtk::MessageDialog("", false,
				CMDEX_DIALOG_MESSAGE_TYPE, Gtk::BUTTONS_CANCEL);
	}

	running_dialog_->signal_response().connect(sigc::mem_fun(*this,
			&CmdexSyncGui::on_running_dialog_response));

	running_dialog_->set_decorated(false);
#if APP_GTKMM_CHECK_VERSION(2, 10, 0)
	running_dialog_->set_deletable(false);  // since 2.10
#endif
	running_dialog_->set_skip_pager_hint(true);
	running_dialog_->set_skip_taskbar_hint(true);
	running_dialog_->set_type_hint(Gdk::WindowTypeHint(CMDEX_DIALOG_HINT_TYPE));
	running_dialog_->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	// avoid running multiple programs in parallel (the dialogs can be overlap...).
	// this won't harm the tests - they don't involve long-running commands.
	running_dialog_->set_modal(true);

	return running_dialog_;
}



void CmdexSyncGui::show_hide_dialog(bool show)
{
	if (running_dialog_) {
		if (show) {
			running_dialog_timer_.start();
			// running_dialog_->show();

		} else {
			running_dialog_->hide();
			running_dialog_timer_.stop();
			running_dialog_shown_ = false;
		}
	}
}




void CmdexSyncGui::update_dialog_show_timer()
{
	double timeout = 0.600;  // 0.6 sec for normal dialogs
	if (running_dialog_abort_mode_)
		timeout = 0.150;  // 0.15 sec for aborting... dialogs

	if (!running_dialog_shown_ && running_dialog_timer_.elapsed() > 1.000) {  // 1 sec

		// without first making it sensitive, the "whole label selected" problem may occur.
		running_dialog_->set_response_sensitive(Gtk::RESPONSE_CANCEL, true);
		running_dialog_->show();

		// enable / disable the button. do this after show(), or else the label gets selected or the cursor gets visible.
		running_dialog_->set_response_sensitive(Gtk::RESPONSE_CANCEL, !running_dialog_abort_mode_);

		running_dialog_shown_ = true;
	}
}




void CmdexSyncGui::set_running_dialog_abort_mode(bool aborting)
{
	if (!running_dialog_)
		return;

	if (aborting && !running_dialog_abort_mode_) {
		// hide it until another timeout passes. this way, we:
		// avoid quick show/hide flickering;
		// avoid a strange problem when sensitive but clear dialog appears;
		// make it show at center of parent.

		show_hide_dialog(false);

		running_dialog_->set_message("\n     Aborting...     ");
		// the sensitive button switching is done after show(), to avoid some visual
		// defects - cursor in label, selected label.

		show_hide_dialog(true);  // this resets the timer

		running_dialog_abort_mode_ = true;


	} else if (!aborting) {
		std::string msg = hz::string_sprintf(get_running_msg().c_str(),
				hz::FsPath(this->get_command_name()).get_basename().c_str());
		running_dialog_->set_message("\n     " + msg + "     ");
		// running_dialog_->set_response_sensitive(Gtk::RESPONSE_CANCEL, true);

		running_dialog_abort_mode_ = false;
	}
}



bool CmdexSyncGui::execute_tick_func(tick_status_t status)
{
	if (status == status_starting) {
		if (execution_running_)
			return false;  // already running, abort the new one (?)

		// If quit() was called during one of the manual iterations, and execute()
		// is called in a loop, we need to prevent any real execution past that point.
		if (Gtk::Main::iteration(false) && Gtk::Main::level() > 0) {
			return false;  // try to abort execution
		}

		execution_running_ = true;
		should_abort_ = false;

		// show a dialog with "running..." and an Abort button
		this->show_hide_dialog(true);

		return true;  // proceed with execution
	}


	if (status == status_failed) {

		// close the dialog
		this->show_hide_dialog(false);

		// show a dialog with an error.
		// Handled by sync_errors_warn(), nothing else is needed here.

		execution_running_ = false;
		return true;  // return value is ignored here
	}


	if (status == status_running) {

		while (Gtk::Main::events_pending()) {
			// Gtk::Main::iteration() returns true if Gtk::Main::quit() has been called, or if there's no Main yet.
			// debug_out_dump("app", Gtk::Main::level() << "\n");
			if (Gtk::Main::iteration() && Gtk::Main::level() > 0) {
				set_running_dialog_abort_mode(true);
				return false;  // try to abort execution
			}
		}

		if (should_abort_) {
			should_abort_ = false;
			set_running_dialog_abort_mode(true);
			return false;  // try to abort execution
		}

		// the dialog may be shown only after some time has passed, to avoid quick show/hide.
		// this enables it.
		this->update_dialog_show_timer();

		return true;  // continue execution
	}


	if (status == status_stopping) {
		if (Gtk::Main::iteration(false) && Gtk::Main::level() > 0) {
			return false;  // we're exiting from the main loop, so return early
		}

		// show a dialog with "Aborting..."
		this->update_dialog_show_timer();
		return true;  // return value is ignored here
	}


	if (status == status_stopped) {
		// close the dialog.
		this->show_hide_dialog(false);

		// show error messages if needed (get_errors()).
		// Handled by sync_errors_warn(), nothing is needed here.

		execution_running_ = false;
		return true;  // return value is ignored here
	}


	return true;  // we shouldn't reach this
}






/// @}
