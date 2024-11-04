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

#include "storage_property.h"



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
	if (warning == WarningLevel::Notice) {
		fg = "#770000";  // very dark red

	} else if (warning == WarningLevel::Warning) {
		fg = "#C00000";  // dark red

	} else if (warning == WarningLevel::Alert) {
		fg = "#FF0000";  // red
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
