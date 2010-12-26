/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <gtkmm.h>
#include <gtk/gtk.h>  // gtk_* stuff
#include <vector>

#include "app_gtkmm_features.h"  // APP_GTKMM_OLD_TOOLTIPS

#if defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS
	#include <gtk/gtk.h>  // gtk_tooltips_*
#endif

#include "app_pango_utils.h"  // app_pango_strip_markup()
#include "app_gtkmm_utils.h"




// Note: This works only if the column has custom widget set.
Gtk::Widget* app_gtkmm_get_column_header(Gtk::TreeViewColumn& column)
{
	Gtk::Widget* w = column.get_widget();
	Gtk::Widget* p1 = 0;
	Gtk::Widget* p2 = 0;
	Gtk::Widget* p3 = 0;

	// move up to GtkAlignment, then GtkHBox, then GtkButton.
	if (w && (p1 = w->get_parent()) && (p2 = p1->get_parent()) && (p3 = p2->get_parent()))
		return p3;

	return NULL;
}



// Read column header text and create a label with that text, set it as column's custom widget.
Gtk::Widget* app_gtkmm_labelize_column(Gtk::TreeViewColumn& column)
{
	Gtk::Label* label = Gtk::manage(new Gtk::Label(column.get_title()));
	label->show();
	column.set_widget(*label);
	return label;
}




// A wrapper around set_tooltip_*() for portability across different gtkmm versions.
void app_gtkmm_set_widget_tooltip(Gtk::Widget& widget,
		const Glib::ustring& tooltip_text, bool use_markup)
{
	// set_tooltip_* is available since 2.12
#if !(defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS)
	if (use_markup) {
		widget.set_tooltip_markup(tooltip_text);
	} else {
		widget.set_tooltip_text(tooltip_text);
	}

#else  // use the old tooltips api
	Gtk::Widget* toplevel = widget.get_toplevel();
	if (toplevel && toplevel->is_toplevel()) {  // orphan widgets return themselves, so check toplevelness.
		GtkTooltips* tooltips = static_cast<GtkTooltips*>(toplevel->get_data("window_tooltips"));
		if (tooltips) {
			if (use_markup) {
				// strip markup
				Glib::ustring stripped;
				if (app_pango_strip_markup(tooltip_text, stripped)) {
					gtk_tooltips_set_tip(tooltips, widget.gobj(), stripped.c_str(), "");
				}

			} else {
				gtk_tooltips_set_tip(tooltips, widget.gobj(), tooltip_text.c_str(), "");
			}
		}
	}
#endif
}



// unset model on treeview, cross-gtkmm-version.
void app_gtkmm_treeview_unset_model(Gtk::TreeView* treeview)
{
	// gtkmm's TreeView::unset_model() is since gtkmm 2.8
	// (and there's no version info in docs), so use this instead.
	if (treeview)
		gtk_tree_view_set_model(treeview->gobj(), 0);
}



// unset model on combobox, cross-gtkmm-version.
void app_gtkmm_combobox_unset_model(Gtk::ComboBox* box)
{
	// there's no ComboBox::unset_model() in gtkmm (huh?)
	if (box)
		gtk_combo_box_set_model(box->gobj(), 0);
}






#if defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS

namespace {

	// helper function for gtkmm_set_treeview_tooltip_column()
	inline bool app_on_treeview_motion_notify_event_tooltip(GdkEventMotion* event, Gtk::TreeView* treeview,
			Gtk::TreeModelColumn<Glib::ustring> tooltip_column)
	{
		// debug_out_dump("app", DBG_FUNC_MSG << "x=" << int(event->x) << ", y=" << int(event->y) << "\n");

		Gtk::TreePath path;
		Gtk::TreeViewColumn* mouse_column = 0;
		int cell_x = 0, cell_y = 0;

		if (treeview->get_path_at_pos(static_cast<int>(event->x), static_cast<int>(event->y),
				path, mouse_column, cell_x, cell_y)) {  // returns true if we're on a cell
			app_gtkmm_set_widget_tooltip(*treeview, (*(treeview->get_model()->get_iter(path)))[tooltip_column], true);
		}

		return false;  // continue handling (no conflicts there?)
	}


	// helper function for gtkmm_set_iconview_tooltip_column()
	inline bool app_on_iconview_motion_notify_event_tooltip(GdkEventMotion* event, Gtk::IconView* iconview,
			Gtk::TreeModelColumn<Glib::ustring> tooltip_column, Glib::RefPtr<Gtk::ListStore> model)
	{
		// debug_out_dump("app", DBG_FUNC_MSG << "x=" << int(event->x) << ", y=" << int(event->y) << "\n");

		// don't use get_item_at_pos() - it's not available in gtkmm < 2.8.
		Gtk::TreePath tpath = iconview->get_path_at_pos(static_cast<int>(event->x), static_cast<int>(event->y));
		if (tpath.gobj() && !tpath.empty()) {  // without gobj() check gtkmm 2.6 (but not 2.12) prints lots of errors
			// somehow, getting model from iconview doesn't work (segfault).
			// Glib::RefPtr<Gtk::TreeModel> model = iconview->get_model();
			if (tpath.gobj())
				app_gtkmm_set_widget_tooltip(*iconview, (*(model->get_iter(tpath)))[tooltip_column], true);
		}

		return false;  // continue handling (no conflicts there?)
	}

}

#endif



void gtkmm_set_treeview_tooltip_column(Gtk::TreeView* treeview,
		Gtk::TreeModelColumn<Glib::ustring>& col_tooltip)
{
	if (!treeview)
		return;

#if !(defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS)
	treeview->set_tooltip_column(col_tooltip.index());  // gtkmm has only index-based api here

#else  // manually set tooltips when hovering over cells
	treeview->signal_motion_notify_event().connect(  // connect before other events, or we'll get some screwed event data
			sigc::bind(sigc::bind(sigc::ptr_fun(&app_on_treeview_motion_notify_event_tooltip), col_tooltip), treeview), false);
#endif
}



void gtkmm_set_iconview_tooltip_column(Gtk::IconView* iconview,
		Gtk::TreeModelColumn<Glib::ustring>& col_tooltip, Glib::RefPtr<Gtk::ListStore> model)
{
	if (!iconview)
		return;

#if !(defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS)
	iconview->set_tooltip_column(col_tooltip.index());  // gtkmm has only index-based api here

#else  // manually set tooltips when hovering over cells
	iconview->signal_motion_notify_event().connect(  // connect before other events, or we'll get some screwed event data
			sigc::bind(sigc::bind(sigc::bind(sigc::ptr_fun(&app_on_iconview_motion_notify_event_tooltip), model), col_tooltip), iconview), false);
#endif
}




bool app_gtkmm_icon_theme_has_icon(Glib::RefPtr<Gtk::IconTheme> theme,
		const Glib::ustring& icon_name, int size)
{
	if (!theme || !theme->has_icon(icon_name))  // check if it exists first, or exception may be thrown.
		return false;

	std::vector<int> sizes = theme->get_icon_sizes(icon_name);

	for (std::vector<int>::const_iterator iter = sizes.begin(); iter != sizes.end(); ++iter) {
		if (*iter == size || *iter == -1)  // -1 means scalable
			return true;
	}

	return false;
}






