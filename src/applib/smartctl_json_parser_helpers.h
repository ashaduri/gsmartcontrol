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


using PropertyRetrievalFunc = std::function<
		auto(const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
				-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError> >;


auto string_formatter()
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


auto bool_formatter(const std::string_view& true_str, const std::string_view& false_str)
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



}  // namespace SmartctlJsonParserHelpers


#endif

/// @}
