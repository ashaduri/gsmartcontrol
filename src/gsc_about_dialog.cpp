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

#include <vector>

#include "hz/debug.h"
#include "hz/string_algo.h"  // hz::string_*
#include "hz/launch_url.h"
#include "hz/data_file.h"

#include "gsc_about_dialog.h"

#include "build_config.h"  // VERSION



// GtkBuilder needs this constructor
GscAboutDialog::GscAboutDialog(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
		: AppBuilderWidget<GscAboutDialog, false, Gtk::AboutDialog>(gtkcobj, std::move(ui))
{
	// Connect callbacks

	// Note: The dialogs have ESC accelerator attached by default.

	set_version(BuildEnv::package_version());

	// set these properties here (after setting hooks) to make the links work.
	set_website("https://gsmartcontrol.shaduri.dev");

//	set_license_type(Gtk::LICENSE_GPL_3_0_ONLY);  // this overrides set_license()
//	set_license(hz::data_file_get_contents("doc", "LICENSE.txt", 1*1024*1024));  // 1M

	// spammers go away
	set_copyright(Glib::ustring::compose("Copyright (C) %1",
			"2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>"));

	// set_authors({"Alexander Shaduri <ashaduri@gmail.com>"});
	// set_documenters({"Alexander Shaduri <ashaduri@gmail.com>"});
	// set_translator_credits({"Alexander Shaduri <ashaduri@gmail.com>"});

	// std::string authors_str = hz::data_file_get_contents("doc", "AUTHORS.txt", 1*1024*1024);  // 1M
	// hz::string_any_to_unix(authors_str);

	// std::vector<Glib::ustring> authors;
	// hz::string_split(authors_str, '\n', authors, true);
	//
	// for (auto& author : authors) {
	// 	std::string s = author;
	// 	hz::string_replace(s, " '@' ", "@");  // despammer
	// 	hz::string_replace(s, " 'at' ", "@");  // despammer
	// 	author = s;
	// }
	// set_authors(authors);

	// set_documenters(authors);

	// std::string translators_str = hz::data_file_get_contents("doc", "TRANSLATORS.txt", 10*1024*1024);  // 10M
	// set_translator_credits(translators_str);

// 	run();  // don't use run - it's difficult to exit it manually.
// 	show();  // shown by the caller to enable setting the parent window.
}



void GscAboutDialog::on_response(int response_id)
{
	debug_out_info("app", DBG_FUNC_MSG << "Response ID: " << response_id << "\n");

	if (response_id == Gtk::RESPONSE_NONE || response_id == Gtk::RESPONSE_DELETE_EVENT
			|| response_id == Gtk::RESPONSE_CANCEL || response_id == Gtk::RESPONSE_CLOSE) {
		debug_out_info("app", DBG_FUNC_MSG << "Closing the dialog.\n");
		destroy_instance();  // close the window and delete the object
	}
}



bool GscAboutDialog::on_activate_link(const std::string& uri)
{
	// The default handler, gtk_show_uri_on_window() doesn't work with mailto: URIs in Windows.
	// Our handler does.
	return hz::launch_url(GTK_WINDOW(this->gobj()), uri).empty();
}






/// @}
