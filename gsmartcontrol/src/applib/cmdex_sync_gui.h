/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef APP_CMDEX_SYNC_GUI_H
#define APP_CMDEX_SYNC_GUI_H

#include <gtkmm/messagedialog.h>
#include <glibmm/timer.h>

#include "hz/noncopyable.h"

#include "cmdex_sync.h"



// CmdexSync with GUI support



// same as CmdexSync, but with GTK UI stuff.
// this one is noncopyable, because we don't copy the dialogs, etc...
class CmdexSyncGui : public CmdexSync, public hz::noncopyable {

	public:

		CmdexSyncGui(const std::string& cmd, const std::string& cmdargs)
			: CmdexSync(cmd, cmdargs), execution_running_(false), should_abort_(false),
				running_dialog_(0), running_dialog_shown_(false), running_dialog_abort_mode_(false)
		{
			signal_execute_tick.connect(sigc::mem_fun(*this, &CmdexSyncGui::execute_tick_func));
		}


		CmdexSyncGui()
			: execution_running_(false), should_abort_(false),
				running_dialog_(0), running_dialog_shown_(false), running_dialog_abort_mode_(false)
		{
			signal_execute_tick.connect(sigc::mem_fun(*this, &CmdexSyncGui::execute_tick_func));
		}


		~CmdexSyncGui()
		{
			delete running_dialog_;
		}


		virtual bool execute();


		// UI callbacks may use this to abort execution.
		void set_should_abort()
		{
			should_abort_ = true;
		}


		// create a dialog or return already existing one.
		// the dialog will be auto-created and displayed on execute().
		// you need the function only if you intend to modify it before execute().
		Gtk::MessageDialog* create_running_dialog(Gtk::Window* parent = 0, const Glib::ustring& msg = "");


		Gtk::MessageDialog* get_running_dialog()
		{
			return running_dialog_;
		}


		// this actually shows the dialog only after some time has passed,
		// to avoid very quick show/hide.
		void show_hide_dialog(bool show);


		// called from ticker function in running mode, to show the dialog when requested time elapses.
		void update_dialog_show_timer();



		// switch the dialog to "Aborting..." mode
		void set_running_dialog_abort_mode(bool aborting);



	private:

		// dialog callback.
		void on_running_dialog_response(int response_id)
		{
			// Try to abort if Cancel was clicked
			if (response_id == Gtk::RESPONSE_CANCEL)
				set_should_abort();
		}


		// this function is called every time something happens
		// with the executor.
		bool execute_tick_func(tick_status_t status);


		bool execution_running_;
		bool should_abort_;  // GUI callbacks may set this to abort the execution

		Gtk::MessageDialog* running_dialog_;
		bool running_dialog_shown_;
		bool running_dialog_abort_mode_;
		Glib::Timer running_dialog_timer_;

};







#endif
