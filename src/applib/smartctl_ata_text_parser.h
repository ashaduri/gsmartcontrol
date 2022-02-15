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

#ifndef SMARTCTL_ATA_TEXT_PARSER_H
#define SMARTCTL_ATA_TEXT_PARSER_H

#include <string>
#include <vector>

#include "smartctl_parser.h"



/// Smartctl (S)ATA text output parser.
/// Note: ALL parse_* functions (except parse_full() and parse_version())
/// expect data in unix-newline format!
class SmartctlAtaTextParser : public SmartctlParser {
	public:

		// Defaulted, used by make_unique.
		SmartctlAtaTextParser() = default;

		// Overridden
		bool parse_full(const std::string& full) override;


	protected:

		/// Parse the section part (with "=== .... ===" header) - info or data sections.
		bool parse_section(const std::string& header, const std::string& body);


		/// Parse the info section (without "===" header).
		/// This includes --info and --get=all.
		bool parse_section_info(const std::string& body);

		/// Parse a component (one line) of the info section
		bool parse_section_info_property(AtaStorageProperty& p);


		/// Parse the Data section (without "===" header)
		bool parse_section_data(const std::string& body);

		/// Parse subsections of Data section
		bool parse_section_data_subsection_health(const std::string& sub);
		bool parse_section_data_subsection_capabilities(const std::string& sub);
		bool parse_section_data_subsection_attributes(const std::string& sub);
		bool parse_section_data_subsection_directory_log(const std::string& sub);
		bool parse_section_data_subsection_error_log(const std::string& sub);
		bool parse_section_data_subsection_selftest_log(const std::string& sub);
		bool parse_section_data_subsection_selective_selftest_log(const std::string& sub);
		bool parse_section_data_subsection_scttemp_log(const std::string& sub);
		bool parse_section_data_subsection_scterc_log(const std::string& sub);
		bool parse_section_data_subsection_devstat(const std::string& sub);
		bool parse_section_data_subsection_sataphy(const std::string& sub);

		/// Check the capabilities for internal properties we can use.
		bool parse_section_data_internal_capabilities(AtaStorageProperty& cap_prop);


		/// Set "info" section data ("smartctl -i" output, or the first part of "smartctl -x" output)
		void set_data_section_info(const std::string& s);

		/// Parse "data" section data (the second part of "smartctl -x" output).
		void set_data_section_data(const std::string& s);


	private:

		std::string data_section_info_;  ///< "info" section data, filled by parse_section_info()
		std::string data_section_data_;  ///< "data" section data, filled by parse_section_data()

};






#endif

/// @}
