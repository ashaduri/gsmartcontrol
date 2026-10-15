/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <glibmm.h>

#include "warning_colors.h"
#include "gui_utils.h"


bool app_property_get_row_highlight_colors(bool dark_mode, WarningLevel warning, std::string& fg, std::string& bg)
{
	// Note: we're setting both fg and bg, to avoid theme conflicts.
	if (warning == WarningLevel::Notice) {
		fg = dark_mode ? "#FFFFFF" : "#000000";  // white for dark themes, black for light themes
		bg = dark_mode ? "#6B2050" : "#FFD5EE";  // dark pinkish for dark themes, pinkish for light themes

	} else if (warning == WarningLevel::Warning) {
		fg = dark_mode ? "#FFFFFF" : "#000000";  // white for dark themes, black for light themes
		bg = dark_mode ? "#802020" : "#FFA0A0";  // dark red for dark themes, light red for light themes

	} else if (warning == WarningLevel::Alert) {
		fg = dark_mode ? "#FFFFFF" : "#000000";  // white for dark themes, black for light themes
		bg = dark_mode ? "#AA0000" : "#FF0000";  // darker red for dark themes, bright red for light themes
	}

	return !(fg.empty());
}



bool app_property_get_label_highlight_color(bool dark_mode, WarningLevel warning, std::string& fg)
{
	if (warning == WarningLevel::None) {
		return false;
	}

	if (warning == WarningLevel::Notice) {
		fg = dark_mode ? "#FF9999" : "#770000";  // lighter red for dark themes, very dark red for light themes

	} else if (warning == WarningLevel::Warning) {
		fg = dark_mode ? "#FF6666" : "#C00000";  // lighter red for dark themes, dark red for light themes

	} else if (warning == WarningLevel::Alert) {
		fg = dark_mode ? "#FF4444" : "#FF0000";  // lighter/pink red for dark themes, bright red for light themes
	}

	return !(fg.empty());
}



std::string storage_property_get_warning_reason(const StorageProperty& p)
{
	std::string fg, start = "<b>", stop = "</b>";
	if (app_property_get_label_highlight_color(gui_is_dark_theme_active(), p.warning_level, fg)) {
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



/// @}
