/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include "json/picojson.h"



namespace rconfig {


namespace impl {

	inline std::unique_ptr<picojson::value> config_node;  ///< Node for serializable branch
	inline std::unique_ptr<picojson::value> default_node;  ///< Node for default branch


	template<typename T>
	inline void set_node_data(picojson::value& root, const std::string& path, T&& value)
	{
		std::vector<std::string> components;
		hz::string_split(path, '/', components, true);

		picojson::value* curr = &root;
		for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
			const std::string& comp_name = components[comp_index];

			// we can't have non-object values in the middle of a path
			if (!curr->is<picojson::object>()) {
				throw std::runtime_error(std::string("Cannot set node data \"" + path + "\", component \"" + comp_name + "\" is not an object."));
			}
			auto& curr_obj = curr->get<picojson::object>();
			if (auto iter = curr_obj.find(comp_name); iter != curr_obj.end()) {  // path component exists
				picojson::value& jval = iter->second;
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					jval = picojson::value(std::forward<T>(value));
					break;
				}
				// continue to the next component
				curr = &jval;

			} else {  // path component doesn't exist
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					curr->get<picojson::object>()[comp_name] = picojson::value(std::forward<T>(value));
					break;
				}
				curr = &(curr->get<picojson::object>()[comp_name] = picojson::value(picojson::object()));
			}
		}
	}



	template<typename T>
	bool get_node_data(picojson::value& root, const std::string& path, T& value)
	{
		std::vector<std::string> components;
		hz::string_split(path, '/', components, true);

		picojson::value* curr = &root;
		for (std::size_t comp_index = 0; comp_index < components.size(); ++comp_index) {
			const std::string& comp_name = components[comp_index];

			if (!curr->is<picojson::object>()) {  // we can't have non-object values in the middle of a path
				throw std::runtime_error(std::string("Cannot get node data \"" + path + "\", component \"" + comp_name + "\" is not an object."));
			}
			auto& curr_obj = curr->get<picojson::object>();
			if (auto iter = curr_obj.find(comp_name); iter != curr_obj.end()) {  // path component exists
				picojson::value& jval = iter->second;
				if (comp_index + 1 == components.size()) {  // it's the "value" component
					if (!jval.is<T>()) {
						throw std::runtime_error(std::string("Cannot get node data \"" + path + "\", type mismatch."));
					}
					value = jval.get<T>();
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


}



/// Clear user config
inline void clear_config()
{
	impl::config_node = std::make_unique<picojson::value>(picojson::object());
}


/// Clear defaults
inline void clear_defaults()
{
	impl::default_node = std::make_unique<picojson::value>(picojson::object());
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
inline picojson::value& get_config_branch()
{
	init_root();
	return *impl::config_node;
}



/// Get the default branch node
inline picojson::value& get_default_branch()
{
	init_root();
	return *impl::default_node;
}



/// Set the data in path
template<typename T>
void set_data(const std::string& path, T data)
{
	impl::set_node_data(get_config_branch(), path, std::move(data));
}



/// picojson has only int64_t, add int for convenience.
template<>
inline void set_data<int>(const std::string& path, int data)
{
	set_data<int64_t>(path, int64_t(data));
}



/// Set the default data in path
template<typename T>
void set_default_data(const std::string& path, T data)
{
	impl::set_node_data(get_default_branch(), path, std::move(data));
}



/// picojson has only int64_t, add int for convenience.
template<>
inline void set_default_data<int>(const std::string& path, int data)
{
	set_default_data<int64_t>(path, int64_t(data));
}



/// Get the data from config. If no such node exists, look it up in defaults.
template<typename T>
T get_data(const std::string& path)
{
	T data = {};
	if (!impl::get_node_data(get_config_branch(), path, data)) {
		if (!impl::get_node_data(get_default_branch(), path, data)) {
			throw std::runtime_error(std::string("No such node: ") + path);
		}
	}
	return data;
}


/// picojson has only int64_t, add int for convenience.
template<>
inline int get_data<int>(const std::string& path)
{
	return static_cast<int>(get_data<int64_t>(path));
}



/// Get the data from defaults.
template<typename T>
T get_default_data(const std::string& path)
{
	T data = {};
	if (!impl::get_node_data(get_default_branch(), path, data)) {
		throw std::runtime_error(std::string("No such node: ") + path);
	}
	return data;
}


/// picojson has only int64_t, add int for convenience.
template<>
inline int get_default_data<int>(const std::string& path)
{
	return static_cast<int>(get_default_data<int64_t>(path));
}



inline void dump_config()
{
	debug_begin();
	debug_out_dump("rconfig", "Config:\n" + get_config_branch().serialize() + "\n");
	debug_end();

	debug_begin();
	debug_out_dump("rconfig", "Defaults:\n" + get_default_branch().serialize() + "\n");
	debug_end();
}




}  // ns



#endif

/// @}
