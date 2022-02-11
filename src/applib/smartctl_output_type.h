/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_OUTPUT_TYPE_H
#define SMARTCTL_OUTPUT_TYPE_H

#include "local_glibmm.h"

#include "hz/enum_helper.h"


enum class SmartctlOutputParserType {
	Auto,
	Json,
	Text,
};



/// Helper structure for enum-related functions
struct SmartctlOutputParserTypeExt
		: public hz::EnumHelper<
				SmartctlOutputParserType,
				SmartctlOutputParserTypeExt,
		        Glib::ustring>
{
	static constexpr inline SmartctlOutputParserType default_value = SmartctlOutputParserType::Auto;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{SmartctlOutputParserType::Auto, {"auto", _("Automatic")}},
			{SmartctlOutputParserType::Json, {"json", _("JSON")}},
			{SmartctlOutputParserType::Text, {"text", _("Text")}},
		};
	}

};




#endif

/// @}
