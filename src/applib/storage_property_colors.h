/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_PROPERTY_COLORS_H
#define STORAGE_PROPERTY_COLORS_H

#include "storage_property.h"



// colors for tree rows
inline bool app_property_get_row_hilight_colors(StorageProperty::warning_t warning, std::string& fg, std::string& bg)
{
	// Note: we're setting both fg and bg, to avoid theme conflicts.
	if (warning == StorageProperty::warning_notice) {
		fg = "#000000";  // black
		bg = "#FFD5EE";  // pinkish

	} else if (warning == StorageProperty::warning_warn) {
		fg = "#000000";  // black
		bg = "#FFA0A0";  // even more pinkish

	} else if (warning == StorageProperty::warning_alert) {
		fg = "#000000";  // black
		bg = "#FF0000";  // red
	}

	return !(fg.empty());
}



// colors for labels
inline bool app_property_get_label_hilight_color(StorageProperty::warning_t warning, std::string& fg)
{
	if (warning == StorageProperty::warning_notice) {
		fg = "#770000";  // very dark red

	} else if (warning == StorageProperty::warning_warn) {
		fg = "#C00000";  // dark red

	} else if (warning == StorageProperty::warning_alert) {
		fg = "#FF0000";  // red
	}

	return !(fg.empty());
}




// Format warning text, but without description
inline std::string storage_property_get_warning_reason(const StorageProperty& p)
{
	std::string fg, start, stop;
	if (app_property_get_label_hilight_color(p.warning, fg)) {
		start = "<span color=\"" + fg + "\">";
		stop = "</span>";
	}

	if (p.warning == StorageProperty::warning_notice) {
		return "<b>" + start + "Notice:" + stop + "</b> " + p.warning_reason;

	} else if (p.warning == StorageProperty::warning_warn) {
		return "<b>" + start + "Warning:" + stop + "</b> " + p.warning_reason;

	} else if (p.warning == StorageProperty::warning_alert) {
		return "<b>" + start + "ALERT:" + stop + "</b> " + p.warning_reason;
	}

	return "";
}




// append warning to description
inline void storage_property_autoset_warning_descr(StorageProperty& p)
{
	std::string reason = storage_property_get_warning_reason(p);
	p.set_description(p.get_description() + (reason.empty() ? "" : "\n\n" + reason));
}






#endif
