/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_ABOUT_DIALOG_H
#define GSC_ABOUT_DIALOG_H

#include <gtkmm.h>

#include "applib/app_builder_widget.h"




/// The About dialog.
/// Use create() / destroy() with this class instead of new / delete!
class GscAboutDialog : public AppBuilderWidget<GscAboutDialog, false, Gtk::AboutDialog> {
	public:

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline std::string ui_name = "gsc_about_dialog";


		/// Constructor, GtkBuilder needs this.
		GscAboutDialog(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);



	protected:

		// -------------------- Callbacks


		/// Callback - dialog response
		void on_response(int response_id) override;


		bool on_activate_link(const std::string& uri) override;

		// ---------- override virtual methods

		// we use .run(), so we don't need this
/*
		// by default, delete_event calls hide().
		bool on_delete_event(GdkEventAny* e) override
		{
			destroy(this);  // deletes this object and nullifies instance
			return true;  // event handled, don't call default virtual handler
		}
*/

};






#endif

/// @}
