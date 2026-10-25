/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#include <glibmm.h>
#include <gtkmm.h>
#include <memory>
#include <optional>
#include <string>

#include "hz/debug.h"
#include "hz/fs.h"
#include "rconfig/rconfig.h"

#include "applib/app_gtkmm_tools.h"  // app_string_from_gchar
#include "applib/gui_utils.h"  // gui_show_error_dialog
#include "applib/smartctl_output_export.h"

#include "gsc_save_output_dialog.h"



void gsc_show_save_smartctl_output_dialog(Gtk::Window* parent, const std::string& smartctl_output,
		const std::string& default_filename, SmartctlOutputFormat preferred_format)
{
	static std::string last_dir;
	if (last_dir.empty()) {
		last_dir = rconfig::get_data<std::string>("gui/drive_data_open_save_dir");
	}

	const SmartctlOutputVariants variants = smartctl_output_get_variants(smartctl_output);
	const bool has_json = variants.has(SmartctlOutputFormat::Json);
	const bool has_text = variants.has(SmartctlOutputFormat::Text);
	if (!variants.has(preferred_format)) {
		preferred_format = has_json ? SmartctlOutputFormat::Json : SmartctlOutputFormat::Text;
	}

	std::string filename;
	if (!default_filename.empty()) {
		filename = hz::fs_path_to_string(smartctl_output_set_file_extension(
				hz::fs_path_from_string(default_filename), preferred_format));
	}

	// The combined filter lets the user choose the format by typing the extension.
	Glib::RefPtr<Gtk::FileFilter> specific_filter;
	if (has_json && has_text) {
		specific_filter = Gtk::FileFilter::create();
		specific_filter->set_name(_("JSON and Text Files"));
		specific_filter->add_pattern("*.json");
		specific_filter->add_pattern("*.txt");
	}

	Glib::RefPtr<Gtk::FileFilter> json_filter;
	if (has_json) {
		json_filter = Gtk::FileFilter::create();
		json_filter->set_name(_("JSON Files"));
		json_filter->add_pattern("*.json");
	}

	Glib::RefPtr<Gtk::FileFilter> txt_filter;
	if (has_text) {
		txt_filter = Gtk::FileFilter::create();
		txt_filter->set_name(_("Text Files"));
		txt_filter->add_pattern("*.txt");
	}

	Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
	all_filter->set_name(_("All Files"));
	all_filter->add_pattern("*");

	int result = 0;
	hz::fs::path file;
	std::optional<SmartctlOutputFormat> filter_format;

	// Get the format of the selected filter, if it's format-specific
	auto get_filter_format = [&](GtkFileFilter* selected) -> std::optional<SmartctlOutputFormat> {
		if (selected != nullptr) {
			if (json_filter && selected == json_filter->gobj()) {
				return SmartctlOutputFormat::Json;
			}
			if (txt_filter && selected == txt_filter->gobj()) {
				return SmartctlOutputFormat::Text;
			}
		}
		return std::nullopt;
	};

#if GTK_CHECK_VERSION(3, 20, 0)
	std::unique_ptr<GtkFileChooserNative, decltype(&g_object_unref)> dialog(gtk_file_chooser_native_new(
			_("Save Data As..."), (parent ? parent->gobj() : nullptr), GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr),
			&g_object_unref);
	auto* chooser = GTK_FILE_CHOOSER(dialog.get());

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	for (const auto& filter : {specific_filter, json_filter, txt_filter, all_filter}) {
		if (filter) {
			gtk_file_chooser_add_filter(chooser, filter->gobj());
		}
	}

	if (!last_dir.empty())
		gtk_file_chooser_set_current_folder(chooser, last_dir.c_str());

	if (!filename.empty())
		gtk_file_chooser_set_current_name(chooser, filename.c_str());

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

	if (result == Gtk::RESPONSE_ACCEPT) {
		file = hz::fs_path_from_string(app_string_from_gchar(gtk_file_chooser_get_filename(chooser)));
		last_dir = hz::fs_path_to_string(file.parent_path());
		filter_format = get_filter_format(gtk_file_chooser_get_filter(chooser));
	}

#else
	Gtk::FileChooserDialog dialog(_("Save Data As..."), Gtk::FILE_CHOOSER_ACTION_SAVE);
	if (parent) {
		dialog.set_transient_for(*parent);
	}

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);

	dialog.set_do_overwrite_confirmation(true);

	for (const auto& filter : {specific_filter, json_filter, txt_filter, all_filter}) {
		if (filter) {
			dialog.add_filter(filter);
		}
	}

	if (!last_dir.empty())
		dialog.set_current_folder(last_dir);

	if (!filename.empty())
		dialog.set_current_name(filename);

	// Show the dialog and wait for a user response
	result = dialog.run();  // the main cycle blocks here

	if (result == Gtk::RESPONSE_ACCEPT) {
		file = hz::fs_path_from_string(dialog.get_filename());  // in fs encoding
		last_dir = dialog.get_current_folder();  // save for the future
		filter_format = get_filter_format(dialog.get_filter() ? dialog.get_filter()->gobj() : nullptr);
	}
#endif

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

			const auto [target_file, format] = smartctl_output_resolve_save_target(
					file, filter_format, preferred_format, variants);

			// The dialog asked about overwriting the chosen file only, not the one with the adjusted extension.
			std::error_code exists_ec;
			if (target_file != file && hz::fs::exists(target_file, exists_ec)) {
				Gtk::MessageDialog confirm_dialog(
						Glib::ustring::compose(_("File \"%1\" already exists. Overwrite it?"), hz::fs_path_to_string(target_file.filename())),
						false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
				if (parent) {
					confirm_dialog.set_transient_for(*parent);
				}
				if (confirm_dialog.run() != Gtk::RESPONSE_YES) {
					break;
				}
			}

			const std::error_code ec = hz::fs_file_put_contents(target_file, variants.get(format));
			if (ec) {
				gui_show_error_dialog(_("Cannot save SMART data to file"), ec.message(), parent);
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



/// @}
