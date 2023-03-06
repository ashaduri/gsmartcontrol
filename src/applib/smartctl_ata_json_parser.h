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

#ifndef SMARTCTL_ATA_JSON_PARSER_H
#define SMARTCTL_ATA_JSON_PARSER_H

#include <string>
#include <vector>

#include "smartctl_parser.h"



/// Smartctl (S)ATA text output parser.
/// Note: ALL parse_* functions (except parse_full() and parse_version())
/// expect data in unix-newline format!
class SmartctlAtaJsonParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlAtaJsonParser() = default;

		// Overridden
		bool parse_full(const std::string& json_data_full) override;


};




#endif

/// @}
