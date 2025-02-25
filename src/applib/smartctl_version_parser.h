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

#ifndef SMARTCTL_VERSION_PARSER_H
#define SMARTCTL_VERSION_PARSER_H

#include <glibmm.h>

#include <string>
#include <optional>

#include "smartctl_parser_types.h"
#include "storage_property_descr.h"
#include "storage_device_detected_type.h"





/// Smartctl version parser.
class SmartctlVersionParser {
	public:

		/// Supply any text (not JSON) output of smartctl here, the smartctl version will be retrieved.
		/// The text does not have to be in Unix newline format.
		/// \param s "smartctl -V" command output.
		/// \param[out] version_only A string similar to "7.2"
		/// \param[out] version_full A string similar to "smartctl 7.2 2020-12-30 r5155"
		/// \return false if the version could not be parsed.
		static bool parse_version_text(const std::string& s, std::string& version_only, std::string& version_full);


		/// Get numeric version as a double from a parsed version.
		/// \param version_only A string similar to "7.2", as parsed by parse_version_text().
		/// \return Numeric version as a double, e.g. 7.2. std::nullopt if the version could not be parsed.
		static std::optional<double> get_numeric_version(const std::string& version_only);


		/// Check that the version of smartctl output can be parsed with a parser.
		static bool check_format_supported(SmartctlOutputFormat format, const std::string& version_only);


		/// Get default output format for a parser type.
		static void set_default_format(SmartctlOutputFormat format);

		/// Get default output format for a parser type.
		static SmartctlOutputFormat get_default_format(SmartctlParserType parser_type);


		/// Get default output format for a parser type.
		static SmartctlParserType get_default_parser_type(StorageDeviceDetectedType detected_type);


		// We require this version at runtime to support --get=all.
		static constexpr double minimum_req_runtime_version = 5.43;


	private:

		// Text Parser:
		// Tested with 5.1-xx versions (1 - 18), and 5.[20 - 38].
		// Note: 5.1-11 (maybe others too) with scsi disk gives non-parsable output (why?).
		// 5.0-24, 5.0-36, 5.0-49 tested with data only, from smartmontools site.
		// Can't fully test 5.0-xx, they don't support sata, and I have only sata.
		static constexpr double minimum_req_text_version = 5.0;

		// 7.3 adds "smart_support" section in json.
		// 7.4 adds nvme testing, but it's an optional feature not related to parsing.
		static constexpr double minimum_req_json_version = 7.3;


};






#endif

/// @}
