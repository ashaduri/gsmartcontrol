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

#ifndef SMARTCTL_PARSER_TYPES_H
#define SMARTCTL_PARSER_TYPES_H

#include "local_glibmm.h"

#include "hz/enum_helper.h"


enum class SmartctlParserError {
	EmptyInput,
	UnsupportedFormat,
	SyntaxError,
	NoVersion,
	IncompatibleVersion,
	NoSection,  ///< Returned by parser of each section if the section is not found
	UnknownSection,  ///< Local parsing function error
	InternalError,
	NoSubsectionsParsed,
	DataError,
	KeyNotFound,
};



enum class SmartctlParserType {
	Basic,  ///< Info only, supports all types of devices
	Ata,  ///< (S)ATA
	Nvme,  ///< NVMe
//	Scsi,  ///< SCSI
};



enum class SmartctlOutputFormat {
	Json,
	Text,
};



enum class SmartctlParserPreferenceType {
	Auto,
	Json,
	Text,
};



/// Helper structure for enum-related functions
struct SmartctlParserPreferenceTypeExt
		: public hz::EnumHelper<
				SmartctlParserPreferenceType,
				SmartctlParserPreferenceTypeExt,
		        Glib::ustring>
{
	static constexpr inline SmartctlParserPreferenceType default_value = SmartctlParserPreferenceType::Auto;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{SmartctlParserPreferenceType::Auto, {"auto", _("Automatic")}},
			{SmartctlParserPreferenceType::Json, {"json", _("JSON")}},
			{SmartctlParserPreferenceType::Text, {"text", _("Text")}},
		};
	}

};




#endif

/// @}
