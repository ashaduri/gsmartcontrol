/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <vector>
#include <gtkmm/button.h>
#include <gtkmm/textview.h>
#include <gtkmm/treeview.h>
#include <gdk/gdkkeysyms.h>  // GDK_Escape

#include "hz/string_algo.h"
#include "applib/app_gtkmm_utils.h"  // app_gtkmm_create_tree_view_column
#include "applib/app_pcrecpp.h"

#include "gsc_help_window.h"



HZ_RES_DATA_INIT_NAMED(README_txt, "README.txt", ReadmeTextResData);



GscHelpWindow::GscHelpWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
		: AppUIResWidget<GscHelpWindow, false>(gtkcobj, ref_ui), selection_callback_enabled(true)
{
	// Connect callbacks

	APP_GTKMM_CONNECT_VIRTUAL(delete_event);  // make sure the event handler is called

	Gtk::Button* window_close_button = 0;
	APP_UI_RES_AUTO_CONNECT(window_close_button, clicked);


	// Accelerators

	Glib::RefPtr<Gtk::AccelGroup> accel_group = this->get_accel_group();
	if (window_close_button) {
		window_close_button->add_accelerator("clicked", accel_group, GDK_Escape,
				Gdk::ModifierType(0), Gtk::AccelFlags(0));
	}


	// --------------- Make a treeview

	Gtk::TreeView* treeview = this->lookup_widget<Gtk::TreeView*>("topics_treeview");
	if (treeview) {
		Gtk::TreeModelColumnRecord model_columns;
		int num_tree_cols = 0;

		// Topic
		model_columns.add(col_topic);
		num_tree_cols = app_gtkmm_create_tree_view_column(col_topic, *treeview, "Topic", "Topic");

		// create a TreeModel (ListStore)
		list_store = Gtk::ListStore::create(model_columns);
		treeview->set_model(list_store);

		selection = treeview->get_selection();
		selection->signal_changed().connect(sigc::mem_fun(*this,
				&self_type::on_tree_selection_changed) );

	}


	// --------------- Parse help text

	/*
	README.txt File Format

	The whole text is converted to unix newline format before parsing.
	Sections are separated by 3 newlines (two empty lines).
	The first line of the section is its header.
	When splitting the file to sections and headers, any leading or trailing
		whitespace is removed.
	If there is a single newline inside a section, it is converted to
		space to enable correct wrapping.
	If there are two consequent newlines, they are left as they are,
		essentially making a paragraph break.
	*/

	std::string readme = hz::string_any_to_unix_copy(ReadmeTextResData().get_string());


	// Paragraphs are delimited by 3 empty lines
	std::vector<std::string> topics;
	hz::string_split(readme, "\n\n\n\n", topics, true);  // skip empty


	// Add to treeview and textview

	Gtk::TextView* content = this->lookup_widget<Gtk::TextView*>("content_textview");

	if (treeview && content) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = content->get_buffer();

		buffer->create_mark("Begin", buffer->begin(), true);

		for (unsigned int i = 0; i < topics.size(); ++i) {
			std::string topic = hz::string_trim_copy(topics[i]);

			// The first line of topic is its title
			std::vector<std::string> topic_split;
			hz::string_split(topic, "\n\n", topic_split, true, 2);  // skip empty, get 2 elements only

			if (topic_split.size() < 2) {
				debug_out_warn("app", DBG_FUNC_MSG << "Cannot extract topic title in topic " << i << "\n");
				continue;
			}

			std::string topic_title = hz::string_trim_copy(topic_split[0]);
			std::string topic_body = hz::string_trim_copy(topic_split[1]);

			buffer->create_mark(topic_title, buffer->end(), true);  // set topic mark to the end of what's there

			// add the title and make it bold
			buffer->insert(buffer->end(), "\n" + topic_title);

			Gtk::TextIter first = buffer->end(), last = first;
			first.backward_lines(1);

			Glib::RefPtr<Gtk::TextTag> tag = buffer->create_tag();
			tag->property_weight() = Pango::WEIGHT_BOLD;
			tag->property_size_points() = 14;

			buffer->apply_tag(tag, first, last);

			// add the rest

			// single newlines to spaces, to allow proper wrapping.
			app_pcre_replace("/([^\\n])\\n([^\\n])/", "\\1 \\2", topic_body);
			buffer->insert(buffer->end(), "\n\n" + topic_body + "\n\n");


			// Add to treeview

			Gtk::TreeRow row = *(list_store->append());
			row[col_topic] = topic_title;

		}

	}


	// ---------------


	// show();
}



void GscHelpWindow::set_topic(const Glib::ustring& topic)
{
	this->selection_callback_enabled = false;  // temporarily disable it

	// scroll to it

	Gtk::TextView* content = this->lookup_widget<Gtk::TextView*>("content_textview");
	if (content) {
		Glib::RefPtr<Gtk::TextBuffer> buffer = content->get_buffer();

		Glib::RefPtr<Gtk::TextMark> mark = buffer->get_mark(topic);
		if (mark)
			content->scroll_to(mark, 0., 0., 0.);
	}

	// select it in tree view
	Gtk::TreeView* treeview = this->lookup_widget<Gtk::TreeView*>("topics_treeview");

	if (treeview && !list_store->children().empty()) {
		for (Gtk::TreeIter iter = list_store->children().begin(); iter != list_store->children().end(); ++iter) {
			if (iter->get_value(col_topic) == topic) {
				selection->select(*iter);
				treeview->scroll_to_cell(list_store->get_path(iter), *(treeview->get_column(0)), 0.3, 0.);  // about 30% from top
				break;
			}
		}
	}

	this->selection_callback_enabled = true;  // enable it back
}




void GscHelpWindow::on_tree_selection_changed()
{
	if (!this->selection_callback_enabled)
		return;

	if (selection->count_selected_rows()) {
		Gtk::TreeIter iter = selection->get_selected();
		Gtk::TreeRow row = *iter;

		set_topic(row[col_topic]);
	}
}





