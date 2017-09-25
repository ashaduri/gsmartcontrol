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

#ifndef GSC_TEXT_WINDOW_H
#define GSC_TEXT_WINDOW_H

#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape

#include "hz/debug.h"
#include "hz/fs_file.h"
#include "hz/scoped_ptr.h"
#include "rconfig/rconfig_mini.h"

#include "applib/app_gtkmm_features.h"
#include "applib/app_ui_res_utils.h"
#include "applib/app_gtkmm_utils.h"



/// GscTextWindow InstanceSwitch parameter for smartctl output instance.
struct SmartctlOutputInstance { static const bool multi_instance = true; };



/// A generic text-displaying window.
/// Use create() / destroy() with this class instead of new / delete!
/// \tparam InstanceSwitch e.g. supply 3 different classes to use 3 different single-instance windows.
template<class InstanceSwitch>
class GscTextWindow : public AppUIResWidget<GscTextWindow<InstanceSwitch>, InstanceSwitch::multi_instance> {
	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(gsc_text_window);

		/// Self type, needed for glade, not inherited from parent because of templates
		typedef GscTextWindow<InstanceSwitch> self_type;


		/// Constructor, gtkbuilder/glade needs this.
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
				close_window_button->add_accelerator("clicked", accel_group, GDK_KEY_Escape,
						Gdk::ModifierType(0), Gtk::AccelFlags(0));
			}

			// show();  // better show later, after set_text().

			default_title_ = this->get_title();
		}


		/// Virtual destructor
		virtual ~GscTextWindow()
		{ }


		/// Set the text to display
		void set_text(const Glib::ustring& title, const Glib::ustring& contents,
				bool save_visible = false, bool use_monospace = false)
		{
			this->set_title(title + " - " + default_title_);  // something - gsmartcontrol

			this->contents_ = contents;  // we save it to prevent its mangling through the widget

			Gtk::TextView* textview = this->template lookup_widget<Gtk::TextView*>("main_textview");
			if (textview) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();
				buffer->set_text(app_output_make_valid(contents));

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


		/// Set the default file name to be shown on Save As
		void set_save_filename(const std::string& filename)
		{
			save_filename_ = filename;
		}


	protected:


		// -------------------- callbacks


		// ---------- overriden virtual methods

		/// Destroy this object on delete event (by default it calls hide()).
		/// Reimplemented from Gtk::Window.
		bool on_delete_event_before(GdkEventAny* e)
		{
			this->destroy(this);  // deletes this object and nullifies instance
			return true;  // event handled, don't call default virtual handler
		}


		// ---------- other callbacks

		/// Button click callback
		void on_save_as_button_clicked()
		{
			static std::string last_dir;
			if (last_dir.empty()) {
				rconfig::get_data("gui/drive_data_open_save_dir", last_dir);
			}
			int result = 0;

			Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
			specific_filter->set_name("Text Files");
			specific_filter->add_pattern("*.txt");

			Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
			all_filter->set_name("All Files");
			all_filter->add_pattern("*");

#if GTK_CHECK_VERSION(3, 20, 0)
			hz::scoped_ptr<GtkFileChooserNative> dialog(gtk_file_chooser_native_new(
					"Save Data As...", this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, NULL, NULL), g_object_unref);

			gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), true);

			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

			if (!last_dir.empty())
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());

			if (!save_filename_.empty())
				gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog.get()), save_filename_.c_str());

			result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
			Gtk::FileChooserDialog dialog(*this, "Save Data As...",
					Gtk::FILE_CHOOSER_ACTION_SAVE);

			// Add response buttons the the dialog
			dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);

			dialog.set_do_overwrite_confirmation(true);

			dialog.add_filter(specific_filter);
			dialog.add_filter(all_filter);

			if (!last_dir.empty())
				dialog.set_current_folder(last_dir);

			if (!save_filename_.empty())
				dialog.set_current_name(save_filename_);

			// Show the dialog and wait for a user response
			result = dialog.run();  // the main cycle blocks here
#endif

			// Handle the response
			switch (result) {
				case Gtk::RESPONSE_ACCEPT:
				{
					std::string file;
#if GTK_CHECK_VERSION(3, 20, 0)
					file = app_ustring_from_gchar(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog.get())));
					last_dir = hz::path_get_dirname(file);
#else
					file = dialog.get_filename();  // in fs encoding
					last_dir = dialog.get_current_folder();  // save for the future
#endif
					rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

					if (file.rfind(".txt") != (file.size() - std::strlen(".txt"))) {
						file += ".txt";
					}

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


		/// Button click callback
		void on_close_window_button_clicked()
		{
			this->destroy(this);
		}


	private:

		Glib::ustring default_title_;  ///< Window title
		Glib::ustring contents_;  ///< The text to display
		std::string save_filename_;  ///< Default filename for Save As


};






#endif

/// @}
