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

#ifndef WARNING_LEVEL_H
#define WARNING_LEVEL_H



/// Warning type
enum class WarningLevel {
	none,  ///< No warning
	notice,  ///< A known attribute is somewhat disturbing, but no smart error
	warning,  ///< SMART warning is raised by old-age attribute
	alert  ///< SMART warning is raised by pre-fail attribute, and similar errors
};





#endif

/// @}
