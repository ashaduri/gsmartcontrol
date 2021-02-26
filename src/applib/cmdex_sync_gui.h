/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_CMDEX_SYNC_GUI_H
#define APP_CMDEX_SYNC_GUI_H

// TODO Remove this in gtkmm4.
#include "local_glibmm.h"

#include <gtkmm.h>

#include "cmdex_sync.h"



/// Same as CmdexSync, but with GTK UI support.
/// This one is noncopyable, because we can't copy the dialogs, etc...
class CmdexSyncGui : public CmdexSync {
	public:

		/// Constructor
		CmdexSyncGui(const std::string& cmd, const std::string& cmdargs)
				: CmdexSync(cmd, cmdargs)
		{
			signal_execute_tick.connect(sigc::mem_fun(*this, &CmdexSyncGui::execute_tick_func));
		}


		/// Constructor
		CmdexSyncGui()
		{
			signal_execute_tick.connect(sigc::mem_fun(*this, &CmdexSyncGui::execute_tick_func));
		}


		/// Non-construction-copyable
		CmdexSyncGui(const CmdexSyncGui& other) = delete;

		/// Non-copyable
		CmdexSyncGui& operator=(const CmdexSyncGui&) = delete;


		/// Destructor
		~CmdexSyncGui()
		{
			delete running_dialog_;
		}


		// Reimplemented from CmdexSync
		bool execute() override;


		/// UI callbacks may use this to abort execution.
		void set_should_abort()
		{
			should_abort_ = true;
		}


		/// Create a "running" dialog or return already existing one.
		/// The dialog will be auto-created and displayed on execute().
		/// You need the function only if you intend to modify it before execute().
		Gtk::MessageDialog* create_running_dialog(Gtk::Window* parent = nullptr, const Glib::ustring& msg = Glib::ustring());


		/// Return the "running" dialog
		Gtk::MessageDialog* get_running_dialog()
		{
			return running_dialog_;
		}


		/// Show or hide the "running" dialog.
		/// This actually shows the dialog only after some time has passed,
		/// to avoid very quick show/hide in case the command exits very quickly.
		void show_hide_dialog(bool show);


		/// This function is called from ticker function in running mode
		/// to show the dialog when requested time elapses.
		void update_dialog_show_timer();


		/// Switch the dialog to "aborting..." mode
		void set_running_dialog_abort_mode(bool aborting);


	private:

		/// Dialog response callback.
		void on_running_dialog_response(int response_id)
		{
			// Try to abort if Cancel was clicked
			if (response_id == Gtk::RESPONSE_CANCEL)
				set_should_abort();
		}


		/// This function is attached to CmdexSync::signal_execute_tick.
		bool execute_tick_func(TickStatus status);


		bool execution_running_ = false;  ///< If true, the execution is still in progress
		bool should_abort_ = false;  ///< GUI callbacks may set this to abort the execution

		Gtk::MessageDialog* running_dialog_ = nullptr;  ///< "Running" dialog
		bool running_dialog_shown_ = false;  ///< If true, the "running" dialog is visible
		bool running_dialog_abort_mode_ = false;  ///< If true, the "running" dialog is in "aborting..." mode.
		Glib::Timer running_dialog_timer_;  ///< "Running" dialog show timer.

};







#endif

/// @}
