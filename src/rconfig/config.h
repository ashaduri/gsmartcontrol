/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup rconfig
/// \weakgroup rconfig
/// @{

#ifndef RCONFIG_CONFIG_H
#define RCONFIG_CONFIG_H

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>  // std::runtime_error

#include "hz/debug.h"
#include "hz/string_algo.h"
#include "json/json.hpp"



namespace rconfig {


using json = nlohmann::json;
using namespace std::string_literals;


namespace impl {

	inline std::unique_ptr<json> config_node;  ///< Node for serializable branch
	inline std::unique_ptr<json> default_node;  ///< Node for default branch


	template<typename T>
	inline void set_node_data(json& root, const std::string& path, T&& value)
	{
		std::vector<std::string> components;
		hz::string_split(path, '/', components, true);

		json* curr = &root;
		for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
			const std::string& comp_name = components[comp_index];

			// we can't have non-object values in the middle of a path
			if (!curr->is_object()) {
				throw std::runtime_error("Cannot set node data \""s + path + "\", component \"" + comp_name + "\" is not an object.");
			}
			if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
				json& jval = iter.value();
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					jval = json(std::forward<T>(value));
					break;
				}
				// continue to the next component
				curr = &jval;

			} else {  // path component doesn't exist
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					(*curr)[comp_name] = json(std::forward<T>(value));
					break;
				}
				curr = &((*curr)[comp_name] = json::object());
			}
		}
	}



	template<typename T>
	bool get_node_data(json& root, const std::string& path, T& value)
	{
		std::vector<std::string> components;
		hz::string_split(path, '/', components, true);

		json* curr = &root;
		for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
			const std::string& comp_name = components[comp_index];

			if (!curr->is_object()) {  // we can't have non-object values in the middle of a path
				throw std::runtime_error("Cannot get node data \""s + path + "\", component \"" + comp_name + "\" is not an object.");
			}
			if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
				json& jval = iter.value();
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					value = jval.get<T>();  // may throw json::type_error
					return true;
				}
				// continue to the next component
				curr = &jval;

			} else {  // path component doesn't exist
				break;
			}
		}
		return false;
	}


	inline void unset_node_data(json& root, const std::string& path)
	{
		std::vector<std::string> components;
		hz::string_split(path, '/', components, true);

		json* curr = &root;
		for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
			const std::string& comp_name = components[comp_index];

			if (auto iter = curr->find(comp_name); iter != curr->end()) {  // path component exists
				json& jval = iter.value();
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					curr->erase(comp_name);
					break;
				}
				if (!jval.is_object()) {  // we can't have non-object values in the middle of a path
					debug_out_error("rconfig", "Component \""s + comp_name + "\" in path \"" + path + "\" is not an object, removing it.");
					curr->erase(comp_name);
					break;
				}
				// continue to the next component
				curr = &jval;

			} else {  // path component doesn't exist
				break;
			}
		}
	}


}



/// Clear user config
inline void clear_config()
{
	impl::config_node = std::make_unique<json>(json::object());
}


/// Clear defaults
inline void clear_defaults()
{
	impl::default_node = std::make_unique<json>(json::object());
}


/// Initialize the root node. This is called automatically.
inline bool init_root()
{
	if (impl::config_node)  // already inited
		return false;

	clear_config();
	clear_defaults();

	return true;
}



/// Get the config branch node
inline json& get_config_branch()
{
	init_root();
	return *impl::config_node;
}



/// Get the default branch node
inline json& get_default_branch()
{
	init_root();
	return *impl::default_node;
}



/// Set the data in path
template<typename T>
bool set_data(const std::string& path, T data)
{
	// Since config is loaded from a user file, it may contain some invalid
	// nodes. Don't abort, print warnings.
	try {
		impl::set_node_data(get_config_branch(), path, std::move(data));
	}
	catch (std::exception& e) {
		debug_out_error("rconfig", e.what());
		return false;
	}
	return true;
}



/// Set the default data in path
template<typename T>
void set_default_data(const std::string& path, T data)
{
	// Default data branch must always be valid, so abort if anything is wrong.
	impl::set_node_data(get_default_branch(), path, std::move(data));
}



/// Get the data from config. If no such node exists, look it up in defaults.
template<typename T>
T get_data(const std::string& path)
{
	T data = {};
	bool found = false;
	try {
		// This can possibly throw because the user config file is incorrect
		found = impl::get_node_data(get_config_branch(), path, data);
	}
	catch(std::exception& e) {
		debug_out_error("rconfig", e.what());
	}

	// This can throw only for errors within the program.
	if (!found && !impl::get_node_data(get_default_branch(), path, data)) {
		throw std::runtime_error("No such node: "s + path);
	}
	return data;
}



/// Get the data from defaults.
template<typename T>
T get_default_data(const std::string& path)
{
	T data = {};
	// This can throw only for errors within the program.
	if (!impl::get_node_data(get_default_branch(), path, data)) {
		throw std::runtime_error("No such node: "s + path);
	}
	return data;
}



/// Unset data in config path
inline void unset_data(const std::string& path)
{
	impl::unset_node_data(get_config_branch(), path);
}



/// Dump config to debug output
inline void dump_config(bool print_defaults = false)
{
	debug_begin();
	debug_out_dump("rconfig", "Config:\n" + get_config_branch().dump(4) + "\n");
	debug_end();

	if (print_defaults) {
		debug_begin();
		debug_out_dump("rconfig", "Defaults:\n" + get_default_branch().dump(4) + "\n");
		debug_end();
	}
}




}  // ns



#endif

/// @}
