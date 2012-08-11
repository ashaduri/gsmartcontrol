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

#ifndef GSC_HELP_WINDOW_H
#define GSC_HELP_WINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treemodelcolumn.h>

#include "applib/app_ui_res_utils.h"




/// The Help window.
/// Use create() / destroy() with this class instead of new / delete!
class GscHelpWindow : public AppUIResWidget<GscHelpWindow, false> {
	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_help_window);


		/// Constructor, gtkbuilder/glade needs this.
		GscHelpWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);

		/// Virtual destructor
		virtual ~GscHelpWindow()
		{ }


		/// Set the current help topic
		void set_topic(const Glib::ustring& topic);


	protected:

		// ---------- overriden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks

		/// Button click callback
		void on_window_close_button_clicked();

		/// Callback
		void on_tree_selection_changed();


	private:

		Glib::RefPtr<Gtk::ListStore> list_store;  ///< List store
		Glib::RefPtr<Gtk::TreeSelection> selection;  ///< Tree selection

		Gtk::TreeModelColumn<Glib::ustring> col_topic;  /// Tree column

		bool selection_callback_enabled;  ///< Helper for set_topic(), temporarily disables the tree selection changed callback

};






#endif

/// @}
