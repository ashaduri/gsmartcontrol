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

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "hz/debug.h"
#include "hz/string_algo.h"
#include "smartctl_parser_types.h"
#include "smartctl_version_parser.h"
#include "hz/format_unit.h"
#include "hz/error_container.h"
#include "storage_property.h"



enum class SmartctlJsonParserError {
	UnexpectedObjectInPath,
	PathNotFound,
	TypeError,
	EmptyPath,
	InternalError,
};



namespace SmartctlJsonParserHelpers {


/// Get node from json data. The path is slash-separated string.
[[nodiscard]] inline hz::ExpectedValue<nlohmann::json, SmartctlJsonParserError>
get_node(const nlohmann::json& root, std::string_view path)
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
					fmt::format("Cannot get node data \"{}\", component \"{}\" is not an object.", path, comp_name));
		}
		if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
			const auto& jval = iter.value();
			if (comp_index + 1 == components.size()) {  // it's the "value" component
				return jval;
			}
			// continue to the next component
			curr = &jval;

		} else {  // path component doesn't exist
			return hz::Unexpected(SmartctlJsonParserError::PathNotFound,
					fmt::format("Cannot get node data \"{}\", component \"{}\" does not exist.", path, comp_name));
		}
	}

	return hz::Unexpected(SmartctlJsonParserError::InternalError, "Internal error.");
}




/// Get json node data. The path is slash-separated string.
/// \return SmartctlJsonParserError on error.
template<typename T>
[[nodiscard]] hz::ExpectedValue<T, SmartctlJsonParserError> get_node_data(const nlohmann::json& root, std::string_view path)
{
	auto node_result = get_node(root, path);
	if (!node_result) {
		return hz::UnexpectedFrom(node_result);
	}

	try {
		return node_result.value().get<T>();  // may throw json::type_error
	}
	catch (nlohmann::json::type_error& ex) {
		return hz::Unexpected(SmartctlJsonParserError::TypeError,
				fmt::format("Cannot get node data \"{}\", component has wrong type: {}.", path, ex.what()));
	}
}



/// Get json node data. The path is slash-separated string.
/// If the data is not is found, the default value is returned.
template<typename T>
[[nodiscard]] hz::ExpectedValue<T, SmartctlJsonParserError> get_node_data(const nlohmann::json& root, std::string_view path, const T& default_value)
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



/// Check if json node exists. The path is slash-separated string.
[[nodiscard]] inline hz::ExpectedValue<bool, SmartctlJsonParserError>
get_node_exists(const nlohmann::json& root, std::string_view path)
{
	auto node_result = get_node(root, path);
	if (node_result.has_value()) {
		return true;
	}

	switch (node_result.error().data()) {
		case SmartctlJsonParserError::PathNotFound:
			return false;

		case SmartctlJsonParserError::UnexpectedObjectInPath:
		case SmartctlJsonParserError::EmptyPath:
		case SmartctlJsonParserError::InternalError:
		case SmartctlJsonParserError::TypeError:
			break;
	}

	return hz::UnexpectedFrom(node_result);
}



/// A signature for a property retrieval function.
using PropertyRetrievalFunc = std::function<
		auto(const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
				-> hz::ExpectedValue<StorageProperty, SmartctlParserError> >;



/// Return a lambda which retrieves a key value as a string, and sets it as a property.
inline auto string_formatter()
{
	return [](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
			-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<std::string>(root_node, key); jval) {
			StorageProperty p;
			p.set_name(key, displayable_name);
			// p.reported_value = jval.value();
			p.readable_value = jval.value();
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
	};
}



/// Return a lambda which returns a return_property if conditional_path exists.
/// If the path doesn't exist, an error is returned.
inline auto conditional_formatter(const std::string_view conditional_path, StorageProperty return_property)
{
	return [conditional_path, return_property](const nlohmann::json& root_node, const std::string& key, [[maybe_unused]] const std::string& displayable_name) mutable
			-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
	{
		auto node_exists_result = get_node_exists(root_node, conditional_path);
		if (!node_exists_result.has_value()) {
			return hz::Unexpected(SmartctlParserError::DataError, node_exists_result.error().message());
		}

		if (node_exists_result.value()) {
			return_property.generic_name = key;
			return_property.displayable_name = displayable_name;
			return return_property;
		}

		return hz::Unexpected(SmartctlParserError::InternalError, fmt::format("Error getting key {} from JSON data.", key));
	};
}



/// Return a lambda which retrieves a key value as a bool (formatted according to parameters), and sets it as a property.
inline auto bool_formatter(const std::string_view& true_str, const std::string_view& false_str)
{
	return [true_str, false_str](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
		-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<bool>(root_node, key); jval) {
			StorageProperty p;
			p.set_name(key, displayable_name);
			// p.reported_value = (jval.value() ? true_str : false_str);
			p.readable_value = (jval.value() ? true_str : false_str);
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
	};
}



/// Return a lambda which retrieves a key value as an integer of type IntegerType
/// and formats it using locale, placing it in format_string.
template<typename IntegerType>
auto integer_formatter(const std::string& format_string = "{}")
{
	return [format_string](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
		-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<IntegerType>(root_node, key); jval) {
			StorageProperty p;
			p.set_name(key, displayable_name);
			// p.reported_value = (jval.value() ? true_str : false_str);
			std::string num_str = hz::number_to_string_locale(jval.value());
			p.readable_value = fmt::format(fmt::runtime(format_string), num_str);
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
	};
}



/// Return a lambda which retrieves a key value as a string (formatted using another lambda), and sets it as a property.
template<typename Type>
auto custom_string_formatter(std::function<std::string(Type value)> formatter)
{
	return [formatter](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
			-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
	{
		if (auto jval = get_node_data<Type>(root_node, key); jval) {
			StorageProperty p;
			p.set_name(key, displayable_name);
			// p.reported_value = formatter(jval.value());
			p.readable_value = formatter(jval.value());
			p.value = jval.value();
			return p;
		}
		return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
	};
}



/// Parse version from json output, returning 2 properties.
[[nodiscard]] inline hz::ExpectedVoid<SmartctlParserError> parse_version(const nlohmann::json& json_root_node,
		StorageProperty& merged_property, StorageProperty& full_property)
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
		return hz::Unexpected(SmartctlParserError::DataError, fmt::format("Error getting smartctl version from JSON data: {}", json_ver.error().message()));
	}

	smartctl_version = fmt::format("{}.{}", json_ver->at(0), json_ver->at(1));

	{
		merged_property.set_name("smartctl/version/_merged", _("Smartctl Version"));
		// p.reported_value = smartctl_version;
		merged_property.readable_value = smartctl_version;
		merged_property.value = smartctl_version;  // string-type value
		merged_property.section = StoragePropertySection::Info;  // add to info section
	}
	{
		full_property.set_name("smartctl/version/_merged_full", _("Smartctl Version"));
		full_property.readable_value = fmt::format("{}.{} r{} {} {}", json_ver->at(0), json_ver->at(1),
				get_node_data<std::string>(json_root_node, "smartctl/svn_revision", {}).value_or(std::string()),
				get_node_data<std::string>(json_root_node, "smartctl/platform_info", {}).value_or(std::string()),
				get_node_data<std::string>(json_root_node, "smartctl/build_info", {}).value_or(std::string())
		);
		full_property.value = full_property.readable_value;  // string-type value
		full_property.section = StoragePropertySection::Info;  // add to info section
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
