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

// #include "local_glibmm.h"
//#include <clocale>  // localeconv
//#include <cstdint>
//#include <utility>

// #include "hz/locale_tools.h"  // ScopedCLocale, locale_c_get().
//#include "hz/string_algo.h"  // string_*
//#include "hz/string_num.h"  // string_is_numeric, number_to_string
//#include "hz/debug.h"  // debug_*

#include "app_pcrecpp.h"
//#include "smartctl_text_ata_parser.h"
//#include "ata_storage_property_descr.h"
// #include "warning_colors.h"
//#include "smartctl_version_parser.h"
#include "smartctl_text_basic_parser.h"




hz::ExpectedVoid<SmartctlParserError> SmartctlTextBasicParser::parse(std::string_view smartctl_output)
{

	return {};
}






/// @}
