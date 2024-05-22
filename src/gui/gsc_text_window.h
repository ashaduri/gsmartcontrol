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

#ifndef GSC_TEXT_WINDOW_H
#define GSC_TEXT_WINDOW_H

#include "local_glibmm.h"
#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape
#include <memory>
#include <variant>

#include "hz/debug.h"
#include "hz/fs.h"
#include "rconfig/rconfig.h"

#include "applib/app_builder_widget.h"
#include "applib/app_gtkmm_tools.h"



/// GscTextWindow InstanceSwitch parameter for smartctl output instance.
struct SmartctlOutputInstance {
	static constexpr bool multi_instance = true;
};



/// A generic text-displaying window.
/// Use create() / destroy() with this class instead of new / delete!
/// \tparam InstanceSwitch e.g. supply 3 different classes to use 3 different single-instance windows.
template<class InstanceSwitch>
class GscTextWindow : public AppBuilderWidget<GscTextWindow<InstanceSwitch>, InstanceSwitch::multi_instance> {
	public:

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline const std::string_view ui_name = "gsc_text_window";


		/// Constructor, GtkBuilder needs this.
		GscTextWindow(typename Gtk::Window::BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
				: AppBuilderWidget<GscTextWindow<InstanceSwitch>, InstanceSwitch::multi_instance>(gtkcobj, std::move(ui))
		{
			// Connect callbacks

			Gtk::Button* save_as_button = nullptr;
			APP_BUILDER_AUTO_CONNECT(save_as_button, clicked);

			Gtk::Button* close_window_button = nullptr;
			APP_BUILDER_AUTO_CONNECT(close_window_button, clicked);


			// Accelerators

			Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
			if (close_window_button) {
				close_window_button->add_accelerator("clicked", accel_group, GDK_KEY_Escape,
						Gdk::ModifierType(0), Gtk::AccelFlags(0));
			}

			// show();  // better show later, after set_text().

			default_title_ = this->get_title();
		}


		void set_text_from_command(const Glib::ustring& title, const std::string& contents)
		{
			this->contents_ = contents;  // we save it to prevent its mangling through the widget
			this->set_text_helper(title, true, true);
		}


		void set_text(const Glib::ustring& title, const Glib::ustring& contents,
				bool save_visible = false, bool use_monospace = false)
		{
			this->contents_ = contents;
			this->set_text_helper(title, save_visible, use_monospace);
		}


		/// Set the text to display
		void set_text_helper(const Glib::ustring& title,
				bool save_visible = false, bool use_monospace = false)
		{
			this->set_title(title + " - " + default_title_);  // something - gsmartcontrol

			Gtk::TextView* textview = this->template lookup_widget<Gtk::TextView*>("main_textview");
			if (textview) {
				Glib::RefPtr<Gtk::TextBuffer> buffer = textview->get_buffer();

				if (std::holds_alternative<std::string>(contents_)) {
					buffer->set_text(app_make_valid_utf8_from_command_output(std::get<std::string>(contents_)));
				} else {
					buffer->set_text(std::get<Glib::ustring>(contents_));
				}

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
		bool on_delete_event([[maybe_unused]] GdkEventAny* e) override
		{
			on_close_window_button_clicked();
			return true;  // event handled
		}


		// ---------- other callbacks

		/// Button click callback
		void on_save_as_button_clicked()
		{
			static std::string last_dir;
			if (last_dir.empty()) {
				last_dir = rconfig::get_data<std::string>("gui/drive_data_open_save_dir");
			}
			int result = 0;

			Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
			specific_filter->set_name(_("JSON and Text Files"));
			specific_filter->add_pattern("*.json");
			specific_filter->add_pattern("*.txt");

			Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
			all_filter->set_name(_("All Files"));
			all_filter->add_pattern("*");

#if GTK_CHECK_VERSION(3, 20, 0)
			std::unique_ptr<GtkFileChooserNative, decltype(&g_object_unref)> dialog(gtk_file_chooser_native_new(
					_("Save Data As..."), this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr),
					&g_object_unref);

			gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), TRUE);

			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

			if (!last_dir.empty())
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());

			if (!save_filename_.empty())
				gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog.get()), save_filename_.c_str());

			result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
			Gtk::FileChooserDialog dialog(*this, _("Save Data As..."),
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
					hz::fs::path file;
#if GTK_CHECK_VERSION(3, 20, 0)
					file = hz::fs_path_from_string(app_string_from_gchar(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog.get()))));
					last_dir = hz::fs_path_to_string(file.parent_path());
#else
					file = hz::fs_path_from_string(dialog.get_filename());  // in fs encoding
					last_dir = dialog.get_current_folder();  // save for the future
#endif
					rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

					if (file.extension() != ".json" && file.extension() != ".txt") {
						file += ".json";
					}

					std::string text;
					if (std::holds_alternative<std::string>(contents_)) {
						text = std::get<std::string>(contents_);
					} else {
						text = std::get<Glib::ustring>(contents_);
					}
					auto ec = hz::fs_file_put_contents(file, text);
					if (ec) {
						gui_show_error_dialog(_("Cannot save data to file"), ec.message(), this);
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
			this->destroy_instance();
		}


	private:

		Glib::ustring default_title_;  ///< Window title

		/// The text to display
		std::variant<
			std::string,  // command output
			Glib::ustring  // utf-8 text
		> contents_;

		std::string save_filename_;  ///< Default filename for Save As

};






#endif

/// @}
