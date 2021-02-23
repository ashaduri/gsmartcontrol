/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include <gtkmm.h>
#include <memory>

#include "applib/app_builder_widget.h"
#include "applib/cmdex_sync.h"




/// The "Execution Log" window.
/// Use create() / destroy() with this class instead of new / delete!
class GscExecutorLogWindow : public AppBuilderWidget<GscExecutorLogWindow, false> {
	public:

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline std::string ui_name = "gsc_executor_log_window";


		/// Constructor, GtkBuilder needs this.
		GscExecutorLogWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);


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
		bool on_delete_event(GdkEventAny* e) override;


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

		std::vector<std::shared_ptr<CmdexSyncCommandInfo>> entries;  ///< Command information entries


		Glib::RefPtr<Gtk::ListStore> list_store;  ///< List store
		Glib::RefPtr<Gtk::TreeSelection> selection;  ///< Tree selection

		Gtk::TreeModelColumn<std::size_t> col_num;  ///< Tree column
		Gtk::TreeModelColumn<std::string> col_command;  ///< Tree column
		Gtk::TreeModelColumn<std::shared_ptr<CmdexSyncCommandInfo>> col_entry;  ///< Tree column


};






#endif

/// @}
