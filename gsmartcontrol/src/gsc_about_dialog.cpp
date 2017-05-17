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

#include "hz/hz_config.h"  // VERSION

#include <vector>

#include "hz/debug.h"
#include "hz/string_algo.h"  // hz::string_*
#include "applib/app_gtkmm_features.h"
#include "applib/gui_utils.h"  // gui_show_error_dialog

#include "gsc_about_dialog.h"



// glade/gtkbuilder needs this constructor
GscAboutDialog::GscAboutDialog(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
		: AppUIResWidget<GscAboutDialog, false, Gtk::AboutDialog>(gtkcobj, ref_ui)
{
	// Connect callbacks

	// APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	APP_GTKMM_CONNECT_VIRTUAL(response);

	// Note: The dialogs have ESC accelerator attached by default.

	// Note: AboutDialog changed "name" property to "program-name" in gtk 2.12.
	// We don't set either, but rely on Glib::set_application_name() during init, which
	// works with both older and newer versions.

	set_version(VERSION);

	// set these properties here (after setting hooks) to make the links work.
	set_website("http://gsmartcontrol.sourceforge.net/");

	set_license(LicenseTextResData().get_string());

	// spammers go away
	set_copyright("Copyright (C) 2008 - 2017  Alexander Shaduri " "<ashaduri" "" "@" "" "" "gmail.com>");


	std::string authors_str = AuthorsTextResData().get_string();
	hz::string_any_to_unix(authors_str);

	std::vector<Glib::ustring> authors;
	hz::string_split(authors_str, '\n', authors, true);

	for (std::vector<Glib::ustring>::iterator iter = authors.begin(); iter != authors.end(); ++iter) {
		std::string s = *iter;
		hz::string_replace(s, " '@' ", "@");  // despammer
		hz::string_replace(s, " 'at' ", "@");  // despammer
		*iter = s;
	}

	set_authors(authors);

	set_documenters(authors);


// 	run();  // don't use run - it's difficult to exit it manually.
// 	show();  // shown by the caller to enable setting the parent window.
}



void GscAboutDialog::on_response_before(int response_id)
{
	debug_out_info("app", DBG_FUNC_MSG << "Response ID: " << response_id << "\n");

	if (response_id == Gtk::RESPONSE_NONE || response_id == Gtk::RESPONSE_DELETE_EVENT
			|| response_id == Gtk::RESPONSE_CANCEL || response_id == Gtk::RESPONSE_CLOSE) {
		debug_out_info("app", DBG_FUNC_MSG << "Closing the dialog.\n");
		destroy(this);  // close the window and delete the object
	}
}






/// @}
