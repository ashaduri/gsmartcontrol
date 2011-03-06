/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_EXECUTOR_LOG_WINDOW_H
#define GSC_EXECUTOR_LOG_WINDOW_H

#include <vector>
#include <cstddef>  // std::size_t
#include <gtkmm/window.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treemodelcolumn.h>

#include "applib/app_ui_res_utils.h"
#include "applib/cmdex_sync.h"




// use create() / destroy() with this class instead of new / delete!

class GscExecutorLogWindow : public AppUIResWidget<GscExecutorLogWindow, false> {

	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_executor_log_window);


		// glade/gtkbuilder needs this constructor
		GscExecutorLogWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);


		virtual ~GscExecutorLogWindow()
		{ }


		// Show this window and select the last entry
		void show_last();


	protected:

		// Data

		std::vector<CmdexSyncCommandInfoRefPtr> entries;


		Glib::RefPtr<Gtk::ListStore> list_store;
		Glib::RefPtr<Gtk::TreeSelection> selection;

		Gtk::TreeModelColumn<std::size_t> col_num;
		Gtk::TreeModelColumn<std::string> col_command;
		Gtk::TreeModelColumn<CmdexSyncCommandInfoRefPtr> col_entry;


		void clear_view_widgets();  // clear entries and textviews



		// -------------------- Callbacks

		// Attached to external source
		void on_command_output_received(const CmdexSyncCommandInfo& info);



		// ---------- override virtual methods

		bool on_delete_event_before(GdkEventAny* e)
		{
			this->hide();  // hide only, don't destroy
			return true;  // event handled, don't call default virtual handler
		}


		// ---------- other callbacks

		void on_window_close_button_clicked()
		{
			this->hide();  // hide only, don't destroy
		}


		void on_window_save_current_button_clicked();


		void on_window_save_all_button_clicked();


		void on_clear_command_list_button_clicked();


		void on_tree_selection_changed();

};






#endif
