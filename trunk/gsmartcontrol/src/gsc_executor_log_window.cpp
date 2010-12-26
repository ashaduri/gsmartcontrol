/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <sstream>
#include <cstddef>  // std::size_t
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/textview.h>
#include <gtkmm/treeview.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gdk/gdkkeysyms.h>  // GDK_Escape

#include "applib/app_gtkmm_utils.h"  // app_gtkmm_create_tree_view_column

#include "gsc_executor_log_window.h"
#include "gsc_init.h"  // app_get_debug_buffer_str()




GscExecutorLogWindow::GscExecutorLogWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
		: AppUIResWidget<GscExecutorLogWindow, false>(gtkcobj, ref_ui)
{
	// Connect callbacks

	APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	Gtk::Button* window_close_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_close_button, clicked);

	Gtk::Button* window_save_current_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_save_current_button, clicked);

	Gtk::Button* window_save_all_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_save_all_button, clicked);


	Gtk::Button* clear_command_list_button = 0;
	APP_UI_RES_AUTO_CONNECT(clear_command_list_button, clicked);



	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_close_button) {
		window_close_button->add_accelerator("clicked", accel_group, GDK_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


	// --------------- Make a treeview

	Gtk::TreeView* treeview = this->lookup_widget<Gtk::TreeView*>("command_list_treeview");
	if (treeview) {
		Gtk::TreeModelColumnRecord model_columns;
		int num_tree_cols = 0;

		// #, Command + parameters, [EntryPtr]

		model_columns.add(col_num);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_num, *treeview,
				"#", "# of executed command", true);  // sortable

		model_columns.add(col_command);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_command, *treeview,
				"Command", "Command with parameters", true);  // sortable

		model_columns.add(col_entry);


		// create a TreeModel (ListStore)
		list_store = Gtk::ListStore::create(model_columns);
		// list_store->set_sort_column(col_num, Gtk::SORT_DESCENDING);  // default sort
		treeview->set_model(list_store);


		selection = treeview->get_selection();
		selection->signal_changed().connect(sigc::mem_fun(*this,
				&self_type::on_tree_selection_changed) );

	}



	// Hide command text entry in win32.
	// Setting text on this entry segfaults under win32, (utf8 conversion doesn't
	// help, not that it should). Seems to be connected to non-english locale.
	// Surprisingly, the treeview column text still works.

	// The problem seems to have disappeared (new compiler/runtime?)
// #ifdef _WIN32
// 	Gtk::HBox* command_hbox = this->lookup_widget<Gtk::HBox*>("command_hbox");
// 	if (command_hbox)
// 		command_hbox->hide();
// #endif

	// ---------------

	// Connect to CmdexSync signal
	cmdex_sync_signal_execute_finish()->connect(sigc::mem_fun(*this, &self_type::on_command_output_received));


	// show();
}



void GscExecutorLogWindow::show_last()
{
	Gtk::TreeView* treeview = this->lookup_widget<Gtk::TreeView*>("command_list_treeview");

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
	Gtk::Button* window_save_current_button = this->lookup_widget<Gtk::Button*>("window_save_current_button");
	if (window_save_current_button)
		window_save_current_button->set_sensitive(false);

	Gtk::TextView* output_textview = this->lookup_widget<Gtk::TextView*>("output_textview");
	if (output_textview) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = output_textview->get_buffer();
		buffer->set_text("");
	}

	Gtk::Entry* command_entry = this->lookup_widget<Gtk::Entry*>("command_entry");
	if (command_entry)
		command_entry->set_text("");
}



void GscExecutorLogWindow::on_command_output_received(const CmdexSyncCommandInfo& info)
{
	CmdexSyncCommandInfoRefPtr entry = info.copy();
	entries.push_back(entry);

	// update tree model
	Gtk::TreeRow row = *(list_store->append());
	row[col_num] = entries.size();
	row[col_command] = entry->command + " " + entry->parameters;
	row[col_entry] = entry;

	// if visible, set the selection to it
	if (this->is_visible()) {
		Gtk::TreeView* treeview = this->lookup_widget<Gtk::TreeView*>("command_list_treeview");

		if (treeview) {
			selection->select(row);
			treeview->scroll_to_row(list_store->get_path(row));
		}
	}
}




void GscExecutorLogWindow::on_window_save_current_button_clicked()
{
	if (!selection->count_selected_rows())
		return;

	Gtk::TreeIter iter = selection->get_selected();
	CmdexSyncCommandInfoRefPtr entry = (*iter)[col_entry];


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

	// Show the dialog and wait for a user response
	int result = dialog.run();  // the main cycle blocks here

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			last_dir = dialog.get_current_folder();  // safe for the future

			std::string file = dialog.get_filename();
			hz::File f(file);
			if (!f.put_contents(entry->std_output)) {
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

	// Show the dialog and wait for a user response
	int result = dialog.run();  // the main cycle blocks here

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			last_dir = dialog.get_current_folder();  // safe for the future

			std::string file = dialog.get_filename();
			hz::File f(file);
			if (!f.put_contents(exss.str())) {
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

		CmdexSyncCommandInfoRefPtr entry = row[col_entry];

		Gtk::TextView* output_textview = this->lookup_widget<Gtk::TextView*>("output_textview");

		if (output_textview) {
			Glib::RefPtr<Gtk::TextBuffer> buffer = output_textview->get_buffer();
			if (buffer) {

				// Under win32, we can't execute smartctl under C locale. Smartctl
				// uses locale information only for thousands separator in User Capacity.
				// We can parse that, but we need to insert that text into a textarea
				// widget, which may error out on invalid utf8 char.
				// So, we convert the whole output to utf8, hoping that the result is ok.
				// Note that a separator converted to utf8 may be a different sequence
				// of chars, so we parse it as original charset, but insert into a textarea
				// as utf8.
				std::string buf_text = entry->std_output;
				#ifdef _WIN32
				try {
					buf_text = Glib::locale_to_utf8(buf_text);
				} catch (Glib::ConvertError& e) {
					buf_text = "";  // inserting invalid utf8 may trigger a segfault, so empty better.
				}
				#endif

				buffer->set_text(buf_text);

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

		// Hide in win32 because it is known to cause segfaults there (see above).
		// Not anymore...
// #ifndef _WIN32
		Gtk::Entry* command_entry = this->lookup_widget<Gtk::Entry*>("command_entry");
		if (command_entry) {
			std::string cmd_text = entry->command + " " + entry->parameters;
			command_entry->set_text(cmd_text);
		}
// #endif

		Gtk::Button* window_save_current_button = this->lookup_widget<Gtk::Button*>("window_save_current_button");
		if (window_save_current_button)
			window_save_current_button->set_sensitive(true);
	}

}






