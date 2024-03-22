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

#ifndef SMARTCTL_JSON_PARSER_HELPERS_H
#define SMARTCTL_JSON_PARSER_HELPERS_H

#include <string>
#include <vector>

#include "json/json.hpp"
#include "hz/debug.h"
#include "hz/string_algo.h"
#include "smartctl_version_parser.h"
#include "hz/format_unit.h"
#include "hz/error_container.h"
#include "ata_storage_property.h"



enum class SmartctlJsonParserError {
	UnexpectedObjectInPath,
	PathNotFound,
	TypeError,
	EmptyPath,
	InternalError,
};



namespace SmartctlJsonParserHelpers {


/// Get json node data. The path is slash-separated string.
/// \throws std::runtime_error If not found or one of the paths is not an object
template<typename T>
[[nodiscard]] hz::ExpectedValue<T, SmartctlJsonParserError> get_node_data(const nlohmann::json& root, const std::string& path)
{
	using namespace std::literals;

	std::vector<std::string> components;
	hz::string_split(path, '/', components, true);

	if (components.empty()) {
		return hz::Unexpected(SmartctlJsonParserError::EmptyPath, "Cannot get node data: Empty path.");
	}

	const auto* curr = &root;
	for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
		const std::string& comp_name = components[comp_index];

		if (!curr->is_object()) {  // we can't have non-object values in the middle of a path
			return hz::Unexpected(SmartctlJsonParserError::UnexpectedObjectInPath,
					std::format("Cannot get node data \"{}\", component \"{}\" is not an object.", path, comp_name));
		}
		if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
			const auto& jval = iter.value();
			if (comp_index + 1 == components.size()) {  // it's the "value" component
				try {
					return jval.get<T>();  // may throw json::type_error
				}
				catch (nlohmann::json::type_error& ex) {
					return hz::Unexpected(SmartctlJsonParserError::TypeError,
							std::format("Cannot get node data \"{}\", component \"{}\" has wrong type: {}.", path, comp_name, ex.what()));
				}
			}
			// continue to the next component
			curr = &jval;

		} else {  // path component doesn't exist
			return hz::Unexpected(SmartctlJsonParserError::PathNotFound,
					std::format("Cannot get node data \"{}\", component \"{}\" does not exist.", path, comp_name));
		}
	}

	return hz::Unexpected(SmartctlJsonParserError::InternalError, "Internal error.");
}



/// Get json node data. The path is slash-separated string.
/// If the data is not is found, the default value is returned.
template<typename T>
[[nodiscard]] hz::ExpectedValue<T, SmartctlJsonParserError> get_node_data(const nlohmann::json& root, const std::string& path, const T& default_value)
{
	auto expected_data = get_node_data<T>(root, path);

	if (!expected_data.has_value()) {
		switch(expected_data.error().data()) {
			case SmartctlJsonParserError::PathNotFound:
				return default_value;

			case SmartctlJsonParserError::TypeError:
			case SmartctlJsonParserError::UnexpectedObjectInPath:
			case SmartctlJsonParserError::EmptyPath:
			case SmartctlJsonParserError::InternalError:
				break;
		}
	}

	return expected_data;
}



/// A signature for a property retrieval function.
using PropertyRetrievalFunc = std::function<
		auto(const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
				-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError> >;



/// Return a lambda which retrieves a key value as a string, and sets it as a property.
inline auto string_formatter()
{
	return [](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
			-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<std::string>(root_node, key); jval) {
			AtaStorageProperty p;
			p.set_name(key, key, displayable_name);
			// p.reported_value = jval.value();
			p.readable_value = jval.value();
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
	};
}



/// Return a lambda which retrieves a key value as a bool (formatted according to parameters), and sets it as a property.
inline auto bool_formatter(const std::string_view& true_str, const std::string_view& false_str)
{
	return [true_str, false_str](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
		-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<bool>(root_node, key); jval) {
			AtaStorageProperty p;
			p.set_name(key, key, displayable_name);
			// p.reported_value = (jval.value() ? true_str : false_str);
			p.readable_value = (jval.value() ? true_str : false_str);
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
	};
}



/// Return a lambda which retrieves a key value as a string (formatted using another lambda), and sets it as a property.
template<typename Type>
auto custom_string_formatter(std::function<std::string(Type value)> formatter)
{
	return [formatter](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
			-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<Type>(root_node, key); jval) {
			AtaStorageProperty p;
			p.set_name(key, key, displayable_name);
			// p.reported_value = formatter(jval.value());
			p.readable_value = formatter(jval.value());
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
	};
}



/// Parse version from json output, returning 2 properties.
[[nodiscard]] inline hz::ExpectedVoid<SmartctlParserError> parse_version(const nlohmann::json& json_root_node,
		AtaStorageProperty& merged_property, AtaStorageProperty& full_property)
{
	using namespace SmartctlJsonParserHelpers;

	std::string smartctl_version;

	auto json_ver = get_node_data<std::vector<int>>(json_root_node, "smartctl/version");

	if (!json_ver.has_value()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Smartctl version not found in JSON.\n");

		if (json_ver.error().data() == SmartctlJsonParserError::PathNotFound) {
			return hz::Unexpected(SmartctlParserError::NoVersion, "Smartctl version not found in JSON data.");
		}
		if (json_ver->size() < 2) {
			return hz::Unexpected(SmartctlParserError::DataError, "Error getting smartctl version from JSON data: Not enough version components.");
		}
		return hz::Unexpected(SmartctlParserError::DataError, std::format("Error getting smartctl version from JSON data: {}", json_ver.error().message()));
	}

	smartctl_version = std::format("{}.{}", json_ver->at(0), json_ver->at(1));

	{
		merged_property.set_name("Smartctl version", "smartctl/version/_merged", "Smartctl Version");
		// p.reported_value = smartctl_version;
		merged_property.readable_value = smartctl_version;
		merged_property.value = smartctl_version;  // string-type value
		merged_property.section = AtaStorageProperty::Section::Info;  // add to info section
	}
	{
		full_property.set_name("Smartctl version", "smartctl/version/_merged_full", "Smartctl Version");
		full_property.readable_value = std::format("{}.{} r{} {} {}", json_ver->at(0), json_ver->at(1),
				get_node_data<std::string>(json_root_node, "smartctl/svn_revision", {}).value_or(std::string()),
				get_node_data<std::string>(json_root_node, "smartctl/platform_info", {}).value_or(std::string()),
				get_node_data<std::string>(json_root_node, "smartctl/build_info", {}).value_or(std::string())
		);
		full_property.value = full_property.readable_value;  // string-type value
		full_property.section = AtaStorageProperty::Section::Info;  // add to info section
	}
	if (!SmartctlVersionParser::check_format_supported(SmartctlOutputFormat::Json, smartctl_version)) {
		debug_out_warn("app", DBG_FUNC_MSG << "Incompatible smartctl version. Returning.\n");
		return hz::Unexpected(SmartctlParserError::IncompatibleVersion, "Incompatible smartctl version.");
	}

	return {};
}




}  // namespace SmartctlJsonParserHelpers


#endif

/// @}
