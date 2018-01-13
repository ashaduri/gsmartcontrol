/**************************************************************************
 Copyright:
      (C) 2011 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_ADD_DEVICE_WINDOW_H
#define GSC_ADD_DEVICE_WINDOW_H

#include <gtkmm.h>

#include "applib/app_ui_res_utils.h"



class GscMainWindow;


/// The "Add Device" window.
/// Use create() / destroy() with this class instead of new / delete!
class GscAddDeviceWindow : public AppUIResWidget<GscAddDeviceWindow, true> {
	public:

		// name of ui file without a .ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_add_device_window);


		/// Constructor, GtkBuilder needs this.
		GscAddDeviceWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);

		/// Virtual destructor
		virtual ~GscAddDeviceWindow()
		{ }


		/// Set the main window.
		/// On OK button click main_window->add_device() will be called.
		void set_main_window(GscMainWindow* main_window);


	protected:


		// ---------- overriden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e);


		// ---------- other callbacks

		/// Button click callback
		void on_window_cancel_button_clicked();

		/// Button click callback
		void on_window_ok_button_clicked();

		/// Button click callback
		void on_device_name_browse_button_clicked();

		/// Entry text change callback
		void on_device_name_entry_changed();


	private:

		GscMainWindow* main_window_;  ///< The main window that created us

};






#endif

/// @}
