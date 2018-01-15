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

#ifndef GSC_ABOUT_DIALOG_H
#define GSC_ABOUT_DIALOG_H

#include <gtkmm.h>

#include "applib/app_ui_res_utils.h"




/// The About dialog.
/// Use create() / destroy() with this class instead of new / delete!
class GscAboutDialog : public AppUIResWidget<GscAboutDialog, false, Gtk::AboutDialog> {
	public:

		// name of ui file without a .ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_about_dialog);

		// we need the license file to show it.
		HZ_RES_DATA_INIT_NAMED(LICENSE_gsmartcontrol_txt,
				"LICENSE_gsmartcontrol.txt", LicenseTextResData);

		// show the authors
		HZ_RES_DATA_INIT_NAMED(AUTHORS_txt, "AUTHORS.txt", AuthorsTextResData);


		/// Constructor, GtkBuilder needs this.
		GscAboutDialog(BaseObjectType* gtkcobj, const Glib::RefPtr<Gtk::Builder>& ref_ui);



	protected:

		// -------------------- Callbacks


		/// Callback - dialog response
		void on_response_before(int response_id);


		bool on_activate_link_before(const std::string& uri);

		// ---------- override virtual methods

		// we use .run(), so we don't need this
/*
		// by default, delete_event calls hide().
		bool on_delete_event_before(GdkEventAny* e)
		{
			destroy(this);  // deletes this object and nullifies instance
			return true;  // event handled, don't call default virtual handler
		}
*/

};






#endif

/// @}
