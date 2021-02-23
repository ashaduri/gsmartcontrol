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

#ifndef STORAGE_PROPERTY_COLORS_H
#define STORAGE_PROPERTY_COLORS_H

// TODO Remove this in gtkmm4.
#include "local_glibmm.h"

#include <glibmm.h>  // compose()
#include <glibmm/i18n.h>

#include "storage_property.h"



/// Get colors for tree rows according to warning severity.
/// \return true if the colors were changed.
inline bool app_property_get_row_highlight_colors(WarningLevel warning, std::string& fg, std::string& bg)
{
	// Note: we're setting both fg and bg, to avoid theme conflicts.
	if (warning == WarningLevel::notice) {
		fg = "#000000";  // black
		bg = "#FFD5EE";  // pinkish

	} else if (warning == WarningLevel::warning) {
		fg = "#000000";  // black
		bg = "#FFA0A0";  // even more pinkish

	} else if (warning == WarningLevel::alert) {
		fg = "#000000";  // black
		bg = "#FF0000";  // red
	}

	return !(fg.empty());
}



/// Get color for labels according to warning severity.
/// \return true if the color was changed.
inline bool app_property_get_label_highlight_color(WarningLevel warning, std::string& fg)
{
	if (warning == WarningLevel::notice) {
		fg = "#770000";  // very dark red

	} else if (warning == WarningLevel::warning) {
		fg = "#C00000";  // dark red

	} else if (warning == WarningLevel::alert) {
		fg = "#FF0000";  // red
	}

	return !(fg.empty());
}




/// Format warning text, but without description
inline std::string storage_property_get_warning_reason(const StorageProperty& p)
{
	std::string fg, start = "<b>", stop = "</b>";
	if (app_property_get_label_highlight_color(p.warning, fg)) {
		start += "<span color=\"" + fg + "\">";
		stop = "</span>" + stop;
	}

	if (p.warning == WarningLevel::notice) {
		/// Translators: %1 and %2 are HTML tags, %3 is a message.
		return Glib::ustring::compose(_("%1Notice:%2 %3"), start, stop, p.warning_reason);

	} else if (p.warning == WarningLevel::warning) {
		/// Translators: %1 and %2 are HTML tags, %3 is a message.
		return Glib::ustring::compose(_("%1Warning:%2 %3"), start, stop, p.warning_reason);

	} else if (p.warning == WarningLevel::alert) {
		/// Translators: %1 and %2 are HTML tags, %3 is a message.
		return Glib::ustring::compose(_("%1ALERT:%2 %3"), start, stop, p.warning_reason);
	}

	return std::string();
}




/// Append warning text to description and set it on the property
inline void storage_property_autoset_warning_descr(StorageProperty& p)
{
	std::string reason = storage_property_get_warning_reason(p);
	p.set_description(p.get_description() + (reason.empty() ? "" : "\n\n" + reason));
}






#endif

/// @}
