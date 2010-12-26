/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_HELP_WINDOW_H
#define GSC_HELP_WINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treemodelcolumn.h>

#include "applib/app_ui_res_utils.h"




// use create() / destroy() with this class instead of new / delete!

class GscHelpWindow : public AppUIResWidget<GscHelpWindow, false> {

	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_help_window);


		// glade/gtkbuilder needs this constructor
		GscHelpWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);


		virtual ~GscHelpWindow()
		{ }


		void set_topic(const Glib::ustring& topic);


	protected:

		// Data

		Glib::RefPtr<Gtk::ListStore> list_store;
		Glib::RefPtr<Gtk::TreeSelection> selection;

		Gtk::TreeModelColumn<Glib::ustring> col_topic;

		bool selection_callback_enabled;



		// -------------------- Callbacks

		// ---------- override virtual methods

		bool on_delete_event_before(GdkEventAny* e)
		{
			destroy(this);
			return true;  // event handled, don't call default virtual handler
		}


		// ---------- other callbacks

		void on_window_close_button_clicked()
		{
			destroy(this);
		}


		void on_tree_selection_changed();

};






#endif
