/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_GTKMM_TOOLS_H
#define APP_GTKMM_TOOLS_H

#include <string>
#include <gtkmm.h>



/// \def APP_GTKMM_CHECK_VERSION(major, minor, micro)
/// Similar to GTK_CHECK_VERSION, but for Gtkmm, which lacks this before gtkmm4.
/// This is useful as Gtk and Gtkmm versions may differ.
#ifndef APP_GTKMM_CHECK_VERSION
	#define APP_GTKMM_CHECK_VERSION(major, minor, micro) \
		(GTKMM_MAJOR_VERSION > (major) \
			|| (GTKMM_MAJOR_VERSION == (major) && (GTKMM_MINOR_VERSION > (minor) \
				|| (GTKMM_MINOR_VERSION == (minor) && GTKMM_MICRO_VERSION >= (micro)) \
				) \
			) \
		)
#endif




/// Get column header widget of a tree view column.
/// Note: This works only if the column has custom widget set.
/// \return nullptr on failure.
Gtk::Widget* app_gtkmm_get_column_header(Gtk::TreeViewColumn& column);


/// Read column header text and create a label with that text. Set the label as
/// column's custom widget and return it.
Gtk::Label& app_gtkmm_labelize_column(Gtk::TreeViewColumn& column);


/// A wrapper around set_tooltip_*(), calling appropriate method depending on `use_markup`.
void app_gtkmm_set_widget_tooltip(Gtk::Widget& widget,
		const Glib::ustring& tooltip_text, bool use_markup = false);


/// Convenience function for creating a TreeViewColumn from model column.
/// \return tree column index
template<typename DataType>
int app_gtkmm_create_tree_view_column(const Gtk::TreeModelColumn<DataType>& model_column,
		Gtk::TreeView& treeview, const Glib::ustring& header_title, const Glib::ustring& header_tooltip_text,
		bool sortable = false, bool use_cell_markup = false, bool header_tooltip_is_markup = false);



/// Get Glib::ustring from gchar*, freeing gchar*.
Glib::ustring app_ustring_from_gchar(gchar* str);


/// Convert a possibly invalid utf-8 string to valid utf-8.
/// \param str string to test and fix.
Glib::ustring app_make_valid_utf8(const Glib::ustring& str);


/// Make command output a valid utf-8 string. This function takes command output
/// (in locale encoding under Windows, utf-8 encoding under other OSes), and converts
/// it to valid utf-8.
/// The reason for this is that in Win32 we can't execute commands under C locale,
/// but we do execute them under C in other systems.
Glib::ustring app_make_valid_utf8_from_command_output(const std::string& str);




// ------------------------------------------- Implementation




template<typename DataType>
int app_gtkmm_create_tree_view_column(const Gtk::TreeModelColumn<DataType>& model_column,
		Gtk::TreeView& treeview, const Glib::ustring& header_title, const Glib::ustring& header_tooltip_text,
		bool sortable, bool use_cell_markup, bool header_tooltip_is_markup)
{
	int num_tree_cols = treeview.append_column(header_title, model_column);
	Gtk::TreeViewColumn* tcol = treeview.get_column(num_tree_cols - 1);
	if (tcol) {
		if (sortable) {
			tcol->set_sort_column(model_column);
		}

		app_gtkmm_labelize_column(*tcol);
		tcol->set_reorderable(true);
		tcol->set_resizable(true);

		Gtk::Widget* header = app_gtkmm_get_column_header(*tcol);
		if (header) {
			app_gtkmm_set_widget_tooltip(*header, header_tooltip_text, header_tooltip_is_markup);
		}
	}

	if (use_cell_markup) {
		if (auto* cr_type = dynamic_cast<Gtk::CellRendererText*>(treeview.get_column_cell_renderer(num_tree_cols - 1))) {
			treeview.get_column(num_tree_cols - 1)->clear_attributes(*cr_type);  // clear "text" attribute. "markup" won't work without this.
			treeview.get_column(num_tree_cols - 1)->add_attribute(cr_type->property_markup(), model_column);  // render col_type as markup.
		}
	}

	return num_tree_cols - 1;
}




#endif

/// @}
