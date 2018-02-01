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

// TODO Remove this in gtkmm4.
#include "local_glibmm.h"

#include <sstream>
#include <cstddef>  // std::size_t
#include <gtkmm.h>
#include <gdk/gdk.h>  // GDK_KEY_Escape

#include "applib/app_gtkmm_utils.h"  // app_gtkmm_create_tree_view_column
#include "applib/app_gtkmm_features.h"
#include "hz/scoped_ptr.h"
#include "hz/fs.h"
#include "rconfig/config.h"

#include "gsc_executor_log_window.h"
#include "gsc_init.h"  // app_get_debug_buffer_str()




GscExecutorLogWindow::GscExecutorLogWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
		: AppBuilderWidget<GscExecutorLogWindow, false>(gtkcobj, std::move(ui))
{
	// Connect callbacks

	Gtk::Button* window_close_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(window_close_button, clicked);

	Gtk::Button* window_save_current_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(window_save_current_button, clicked);

	Gtk::Button* window_save_all_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(window_save_all_button, clicked);


	Gtk::Button* clear_command_list_button = nullptr;
	APP_BUILDER_AUTO_CONNECT(clear_command_list_button, clicked);



	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_close_button) {
		window_close_button->add_accelerator("clicked", accel_group, GDK_KEY_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


	// --------------- Make a treeview

	auto* treeview = this->lookup_widget<Gtk::TreeView*>("command_list_treeview");
	if (treeview) {
		Gtk::TreeModelColumnRecord model_columns;

		// #, Command + parameters, [EntryPtr]

		model_columns.add(col_num);
		app_gtkmm_create_tree_view_column(col_num, *treeview,
				"#", "# of executed command", true);  // sortable

		model_columns.add(col_command);
		app_gtkmm_create_tree_view_column(col_command, *treeview,
				"Command", "Command with parameters", true);  // sortable

		model_columns.add(col_entry);


		// create a TreeModel (ListStore)
		list_store = Gtk::ListStore::create(model_columns);
		// list_store->set_sort_column(col_num, Gtk::SORT_DESCENDING);  // default sort
		treeview->set_model(list_store);


		selection = treeview->get_selection();
		selection->signal_changed().connect(sigc::mem_fun(*this,
				&GscExecutorLogWindow::on_tree_selection_changed) );

	}



	// Hide command text entry in win32.
	// Setting text on this entry segfaults under win32, (utf8 conversion doesn't
	// help, not that it should). Seems to be connected to non-english locale.
	// Surprisingly, the treeview column text still works.

	// The problem seems to have disappeared (new compiler/runtime?)
// #ifdef _WIN32
// 	Gtk::Box* command_hbox = this->lookup_widget<Gtk::Box*>("command_hbox");
// 	if (command_hbox)
// 		command_hbox->hide();
// #endif

	// ---------------

	// Connect to CmdexSync signal
	cmdex_sync_signal_execute_finish().connect(sigc::mem_fun(*this,
			&GscExecutorLogWindow::on_command_output_received));

	// show();
}



void GscExecutorLogWindow::show_last()
{
	auto* treeview = this->lookup_widget<Gtk::TreeView*>("command_list_treeview");

	if (treeview && !list_store->children().empty()) {
// 		Gtk::TreeRow row = *(list_store->children().rbegin());  // this causes invalid read error in valgrind
		Gtk::TreeRow row = *(--(list_store->children().end()));
		selection->select(row);
		// you would think that scroll_to_row would accept a TreeRow for a change (shock!)
		treeview->scroll_to_row(list_store->get_path(row));
	}

	show();
}



void GscExecutorLogWindow::clear_view_widgets()
{
	auto* window_save_current_button = this->lookup_widget<Gtk::Button*>("window_save_current_button");
	if (window_save_current_button)
		window_save_current_button->set_sensitive(false);

	auto* output_textview = this->lookup_widget<Gtk::TextView*>("output_textview");
	if (output_textview) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = output_textview->get_buffer();
		buffer->set_text("");
	}

	auto* command_entry = this->lookup_widget<Gtk::Entry*>("command_entry");
	if (command_entry)
		command_entry->set_text("");
}



void GscExecutorLogWindow::on_command_output_received(const CmdexSyncCommandInfo& info)
{
	auto entry = std::make_shared<CmdexSyncCommandInfo>(info);
	entries.push_back(entry);

	// update tree model
	Gtk::TreeRow row = *(list_store->append());
	row[col_num] = entries.size();
	row[col_command] = info.command + " " + info.parameters;
	row[col_entry] = entry;

	// if visible, set the selection to it
	if (auto* treeview = this->lookup_widget<Gtk::TreeView*>("command_list_treeview")) {
		selection->select(row);
		treeview->scroll_to_row(list_store->get_path(row));
	}
}



bool GscExecutorLogWindow::on_delete_event([[maybe_unused]] GdkEventAny* e)
{
	on_window_close_button_clicked();
	return true;  // event handled
}



