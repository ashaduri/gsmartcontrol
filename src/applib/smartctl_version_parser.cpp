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

// #include <glibmm.h>

#include "hz/string_algo.h"  // string_*
#include "hz/string_num.h"  // string_is_numeric, number_to_string
#include "hz/debug.h"  // debug_*

#include "app_regex.h"
#include "smartctl_version_parser.h"




bool SmartctlVersionParser::parse_version_text(const std::string& s, std::string& version_only, std::string& version_full)
{
	// e.g.
	// "smartctl version 5.37"
	// "smartctl 5.39"
	// "smartctl 5.39 2009-06-03 20:10" (cvs versions)
	// "smartctl 5.39 2009-08-08 r2873" (svn versions)
	// "smartctl 7.3 (build date Feb 11 2022)" (git versions)
	// "smartctl pre-7.4 2023-06-13 r5481" (pre-releases)
	if (!app_regex_partial_match(R"(/^smartctl (?:version )?((?:pre-)?([0-9][^ \t\n\r]+)(?: [0-9 r:-]+)?)/mi)", s, {&version_full, &version_only})) {
		debug_out_error("app", DBG_FUNC_MSG << "No smartctl version information found in supplied string.\n");
		return false;
	}

	hz::string_trim(version_only);
	hz::string_trim(version_full);

	return true;
}



std::optional<double> SmartctlVersionParser::get_numeric_version(const std::string& version_only)
{
	double numeric_version = 0;
	if (!hz::string_is_numeric_nolocale<double>(version_only, numeric_version, false)) {
		return std::nullopt;
	}
	return numeric_version;
}



bool SmartctlVersionParser::check_format_supported(SmartctlOutputFormat format, const std::string& version_only)
{
	if (auto numeric_version = get_numeric_version(version_only); numeric_version.has_value()) {
		switch(format) {
			case SmartctlOutputFormat::Text:
				return numeric_version.value() >= minimum_req_text_version;
			case SmartctlOutputFormat::Json:
				return numeric_version.value() >= minimum_req_json_version;
		}
	}
	return false;
}



namespace {

SmartctlOutputFormat s_smartctl_output_default_format = SmartctlOutputFormat::Json;

}



void SmartctlVersionParser::set_default_format(SmartctlOutputFormat format)
{
	s_smartctl_output_default_format = format;
}



SmartctlOutputFormat SmartctlVersionParser::get_default_format([[maybe_unused]] SmartctlParserType parser_type)
{
	// We no longer differentiate between parser types - they
	// all either use Text, or Json.
//	switch (parser_type) {
//		case SmartctlParserType::Basic:
//			return SmartctlOutputFormat::Json;
//		case SmartctlParserType::Ata:
//			return SmartctlOutputFormat::Json;
//		case SmartctlParserType::Nvme:
//			return SmartctlOutputFormat::Json;
//	}
	return s_smartctl_output_default_format;
}



SmartctlParserType SmartctlVersionParser::get_default_parser_type(StorageDeviceDetectedType detected_type)
{
	switch (detected_type) {
		case StorageDeviceDetectedType::Unknown:
		case StorageDeviceDetectedType::NeedsExplicitType:
		case StorageDeviceDetectedType::BasicScsi:
		case StorageDeviceDetectedType::CdDvd:
		case StorageDeviceDetectedType::UnsupportedRaid:
			break;
		case StorageDeviceDetectedType::AtaAny:
		case StorageDeviceDetectedType::AtaHdd:
		case StorageDeviceDetectedType::AtaSsd:
			return SmartctlParserType::Ata;
		case StorageDeviceDetectedType::Nvme:
			return SmartctlParserType::Nvme;
	}
	return SmartctlParserType::Basic;
}




/// @}
