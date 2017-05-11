/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

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




/// The "Execution Log" window.
/// Use create() / destroy() with this class instead of new / delete!
class GscExecutorLogWindow : public AppUIResWidget<GscExecutorLogWindow, false> {
	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_executor_log_window);


		/// Constructor, gtkbuilder/glade needs this.
		GscExecutorLogWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);

		/// Virtual destructor
		virtual ~GscExecutorLogWindow()
		{ }


		/// Show this window and select the last entry
		void show_last();


	protected:


		/// Clear entries and textviews
		void clear_view_widgets();



		// -------------------- callbacks

		/// Callback attached to external source, adds entries in real time.
		void on_command_output_received(const CmdexSyncCommandInfo& info);



		// ---------- overriden virtual methods

		/// Hide the window, don't destroy.
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks

		/// Button click callback
		void on_window_close_button_clicked();

		/// Button click callback
		void on_window_save_current_button_clicked();

		/// Button click callback
		void on_window_save_all_button_clicked();

		/// Button click callback
		void on_clear_command_list_button_clicked();

		/// Callback
		void on_tree_selection_changed();


	private:

		std::vector<CmdexSyncCommandInfoRefPtr> entries;  ///< Command information entries


		Glib::RefPtr<Gtk::ListStore> list_store;  ///< List store
		Glib::RefPtr<Gtk::TreeSelection> selection;  ///< Tree selection

		Gtk::TreeModelColumn<std::size_t> col_num;  ///< Tree column
		Gtk::TreeModelColumn<std::string> col_command;  ///< Tree column
		Gtk::TreeModelColumn<CmdexSyncCommandInfoRefPtr> col_entry;  ///< Tree column


};






#endif

/// @}
