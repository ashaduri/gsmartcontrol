/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_PREFERENCES_WINDOW_H
#define GSC_PREFERENCES_WINDOW_H

#include <gtkmm/window.h>

#include "applib/app_ui_res_utils.h"



class GscPreferencesDeviceOptionsTreeView;  // defined in cpp file



// use create() / destroy() with this class instead of new / delete!

class GscPreferencesWindow : public AppUIResWidget<GscPreferencesWindow, true> {

	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_preferences_window);


		// glade/gtkbuilder needs this constructor
		GscPreferencesWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);


		virtual ~GscPreferencesWindow()
		{ }


		void update_device_widgets(const std::string& device, const std::string& params);

		void device_widget_set_remove_possible(bool b);


	protected:

		// data

		GscPreferencesDeviceOptionsTreeView* device_options_treeview;


		void import_config();

		void export_config();


		// -------------------- Callbacks

		// ---------- override virtual methods

		// by default, delete_event calls hide().
		bool on_delete_event_before(GdkEventAny* e)
		{
			destroy(this);  // deletes this object and nullifies instance
			return true;  // event handled, don't call default virtual handler
		}


		// ---------- other callbacks

		void on_window_cancel_button_clicked()
		{
			destroy(this);
		}


		void on_window_ok_button_clicked()
		{
			export_config();
			destroy(this);
		}


		void on_window_reset_all_button_clicked();


		void on_smartctl_binary_browse_button_clicked();

		void on_device_options_remove_device_button_clicked();

		void on_device_options_add_device_button_clicked();

		void on_device_options_device_entry_changed();

		void on_device_options_parameter_entry_changed();


};






#endif
