/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_JSON_BASIC_PARSER_H
#define SMARTCTL_JSON_BASIC_PARSER_H

//#include <string>
//#include <vector>

#include "json/json.hpp"

#include "smartctl_parser.h"



/// Parse info output, regardless of device type
class SmartctlJsonBasicParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlJsonBasicParser() = default;

		// Overridden
		[[nodiscard]] hz::ExpectedVoid<SmartctlParserError> parse(std::string_view smartctl_output) override;


	private:

		hz::ExpectedVoid<SmartctlParserError> parse_section_basic_info(const nlohmann::json& json_root_node);

};






#endif

/// @}