void GscExecutorLogWindow::on_window_close_button_clicked()
{
	this->hide();  // hide only, don't destroy
}



void GscExecutorLogWindow::on_window_save_current_button_clicked()
{
	if (!selection->count_selected_rows())
		return;

	Gtk::TreeIter iter = selection->get_selected();
	std::shared_ptr<CmdexSyncCommandInfo> entry = (*iter)[col_entry];

	static std::string last_dir;
	if (last_dir.empty()) {
		last_dir = rconfig::get_data<std::string>("gui/drive_data_open_save_dir");
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
			"Save Data As...", this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr), g_object_unref);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), true);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

	if (!last_dir.empty())
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog.get()), ".txt");

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

	dialog.set_current_name(".txt");

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
			last_dir = hz::fs::u8path(file).parent_path().u8string();
#else
			file = dialog.get_filename();  // in fs encoding
			last_dir = dialog.get_current_folder();  // save for the future
#endif
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

			if (file.rfind(".txt") != (file.size() - std::strlen(".txt"))) {
				file += ".txt";
			}

			auto ec = hz::fs_file_put_contents(hz::fs::u8path(file), entry->std_output);
			if (ec) {
				gui_show_error_dialog("Cannot save data to file", ec.message(), this);
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




void GscExecutorLogWindow::on_window_save_all_button_clicked()
{
	// complete libdebug output + execution logs

	std::ostringstream exss;

	exss << "\n------------------------- LIBDEBUG LOG -------------------------\n\n\n";
	exss << app_get_debug_buffer_str() << "\n\n\n";

	exss << "\n\n\n------------------------- EXECUTION LOG -------------------------\n\n\n";

	for (std::size_t i = 0; i < entries.size(); ++i) {
		exss << "\n\n\n------------------------- EXECUTED COMMAND " << (i+1) << " -------------------------\n\n";
		exss << "\n---------------" << "Command" << "---------------\n";
		exss << entries[i]->command << "\n";
		exss << "\n---------------" << "Parameters" << "---------------\n";
		exss << entries[i]->parameters << "\n";
		exss << "\n---------------" << "STDOUT" << "---------------\n";
		exss << entries[i]->std_output << "\n\n";
		exss << "\n---------------" << "STDERR" << "---------------\n";
		exss << entries[i]->std_error << "\n\n";
		exss << "\n---------------" << "Error Message" << "---------------\n";
		exss << entries[i]->error_msg << "\n\n";
	}


	static std::string last_dir;
	if (last_dir.empty()) {
		last_dir = rconfig::get_data<std::string>("gui/drive_data_open_save_dir");
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
			"Save Data As...", this->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr), g_object_unref);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog.get()), true);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

	if (!last_dir.empty())
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog.get()), ".txt");

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

	dialog.set_current_name(".txt");

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
			last_dir = hz::fs::u8path(file).parent_path().u8string();
#else
			file = dialog.get_filename();  // in fs encoding
			last_dir = dialog.get_current_folder();  // save for the future
#endif
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);

			if (file.rfind(".txt") != (file.size() - std::strlen(".txt"))) {
				file += ".txt";
			}

			auto ec = hz::fs_file_put_contents(hz::fs::u8path(file), exss.str());
			if (ec) {
				gui_show_error_dialog("Cannot save data to file", ec.message(), this);
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



void GscExecutorLogWindow::on_clear_command_list_button_clicked()
{
	entries.clear();
	list_store->clear();  // this will unselect & clear widgets too.
}



void GscExecutorLogWindow::on_tree_selection_changed()
{
	this->clear_view_widgets();

	if (selection->count_selected_rows()) {
		Gtk::TreeIter iter = selection->get_selected();
		Gtk::TreeRow row = *iter;

		std::shared_ptr<CmdexSyncCommandInfo> entry = row[col_entry];

		if (auto* output_textview = this->lookup_widget<Gtk::TextView*>("output_textview")) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = output_textview->get_buffer();
			if (buffer) {
				buffer->set_text(app_output_make_valid(entry->std_output));

				Glib::RefPtr<Gtk::TextTag> tag;
				Glib::RefPtr<Gtk::TextTagTable> table = buffer->get_tag_table();
				if (table)
					tag = table->lookup("font");
				if (!tag)
					tag = buffer->create_tag("font");

				tag->property_family() = "Monospace";
				buffer->apply_tag(tag, buffer->begin(), buffer->end());
			}
		}

		if (auto command_entry = this->lookup_widget<Gtk::Entry*>("command_entry")) {
			std::string cmd_text = entry->command + " " + entry->parameters;
			command_entry->set_text(app_output_make_valid(cmd_text));
		}

		if (auto window_save_current_button = this->lookup_widget<Gtk::Button*>("window_save_current_button"))
			window_save_current_button->set_sensitive(true);
	}

}







/// @}
