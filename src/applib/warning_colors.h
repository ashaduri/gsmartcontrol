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

#ifndef WARNING_COLORS_H
#define WARNING_COLORS_H

#include <string>

#include "storage_property.h"
#include "warning_level.h"


/// Get colors for tree rows according to warning severity.
/// \return true if the colors were changed.
bool app_property_get_row_highlight_colors(bool dark_mode, WarningLevel warning, std::string& fg, std::string& bg);


/// Get color for labels according to warning severity.
/// \return true if the color was changed.
bool app_property_get_label_highlight_color(bool dark_mode, WarningLevel warning, std::string& fg);


/// Format warning text, but without description
std::string storage_property_get_warning_reason(const StorageProperty& p);



#endif

/// @}
