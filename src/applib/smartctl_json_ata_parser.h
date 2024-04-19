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

#include "smartctl_parser.h"

#include <string>
#include <string_view>

#include "json/json.hpp"

#include "hz/error_container.h"



/// Smartctl (S)ATA JSON output parser
class SmartctlJsonAtaParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlJsonAtaParser() = default;

		// Overridden
		[[nodiscard]] hz::ExpectedVoid<SmartctlParserError> parse(std::string_view smartctl_output) override;

	private:

		/// Parse the info section (root node), filling in the properties
		hz::ExpectedVoid<SmartctlParserError> parse_section_info(const nlohmann::json& json_root_node);

		/// Parse the health section (root node), filling in the properties
		hz::ExpectedVoid<SmartctlParserError> parse_section_health(const nlohmann::json& json_root_node);
		
		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_capabilities(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_attributes(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_directory_log(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_error_log(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_selftest_log(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_selective_selftest_log(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_scttemp_log(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_scterc_log(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_devstat(const nlohmann::json& json_root_node);

		/// Parse a section from json data
		hz::ExpectedVoid<SmartctlParserError> parse_section_sataphy(const nlohmann::json& json_root_node);


		/// Check the capabilities for internal properties we can use.
		hz::ExpectedVoid<SmartctlParserError> parse_section_internal_capabilities(AtaStorageProperty& cap_prop);


};




#endif

/// @}
