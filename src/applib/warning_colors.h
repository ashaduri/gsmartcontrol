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

#ifndef WARNING_COLORS_H
#define WARNING_COLORS_H

#include <glibmm.h>
#include <gtkmm.h>

#include "storage_property.h"


/// Check if a dark GTK theme is currently active
inline bool is_dark_theme_active()
{
	// Try to get the GTK settings to check for dark theme preference
	Glib::RefPtr<Gtk::Settings> settings = Gtk::Settings::get_default();
	if (settings) {
		// Check if the application prefers dark theme
		bool prefer_dark = false;
		settings->get_property("gtk-application-prefer-dark-theme", prefer_dark);
		if (prefer_dark) {
			return true;
		}

		// Check theme name for common dark theme identifiers
		Glib::ustring theme_name;
		settings->get_property("gtk-theme-name", theme_name);
		std::string theme_str = theme_name.lowercase();
		if (theme_str.find("dark") != std::string::npos ||
		    theme_str.find("black") != std::string::npos) {
			return true;
		}
	}

	return false;
}


/// Get colors for tree rows according to warning severity.
/// \return true if the colors were changed.
inline bool app_property_get_row_highlight_colors(WarningLevel warning, std::string& fg, std::string& bg)
{
	// Note: we're setting both fg and bg, to avoid theme conflicts.
	if (warning == WarningLevel::Notice) {
		fg = "#000000";  // black
		bg = "#FFD5EE";  // pinkish

	} else if (warning == WarningLevel::Warning) {
		fg = "#000000";  // black
		bg = "#FFA0A0";  // even more pinkish

	} else if (warning == WarningLevel::Alert) {
		fg = "#000000";  // black
		bg = "#FF0000";  // red
	}

	return !(fg.empty());
}



/// Get color for labels according to warning severity.
/// \return true if the color was changed.
inline bool app_property_get_label_highlight_color(WarningLevel warning, std::string& fg)
{
	bool dark_theme = is_dark_theme_active();

	if (warning == WarningLevel::Notice) {
		fg = dark_theme ? "#FF9999" : "#770000";  // lighter red for dark themes, very dark red for light themes

	} else if (warning == WarningLevel::Warning) {
		fg = dark_theme ? "#FF6666" : "#C00000";  // lighter red for dark themes, dark red for light themes

	} else if (warning == WarningLevel::Alert) {
		fg = dark_theme ? "#FF4444" : "#FF0000";  // lighter/pink red for dark themes, bright red for light themes
	}

	return !(fg.empty());
}




/// Format warning text, but without description
inline std::string storage_property_get_warning_reason(const StorageProperty& p)
{
	std::string fg, start = "<b>", stop = "</b>";
	if (app_property_get_label_highlight_color(p.warning_level, fg)) {
		start += "<span color=\"" + fg + "\">";
		stop = "</span>" + stop;
	}

	switch (p.warning_level) {
		case WarningLevel::None:
			// nothing
			break;
		case WarningLevel::Notice:
			/// Translators: %1 and %2 are HTML tags, %3 is a message.
			return Glib::ustring::compose(_("%1Notice:%2 %3"), start, stop, Glib::Markup::escape_text(p.warning_reason));
		case WarningLevel::Warning:
			/// Translators: %1 and %2 are HTML tags, %3 is a message.
			return Glib::ustring::compose(_("%1Warning:%2 %3"), start, stop, Glib::Markup::escape_text(p.warning_reason));
		case WarningLevel::Alert:
			/// Translators: %1 and %2 are HTML tags, %3 is a message.
			return Glib::ustring::compose(_("%1ALERT:%2 %3"), start, stop, Glib::Markup::escape_text(p.warning_reason));
	}

	return {};
}





#endif

/// @}
