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

#include <format>

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


enum class AppJsonError {
	UnexpectedObjectInPath,
	PathNotFound,
	TypeError,
	EmptyPath,
	InternalError,
};



/// Get json node data. The path is slash-separated string.
/// \throws std::runtime_error If not found or one of the paths is not an object
template<typename T>
[[nodiscard]] hz::ExpectedValue<T, AppJsonError> get_node_data(const nlohmann::json& root, const std::string& path)
{
	using namespace std::literals;

	std::vector<std::string> components;
	hz::string_split(path, '/', components, true);

	if (components.empty()) {
		return hz::Unexpected(AppJsonError::EmptyPath, "Cannot get node data: Empty path.");
	}

	const auto* curr = &root;
	for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
		const std::string& comp_name = components[comp_index];

		if (!curr->is_object()) {  // we can't have non-object values in the middle of a path
			return hz::Unexpected(AppJsonError::UnexpectedObjectInPath,
					std::format("Cannot get node data \"{}\", component \"{}\" is not an object.", path, comp_name));
		}
		if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
			const auto& jval = iter.value();
			if (comp_index + 1 == components.size()) {  // it's the "value" component
				try {
					return jval.get<T>();  // may throw json::type_error
				}
				catch (nlohmann::json::type_error& ex) {
					return hz::Unexpected(AppJsonError::TypeError,
							std::format("Cannot get node data \"{}\", component \"{}\" has wrong type: {}.", path, comp_name, ex.what()));
				}
			}
			// continue to the next component
			curr = &jval;

		} else {  // path component doesn't exist
			return hz::Unexpected(AppJsonError::PathNotFound,
					std::format("Cannot get node data \"{}\", component \"{}\" does not exist.", path, comp_name));
		}
	}

	return hz::Unexpected(AppJsonError::InternalError, "Internal error.");
}


/// Get json node data. The path is slash-separated string.
/// If the data is not is found, the default value is returned.
template<typename T>
[[nodiscard]] hz::ExpectedValue<T, AppJsonError> get_node_data(const nlohmann::json& root, const std::string& path, const T& default_value)
{
	auto expected_data = get_node_data<T>(root, path);

	if (!expected_data.has_value()) {
		switch(expected_data.error().data()) {
			case AppJsonError::PathNotFound:
				return default_value;

			case AppJsonError::TypeError:
			case AppJsonError::UnexpectedObjectInPath:
			case AppJsonError::EmptyPath:
			case AppJsonError::InternalError:
				break;
		}
	}

	return expected_data;
}


}



hz::ExpectedVoid<SmartctlParserError> SmartctlAtaJsonParser::parse_full(const std::string& json_data_full)
{
	this->set_data_full(json_data_full);

	if (hz::string_trim_copy(json_data_full).empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Empty string passed as an argument. Returning.\n");
		return hz::Unexpected(SmartctlParserError::EmptyInput, "Smartctl data is empty.");
	}

	nlohmann::json root_node;
	try {
		root_node = nlohmann::json::parse(json_data_full);
	} catch (const nlohmann::json::parse_error& e) {
		debug_out_warn("app", DBG_FUNC_MSG << "Error parsing smartctl output as JSON: " << e.what() << "\n");
		return hz::Unexpected(SmartctlParserError::SyntaxError, std::string("Invalid JSON data: ") + e.what());
	}

	// Version
	std::string smartctl_version;
	{
		auto json_ver = get_node_data<std::vector<int>>(root_node, "smartctl/version");

		if (!json_ver.has_value()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Smartctl version not found in JSON.\n");

			if (json_ver.error().data() == AppJsonError::PathNotFound) {
				return hz::Unexpected(SmartctlParserError::NoVersion, "Smartctl version not found in JSON data.");
			}
			if (json_ver->size() < 2) {
				return hz::Unexpected(SmartctlParserError::DataError, "Error getting smartctl version from JSON data: Not enough version components.");
			}
			return hz::Unexpected(SmartctlParserError::DataError, std::format("Error getting smartctl version from JSON data: {}", json_ver.error().message()));
		}

		smartctl_version = std::format("{}.{}", json_ver->at(0), json_ver->at(1));

		{
			AtaStorageProperty p;
			p.set_name("Smartctl version", "smartctl/version/_merged", "Smartctl Version");
			p.reported_value = smartctl_version;
			p.value = p.reported_value;  // string-type value
			p.section = AtaStorageProperty::Section::info;  // add to info section
			add_property(p);
		}
		{
			AtaStorageProperty p;
			p.set_name("Smartctl version", "smartctl/version/_merged_full", "Smartctl Version");
			p.reported_value = std::format("{}.{} r{} {} {}", json_ver->at(0), json_ver->at(1),
					get_node_data<std::string>(root_node, "smartctl/svn_revision", {}).value_or(std::string()),
					get_node_data<std::string>(root_node, "smartctl/platform_info", {}).value_or(std::string()),
					get_node_data<std::string>(root_node, "smartctl/build_info", {}).value_or(std::string())
			);
			p.value = p.reported_value;  // string-type value
			p.section = AtaStorageProperty::Section::info;  // add to info section
			add_property(p);
		}
		if (!SmartctlVersionParser::check_parsed_version(SmartctlParserType::Json, smartctl_version)) {
			debug_out_warn("app", DBG_FUNC_MSG << "Incompatible smartctl version. Returning.\n");
			return hz::Unexpected(SmartctlParserError::IncompatibleVersion, "Incompatible smartctl version.");
		}
	}


	// Info section
	const std::unordered_map<std::string, std::string> info_keys = {
			{"model_family", _("Model Family")},
			{"model_name", _("Model Name")},
			{"serial_number", _("Serial Number")},
			{"firmware_version", _("Firmware Version")},
			{"user_capacity", _("User Capacity")},
			{"logical_block_size", _("Logical Block Size")},
			{"physical_block_size", _("Physical Block Size")},
			{"rotation_rate", _("Rotation Rate")},
			{"in_smartctl_database", _("In Smartctl Database")},
	};

	for (const auto& [key, displayable_name] : info_keys) {
		auto jval = get_node_data<std::string>(root_node, key);
		// TODO Type error

		if (jval.has_value()) {
			AtaStorageProperty p;
			p.section = AtaStorageProperty::Section::info;
			p.set_name(key, key, displayable_name);
			p.reported_value = jval.value();
			p.value = p.reported_value;  // string-type value

			add_property(p);
		}
	}

	return {};
}





/// @}
