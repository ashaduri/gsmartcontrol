/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <list>  // list is more header-lightweight than vector

#include "hz/debug.h"
#include "hz/string_algo.h"  // hz::string_*
#include "hz/launch_url.h"  // hz::launch_url

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


	set_url_hook(sigc::mem_fun(*this, &GscAboutDialog::on_activate_url));
	set_email_hook(sigc::mem_fun(*this, &GscAboutDialog::on_activate_email));


	// Note: AboutDialog changed "name" property to "program-name" in gtk 2.12.
	// We don't set either, but rely on Glib::set_application_name() during init, which
	// works with both older and newer versions.


	// set these properties here (after setting hooks) to make the links work.
	set_website("http://gsmartcontrol.berlios.de/");

	set_license(LicenseTextResData().get_string());

	// spammers go away
	set_copyright("Copyright (C) 2008  Alexander Shaduri " "<ashaduri" "" "@" "" "" "gmail.com>");


	std::string authors_str = AuthorsTextResData().get_string();
	hz::string_any_to_unix(authors_str);

	std::list<Glib::ustring> authors;
	hz::string_split(authors_str, '\n', authors, true);

	for (std::list<Glib::ustring>::iterator iter = authors.begin(); iter != authors.end(); ++iter) {
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



void GscAboutDialog::on_activate_url(Gtk::AboutDialog& about_dialog, const Glib::ustring& link)
{
	std::string error_msg = hz::launch_url(link, this->get_screen()->gobj());
	if (!error_msg.empty()) {
		gui_show_error_dialog("Cannot open URL", error_msg, &about_dialog);
	}
}



void GscAboutDialog::on_activate_email(Gtk::AboutDialog& about_dialog, const Glib::ustring& link)
{
	std::string email = link;
	// about dialog passes us the link without mailto:, so add it.
	if (email.compare(0, 7, "mailto:") != 0)
		email = "mailto:" + email;

	std::string error_msg = hz::launch_url(email, this->get_screen()->gobj());
	if (!error_msg.empty()) {
		gui_show_error_dialog("Cannot open an email client", error_msg, &about_dialog);
	}
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






