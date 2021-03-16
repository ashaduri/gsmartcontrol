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

#include "local_glibmm.h"
#include <gtkmm.h>
#include <glib.h>
#include <cstring>  // std::strlen
#include <vector>

#include "app_gtkmm_tools.h"




Gtk::Widget* app_gtkmm_get_column_header(Gtk::TreeViewColumn& column)
{
	Gtk::Widget* w = column.get_widget();
	Gtk::Widget* p1 = nullptr;
	Gtk::Widget* p2 = nullptr;
	Gtk::Widget* p3 = nullptr;

	// move up to GtkAlignment, then GtkHBox, then GtkButton.
	if (w && (p1 = w->get_parent()) && (p2 = p1->get_parent()) && (p3 = p2->get_parent()))
		return p3;

	return nullptr;
}



Gtk::Label& app_gtkmm_labelize_column(Gtk::TreeViewColumn& column)
{
	Gtk::Label* label = Gtk::manage(new Gtk::Label(column.get_title()));
	label->show();
	column.set_widget(*label);
	return *label;
}



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
	inline gchar* app_make_valid_utf_c (const gchar* name)
	{
		GString* str = nullptr;
		const gchar* remainder = nullptr;
		const gchar* invalid = nullptr;
		gint remaining_bytes = 0, valid_bytes = 0;

		g_return_val_if_fail (name != nullptr, nullptr);

		str = nullptr;
		remainder = name;
		remaining_bytes = gint(std::strlen(name));

		while (remaining_bytes != 0) {
			if (g_utf8_validate (remainder, remaining_bytes, &invalid) == TRUE)
				break;

			valid_bytes = gint(invalid - remainder);

			if (str == nullptr)
				str = g_string_sized_new (gsize(remaining_bytes));

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



Glib::ustring app_make_valid_utf8(const Glib::ustring& str)
{
	return app_ustring_from_gchar(app_make_valid_utf_c(str.c_str()));
}



Glib::ustring app_make_valid_utf8_from_command_output(const std::string& str)
{
	#ifdef _WIN32
	try {
		return Glib::locale_to_utf8(str);  // detects invalid utf-8 sequences
	} catch (Glib::ConvertError& e) {
		// nothing, try to fix as it is
	}
	#endif
	return app_ustring_from_gchar(app_make_valid_utf_c(str.c_str()));
}







/// @}
