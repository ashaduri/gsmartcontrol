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

#include "smartctl_ata_json_parser.h"
#include "json/json.hpp"
#include "hz/debug.h"
#include "hz/string_algo.h"
#include "smartctl_version_parser.h"
#include "hz/string_num.h"



/*
Information not printed in JSON yet:

- Checksum warnings (smartctl.cpp: checksumwarning()).
	Smartctl output: Warning! SMART <section name> Structure error: invalid SMART checksum
 	Keys:
 		_text_only/attribute_data_checksum_error
 		_text_only/attribute_thresholds_checksum_error
 		_text_only/ata_error_log_checksum_error
 		_text_only/selftest_log_checksum_error

- Samsung warning
 	Smartctl output: May need -F samsung or -F samsung2 enabled; see manual for details
 	We ignore this in text parser.

- Warnings from drivedb.h in the middle of Info section
	Smartctl output (example):
		WARNING: A firmware update for this drive may be available,
		see the following Seagate web pages:
		...
	Keys: _text_only/info_warning

- Errors about consistency:
	"Invalid Error Log index ..."
	"Warning: ATA error count %d inconsistent with error log pointer"
	We ignore this in text parser.

- "mandatory SMART command failed" and similar errors.
	We ignore this in text parser.

- SMART support and some other Info keys
	_text_only/smart_supported
	_text_only/smart_enabled
	_text_only/write_cache_reorder
	_text_only/power_mode

- Automatic Offline Data Collection toggle support
	_text_only/aodc_support

 - Directory log supported
 	We don't use this.
 	_text_only/directory_log_supported

ata_smart_error_log/_not_present


Keys:
smartctl/version/_merged
 	Looks like "7.2"
smartctl/version/_merged_full
	Looks like "smartctl 7.2 2020-12-30 r5155", formed from "/smartctl" subkeys.

_custom/smart_enabled
 	Not present in json?
*/


namespace {


/// Get json node data. The path is slash-separated string.
/// \throws std::runtime_error If not found or one of the paths is not an object
template<typename T>
T get_node_data(const nlohmann::json& root, const std::string& path)
{
	using namespace std::literals;

	std::vector<std::string> components;
	hz::string_split(path, '/', components, true);

	const auto* curr = &root;
	for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
		const std::string& comp_name = components[comp_index];

		if (!curr->is_object()) {  // we can't have non-object values in the middle of a path
			throw std::runtime_error("Cannot get node data \""s + path + "\", component \"" + comp_name + "\" is not an object.");
		}
		if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
			const auto& jval = iter.value();
			if (comp_index + 1 == components.size()) {  // it's the "value" component
				try {
					return jval.get<T>();  // may throw json::type_error
				}
				catch (nlohmann::json::type_error& ex) {
					throw std::runtime_error(ex.what());
				}
			}
			// continue to the next component
			curr = &jval;

		} else {  // path component doesn't exist
			throw std::runtime_error("Cannot get node data \""s + path + "\", component \"" + comp_name + "\" does not exist.");
		}
	}

	throw std::runtime_error("Cannot get node data \""s + path + "\": Internal error.");
}


/// Get json node data. The path is slash-separated string.
/// If an error is found, the default value is returned.
template<typename T>
T get_node_data(const nlohmann::json& root, const std::string& path, const T& default_value)
{
	try {
		return get_node_data<T>(root, path);
	}
	catch (std::runtime_error& ex) {
		return default_value;
	}
}


}



hz::ExpectedVoid<SmartctlParserError> SmartctlAtaJsonParser::parse_full(const std::string& json_data_full)
{
	this->set_data_full(json_data_full);

	if (hz::string_trim_copy(json_data_full).empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Empty string passed as an argument. Returning.\n");
		return hz::Unexpected(SmartctlParserError::EmptyInput, "Smartctl data is empty.");
	}

	try {
		const nlohmann::json root_node = nlohmann::json::parse(json_data_full);

		{
			AtaStorageProperty p;
			p.set_name("Smartctl version", "smartctl/version/_merged", "Smartctl Version");
			auto json_ver = get_node_data<std::vector<int>>(root_node, "smartctl/version", {});
			if (json_ver.size() >= 2) {
				p.reported_value = hz::number_to_string_nolocale(json_ver.at(0)) + "." + hz::number_to_string_nolocale(json_ver.at(1));
			}
			p.value = p.reported_value;  // string-type value
			p.section = AtaStorageProperty::Section::info;  // add to info section
			add_property(p);
		}
		// {
		// 	AtaStorageProperty p;
		// 	p.set_name("Smartctl version", "smartctl/version/_merged_full", "Smartctl Version");
		// 	p.reported_value = version_full;
		// 	p.value = p.reported_value;  // string-type value
		// 	p.section = AtaStorageProperty::Section::info;  // add to info section
		// 	add_property(p);
		// }

		// if (!SmartctlVersionParser::check_parsed_version(SmartctlParserType::Text, version)) {
		// 	set_error_msg("Incompatible smartctl version.");
		// 	debug_out_warn("app", DBG_FUNC_MSG << "Incompatible smartctl version. Returning.\n");
		// 	return false;
		// }

		const std::unordered_map<std::string, std::string> info_keys = {
				{"model_family", _("Model Family")},
		};

		for (const auto& [key, jval] : root_node.items()) {
			if (auto found = info_keys.find(key); found != info_keys.end()) {
				AtaStorageProperty p;
				p.section = AtaStorageProperty::Section::info;
				p.set_name(key, key, found->second);
				p.reported_value = jval.get<std::string>();
				p.value = p.reported_value;  // string-type value

				// parse_section_info_property(p);  // set type and the typed value. may change generic_name too.

				add_property(p);
			}
		}


	}
	catch (const nlohmann::json::parse_error& e) {
		debug_out_warn("app", DBG_FUNC_MSG << "Error parsing smartctl output as JSON: " << e.what() << "\n");
		return hz::Unexpected(SmartctlParserError::SyntaxError, std::string("Invalid JSON data: ") + e.what());
	}
}





/// @}
