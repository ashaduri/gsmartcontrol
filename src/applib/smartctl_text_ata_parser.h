/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SMARTCTL_TEXT_ATA_PARSER_H
#define SMARTCTL_TEXT_ATA_PARSER_H

#include <string>
#include <vector>

#include "smartctl_parser.h"



/// Smartctl (S)ATA text output parser.
/// Note: ALL parse_* functions (except parse())
/// expect data in unix-newline format!
class SmartctlTextAtaParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlTextAtaParser() = default;

		// Overridden
		[[nodiscard]] hz::ExpectedVoid<SmartctlParserError> parse(std::string_view smartctl_output) override;


	protected:

		/// Parse the section part (with "=== .... ===" header) - info or data sections.
		hz::ExpectedVoid<SmartctlParserError> parse_section(const std::string& header, const std::string& body);


		/// Parse the info section (without "===" header).
		/// This includes --info and --get=all.
		hz::ExpectedVoid<SmartctlParserError> parse_section_info(const std::string& body);

		/// Parse a component (one line) of the info section
		hz::ExpectedVoid<SmartctlParserError> parse_section_info_property(StorageProperty& p);


		/// Parse the Data section (without "===" header)
		hz::ExpectedVoid<SmartctlParserError> parse_section_data(const std::string& body);

		/// Parse subsections of Data section
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_health(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_capabilities(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_attributes(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_directory_log(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_error_log(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_selftest_log(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_selective_selftest_log(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_scttemp_log(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_scterc_log(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_devstat(const std::string& sub);
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_subsection_sataphy(const std::string& sub);

		/// Check the capabilities for internal properties we can use.
		hz::ExpectedVoid<SmartctlParserError> parse_section_data_internal_capabilities(StorageProperty& cap_prop);


		/// Set "info" section data ("smartctl -i" output, or the first part of "smartctl -x" output)
		void set_data_section_info(std::string s);

		/// Parse "data" section data (the second part of "smartctl -x" output).
		void set_data_section_data(std::string s);


	private:

		std::string data_section_info_;  ///< "info" section data, filled by parse_section_info()
		std::string data_section_data_;  ///< "data" section data, filled by parse_section_data()

};






#endif

/// @}
