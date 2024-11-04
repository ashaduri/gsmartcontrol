/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_PROPERTY_DESCR_HELPERS_H
#define STORAGE_PROPERTY_DESCR_HELPERS_H

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <string>


/// Get text related to "uncorrectable sectors"
inline const std::string& get_suffix_for_uncorrectable_property_description()
{
	static const std::string text = Glib::Markup::escape_text(
			_("When a drive encounters a surface error, it marks that sector as \"unstable\" (also known as \"pending reallocation\"). "
			"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
			"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
			"transferring the data so that no changes are reported to the operating system. This is why you generally don't see \"bad blocks\" "
			"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area."
			"\n\nNote: SSDs reallocate blocks as part of their normal operation, so low reallocation counts are not critical for them."));
	return text;
}



#endif

/// @}
