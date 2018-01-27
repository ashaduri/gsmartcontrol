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

#include "config.h"  // VERSION

#include <vector>

#include "hz/debug.h"
#include "hz/string_algo.h"  // hz::string_*
#include "hz/launch_url.h"
#include "applib/app_gtkmm_features.h"

#include "gsc_about_dialog.h"



// GtkBuilder needs this constructor
GscAboutDialog::GscAboutDialog(BaseObjectType* gtkcobj, const Glib::RefPtr<Gtk::Builder>& ref_ui)
		: AppUIResWidget<GscAboutDialog, false, Gtk::AboutDialog>(gtkcobj, ref_ui)
{
	// Connect callbacks

	// Note: The dialogs have ESC accelerator attached by default.

	// APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	APP_GTKMM_CONNECT_VIRTUAL(response);

	APP_GTKMM_CONNECT_VIRTUAL(activate_link);

	set_version(VERSION);

	// set these properties here (after setting hooks) to make the links work.
	set_website("https://gsmartcontrol.sourceforge.io/");

	set_license(LicenseTextResData().get_string());

	// This overrides set_license(), so don't do it.
// 	set_license_type(Gtk::LICENSE_GPL_3_0_ONLY);

	// spammers go away
	set_copyright("Copyright (C) 2008 - 2018  Alexander Shaduri " "<ashaduri" "" "@" "" "" "gmail.com>");

	std::string authors_str = AuthorsTextResData().get_string();
	hz::string_any_to_unix(authors_str);

	std::vector<Glib::ustring> authors;
	hz::string_split(authors_str, '\n', authors, true);

	for (auto& author : authors) {
		std::string s = author;
		hz::string_replace(s, " '@' ", "@");  // despammer
		hz::string_replace(s, " 'at' ", "@");  // despammer
		author = s;
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
		destroy();  // close the window and delete the object
	}
}



bool GscAboutDialog::on_activate_link_before(const std::string& uri)
{
	// The default handler - gtk_show_uri_on_window() doesn't work with mailto: URIs in Windows.
	// Our handler does.
	return hz::launch_url(GTK_WINDOW(this->gobj()), uri).empty();
}






/// @}
