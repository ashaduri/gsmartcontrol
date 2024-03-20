/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2022 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_JSON_ATA_PARSER_H
#define SMARTCTL_JSON_ATA_PARSER_H

#include <string>

#include "json/json.hpp"
#include "smartctl_parser.h"



/// Smartctl (S)ATA JSON output parser
class SmartctlJsonAtaParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlJsonAtaParser() = default;

		// Overridden
		hz::ExpectedVoid<SmartctlParserError> parse(std::string_view smartctl_output) override;

	private:

		/// Parse the info section (root node), filling in the properties
		hz::ExpectedVoid<SmartctlParserError> parse_section_info(const nlohmann::json& json_root_node);

		/// Parse the health section (root node), filling in the properties
		hz::ExpectedVoid<SmartctlParserError> parse_section_health(const nlohmann::json& json_root_node);


};




#endif

/// @}
