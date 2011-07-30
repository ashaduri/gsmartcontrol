/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include <gtkmm/widget.h>
#include <gtkmm/treeviewcolumn.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treeview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/iconview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/icontheme.h>  // must include (even if gtkmm.h is included) for older gtkmm (at least 2.6, but not 2.12)

#include "hz/down_cast.h"



/// Get column header widget of a tree view column.
/// Note: This works only if the column has custom widget set.
Gtk::Widget* app_gtkmm_get_column_header(Gtk::TreeViewColumn& column);


/// Read column header text and create a label with that text. Set the label as
/// column's custom widget and return it.
Gtk::Widget* app_gtkmm_labelize_column(Gtk::TreeViewColumn& column);


/// Unset model on treeview, cross-gtkmm-version.
void app_gtkmm_treeview_unset_model(Gtk::TreeView* treeview);


/// Unset model on combobox (there's no straight way to do it in gtkmm, I think)
void app_gtkmm_combobox_unset_model(Gtk::ComboBox* box);


/// A wrapper around set_tooltip_*() for portability across different gtkmm versions.
void app_gtkmm_set_widget_tooltip(Gtk::Widget& widget,
		const Glib::ustring& tooltip_text, bool use_markup = false);


/// A portable wrapper around TreeView::set_tooltip_column().
void gtkmm_set_treeview_tooltip_column(Gtk::TreeView* treeview,
		Gtk::TreeModelColumn<Glib::ustring>& col_tooltip);


/// A portable wrapper around IconView::set_tooltip_column().
void gtkmm_set_iconview_tooltip_column(Gtk::IconView* iconview,
		Gtk::TreeModelColumn<Glib::ustring>& col_tooltip, Glib::RefPtr<Gtk::ListStore> model);



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
		Gtk::CellRendererText* cr_type = hz::down_cast<Gtk::CellRendererText*>(treeview.get_column_cell_renderer(num_tree_cols - 1));
		if (cr_type) {  // may not be true if it's not Text (unless static_cast is used, in which case we're screwed)
			treeview.get_column(num_tree_cols - 1)->clear_attributes(*cr_type);  // clear "text" attribute. "markup" won't work without this.
			treeview.get_column(num_tree_cols - 1)->add_attribute(cr_type->property_markup(), mcol);  // render col_type as markup.
		}
	}

	return num_tree_cols;
}



/// Check whether the icon theme has the specified icon of size \c size.
bool app_gtkmm_icon_theme_has_icon(Glib::RefPtr<Gtk::IconTheme> theme,
		const Glib::ustring& icon_name, int size);






#endif

/// @}
