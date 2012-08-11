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

#ifndef GSC_ABOUT_DIALOG_H
#define GSC_ABOUT_DIALOG_H

#include <gtkmm/window.h>
#include <gtkmm/aboutdialog.h>

#include "applib/app_ui_res_utils.h"




/// The About dialog.
/// Use create() / destroy() with this class instead of new / delete!
class GscAboutDialog : public AppUIResWidget<GscAboutDialog, false, Gtk::AboutDialog> {
	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_about_dialog);

		// we need the license file to show it.
		HZ_RES_DATA_INIT_NAMED(LICENSE_gsmartcontrol_txt,
				"LICENSE_gsmartcontrol.txt", LicenseTextResData);

		// show the authors
		HZ_RES_DATA_INIT_NAMED(AUTHORS_txt, "AUTHORS.txt", AuthorsTextResData);


		/// Constructor, gtkbuilder/glade needs this.
		GscAboutDialog(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui);

		/// Virtual destructor
		virtual ~GscAboutDialog()
		{ }



	protected:

		// -------------------- Callbacks

		/// Callback - activate an URL
		void on_activate_url(Gtk::AboutDialog& about_dialog, const Glib::ustring& link);

		/// Callback - activate an email
		void on_activate_email(Gtk::AboutDialog& about_dialog, const Glib::ustring& link);


		/// Callback - dialog response
		void on_response_before(int response_id);


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
