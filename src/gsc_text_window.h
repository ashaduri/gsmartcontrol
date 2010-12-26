/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_TEXT_WINDOW_H
#define GSC_TEXT_WINDOW_H

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/textview.h>
#include <gdk/gdkkeysyms.h>  // GDK_Escape

#include "hz/debug.h"
#include "hz/fs_file.h"

#include "applib/app_ui_res_utils.h"



struct SmartctlOutputInstance { static const bool multi_instance = true; };



// use create() / destroy() with this class instead of new / delete!


// InstanceSwitch: e.g. supply 3 different classes to use 3 different single-instance windows.
template<class InstanceSwitch>
class GscTextWindow : public AppUIResWidget<GscTextWindow<InstanceSwitch>, InstanceSwitch::multi_instance> {

	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_text_window);

		// needed for glade, not inherited from parent because of templates
		typedef GscTextWindow<InstanceSwitch> self_type;


		// glade/gtkbuilder needs this constructor
		GscTextWindow(typename Gtk::Window::BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
				: AppUIResWidget<GscTextWindow<InstanceSwitch>, InstanceSwitch::multi_instance>(gtkcobj, ref_ui)
		{
			// Connect callbacks

			APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

			Gtk::Button* save_as_button = 0;
			APP_UI_RES_AUTO_CONNECT(save_as_button, clicked);

			Gtk::Button* close_window_button = 0;
			APP_UI_RES_AUTO_CONNECT(close_window_button, clicked);


			// Accelerators

			Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
			if (close_window_button) {
				close_window_button->add_accelerator("clicked", accel_group, GDK_Escape,
						Gdk::ModifierType(0), Gtk::AccelFlags(0));
			}


			// ---------------

			// show();  // better show later, after set_text().

			default_title_ = this->get_title();
		}


		virtual ~GscTextWindow()
		{ }


		void set_text(const Glib::ustring& title, const Glib::ustring& contents,
				bool save_visible = false, bool use_monospace = false)
		{
			this->set_title(title + " - " + default_title_);  // something - gsmartcontrol

			this->contents_ = contents;  // we save it to prevent its mangling through the widget

			Gtk::TextView* textview = this->template lookup_widget<Gtk::TextView*>("main_textview");
			if (textview) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text(contents);

				if (use_monospace) {
					Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
					tag->property_family() = "Monospace";
					buffer->apply_tag(tag, buffer->begin(), buffer->end());
				}
			}

			Gtk::Button* save_as_button = this->template lookup_widget<Gtk::Button*>("save_as_button");
			if (save_as_button) {
				if (save_visible) {
					save_as_button->set_sensitive(true);
					save_as_button->show();
				} else {
					save_as_button->hide();
					save_as_button->set_sensitive(false);  // disable accelerators
				}
			}
		}


		void set_save_filename(const std::string& filename)
		{
			save_filename_ = filename;
		}


	protected:


		// -------------------- Callbacks


		// ---------- override virtual methods

		// by default, delete_event calls hide().
		bool on_delete_event_before(GdkEventAny* e)
		{
			destroy(this);  // deletes this object and nullifies instance
			return true;  // event handled, don't call default virtual handler
		}


		// ---------- other callbacks

		void on_save_as_button_clicked()
		{
			static std::string last_dir;

			Gtk::FileChooserDialog dialog(*this, "Save Data As...",
					Gtk::FILE_CHOOSER_ACTION_SAVE);

			// Add response buttons the the dialog
			dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);

#if APP_GTKMM_CHECK_VERSION(2, 8, 0)
			dialog.set_do_overwrite_confirmation(true);  // since gtkmm 2.8
#endif

			if (!last_dir.empty())
				dialog.set_current_folder(last_dir);

			if (!save_filename_.empty())
				dialog.set_current_name(save_filename_);

			// Show the dialog and wait for a user response
			int result = dialog.run();  // the main cycle blocks here

			// Handle the response
			switch (result) {
				case Gtk::RESPONSE_ACCEPT:
				{
					last_dir = dialog.get_current_folder();  // safe for the future

					std::string file = dialog.get_filename();
					hz::File f(file);
					if (!f.put_contents(this->contents_)) {  // this will send to debug_ too.
						gui_show_error_dialog("Cannot save data to file", f.get_error_utf8(), this);
					}
					break;
				}

				case Gtk::RESPONSE_CANCEL: case Gtk::RESPONSE_DELETE_EVENT:
					// nothing, the dialog is closed already
					break;

				default:
					debug_out_error("app", DBG_FUNC_MSG << "Unknown dialog response code: " << result << ".\n");
					break;
			}
		}


		void on_close_window_button_clicked()
		{
			destroy(this);
		}



		Glib::ustring default_title_;
		Glib::ustring contents_;
		std::string save_filename_;


};






#endif
