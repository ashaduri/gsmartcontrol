/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_GTKMM_UTILS_H
#define APP_GTKMM_UTILS_H

#include <string>
#include <gtkmm.h>



/// Get column header widget of a tree view column.
/// Note: This works only if the column has custom widget set.
Gtk::Widget* app_gtkmm_get_column_header(Gtk::TreeViewColumn& column);


/// Read column header text and create a label with that text. Set the label as
/// column's custom widget and return it.
Gtk::Widget* app_gtkmm_labelize_column(Gtk::TreeViewColumn& column);


/// A wrapper around set_tooltip_*() for portability across different gtkmm versions.
void app_gtkmm_set_widget_tooltip(Gtk::Widget& widget,
		const Glib::ustring& tooltip_text, bool use_markup = false);



/// Convenience function for creating a TreeViewColumn .
template<typename T>
int app_gtkmm_create_tree_view_column(Gtk::TreeModelColumn<T>& mcol, Gtk::TreeView& treeview,
		const Glib::ustring& title, const Glib::ustring& tooltip_text, bool sortable = false, bool cell_markup = false)
{
	int num_tree_cols = treeview.append_column(title, mcol);
	Gtk::TreeViewColumn* tcol = treeview.get_column(num_tree_cols - 1);
	if (tcol) {
		if (sortable)
			tcol->set_sort_column(mcol);

		app_gtkmm_labelize_column(*tcol);
		tcol->set_reorderable(true);
		tcol->set_resizable(true);
	}

	Gtk::Widget* header = app_gtkmm_get_column_header(*tcol);
	if (header)
		app_gtkmm_set_widget_tooltip(*header, tooltip_text);

	if (cell_markup) {
		if (Gtk::CellRendererText* cr_type = dynamic_cast<Gtk::CellRendererText*>(treeview.get_column_cell_renderer(num_tree_cols - 1))) {
			treeview.get_column(num_tree_cols - 1)->clear_attributes(*cr_type);  // clear "text" attribute. "markup" won't work without this.
			treeview.get_column(num_tree_cols - 1)->add_attribute(cr_type->property_markup(), mcol);  // render col_type as markup.
		}
	}

	return num_tree_cols;
}



/// Get Glib::ustring from gchar*, freeing gchar*.
Glib::ustring app_ustring_from_gchar(gchar* str);


/// Convert a possibly invalid utf-8 string to valid utf-8.
/// \param str string to test and fix.
Glib::ustring app_utf8_make_valid(const Glib::ustring& str);


/// Make command output a valid utf-8 string. Essentially, this calls app_utf8_make_valid(),
/// supplying true for \c in_locale under Win32, and false under other systems.
/// The reason for this is that in Win32 we can't execute commands under C locale,
/// but we do execute them under C in other systems.
Glib::ustring app_output_make_valid(const Glib::ustring& str);




#endif

/// @}
