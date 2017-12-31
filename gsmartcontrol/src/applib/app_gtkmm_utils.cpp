/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // NOT NEEDED
#undef throw

#include <gtkmm.h>
#include <glibmm.h>
#include <glib.h>
#include <cstring>  // std::strlen
#include <vector>

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

	return nullptr;
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
	if (use_markup) {
		widget.set_tooltip_markup(tooltip_text);
	} else {
		widget.set_tooltip_text(tooltip_text);
	}
}



namespace {

	/// This has been copied from _g_utf8_make_valid() (glib-2.20.4).
	/// _g_utf8_make_valid() is GLib's private function for auto-correcting
	/// the potentially invalid utf-8 data.
	inline gchar* gsc_g_utf8_make_valid (const gchar* name)
	{
		GString* str;
		const gchar* remainder, *invalid;
		gint remaining_bytes, valid_bytes;

		g_return_val_if_fail (name != nullptr, nullptr);

		str = nullptr;
		remainder = name;
		remaining_bytes = gint(std::strlen(name));

		while (remaining_bytes != 0) {
			if (g_utf8_validate (remainder, remaining_bytes, &invalid))
				break;

			valid_bytes = gint(invalid - remainder);

			if (str == nullptr)
				str = g_string_sized_new (remaining_bytes);

			g_string_append_len (str, remainder, valid_bytes);
			/* append U+FFFD REPLACEMENT CHARACTER */
			g_string_append (str, "\357\277\275");

			remaining_bytes -= valid_bytes + 1;
			remainder = invalid + 1;
		}

		if (str == nullptr)
			return g_strdup (name);

		g_string_append (str, remainder);

		g_assert (g_utf8_validate (str->str, -1, nullptr));

		return g_string_free (str, FALSE);
	}

}



Glib::ustring app_ustring_from_gchar(gchar* str)
{
	if (!str) {
		return Glib::ustring();
	}
	Glib::ustring ustr(str);
	g_free(str);
	return ustr;
}



Glib::ustring app_utf8_make_valid(const Glib::ustring& str)
{
	char* s = gsc_g_utf8_make_valid(str.c_str());
	if (!s) {
		return Glib::ustring();
	}
	Glib::ustring res(s);
	g_free(s);
	return res;
}



Glib::ustring app_output_make_valid(const Glib::ustring& str)
{
	#ifdef _WIN32
	try {
		return app_utf8_make_valid(Glib::locale_to_utf8(str));
	} catch (Glib::ConvertError& e) {
		// nothing, try to fix as it is
	}
	#endif
	return app_utf8_make_valid(str);
}







/// @}
