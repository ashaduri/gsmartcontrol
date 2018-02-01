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

#include "applib/app_builder_widget.h"



class GscMainWindow;


/// The "Add Device" window.
/// Use create() / destroy() with this class instead of new / delete!
class GscAddDeviceWindow : public AppBuilderWidget<GscAddDeviceWindow, true> {
	public:

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline std::string ui_name = "gsc_add_device_window";


		/// Constructor, GtkBuilder needs this.
		GscAddDeviceWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);


		/// Set the main window.
		/// On OK button click main_window->add_device() will be called.
		void set_main_window(GscMainWindow* main_window);


	protected:


		// ---------- overriden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// Reimplemented from Gtk::Window.
		bool on_delete_event(GdkEventAny* e) override;


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

		GscMainWindow* main_window_ = nullptr;  ///< The main window that created us

};






#endif

/// @}
