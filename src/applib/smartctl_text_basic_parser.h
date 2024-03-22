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

#ifndef SMARTCTL_TEXT_BASIC_PARSER_H
#define SMARTCTL_TEXT_BASIC_PARSER_H

//#include <string>
//#include <vector>

#include "smartctl_parser.h"



/// Parse info output, regardless of device type
class SmartctlTextBasicParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlTextBasicParser() = default;

		// Overridden
		[[nodiscard]] hz::ExpectedVoid<SmartctlParserError> parse(std::string_view smartctl_output) override;

};






#endif

/// @}
