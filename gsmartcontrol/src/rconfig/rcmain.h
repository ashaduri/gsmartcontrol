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

#ifndef RCONFIG_RCMAIN_H
#define RCONFIG_RCMAIN_H

#include <string>
#include <stdexcept>  // std::runtime_error

#include "hz/hz_config.h"
#include "rmn/resource_node.h"  // resource_node type
#include "rmn/resource_data_any.h"  // any_type data provider
#include "rmn/resource_data_locking.h"  // locking policies for data
#include "rmn/resource_exception.h"  // rmn::no_such_node



namespace rconfig {



/// Rconfig node type
typedef rmn::resource_node<rmn::ResourceDataAny> node_t;

/// Rconfig strong reference-holding node pointer
typedef node_t::node_ptr node_ptr;


/// Config branch for serializable values ("/config")
static const char* const s_config_name = "config";

/// Config branch for default config values ("/default")
static const char* const s_default_name = "default";



// Note: C++ standard allows multiple _definitions_ of static class
// template member variables. This means that they may be defined
// in headers, as opposed to cpp files (as with static non-template class
// members and other static variables).
// This gives us opportunity to get rid of the cpp file.


/// Static variable holder
template<typename Dummy>
struct NodeStaticHolder {
	static node_ptr root_node;  ///< Node for "/"
	static node_ptr config_node;  ///< Node for "/config"
	static node_ptr default_node;  ///< Node for "/default"
};

// definitions
template<typename Dummy> node_ptr NodeStaticHolder<Dummy>::root_node = 0;
template<typename Dummy> node_ptr NodeStaticHolder<Dummy>::config_node = 0;
template<typename Dummy> node_ptr NodeStaticHolder<Dummy>::default_node = 0;


// Specify the same template parameter to get the same set of variables.
typedef NodeStaticHolder<void> RootHolder;  ///< Holder for static variables (one (and only) instantiation).



/// Initialize the root node. This is called automatically.
/// This function is thread-safe.
inline bool init_root()
{
	// Note: Do NOT check every node individually. This will break handling
	// of absolute paths, e.g. get_data("/default/...") will check for existance of "/config".
	if (RootHolder::root_node)  // already inited
		return false;

	// "/"
	RootHolder::root_node = node_ptr(new node_t);
	RootHolder::root_node->set_name("/");

	// "/config"
	RootHolder::config_node = node_ptr(new node_t);
	RootHolder::config_node->set_name(s_config_name);
	RootHolder::root_node->add_child(RootHolder::config_node);

	// "/default"
	RootHolder::default_node = node_ptr(new node_t);
	RootHolder::default_node->set_name(s_default_name);
	RootHolder::root_node->add_child(RootHolder::default_node);

	return true;
}





// --------------------------------- Get root, config, default branches of rconfig tree.

// Note: These functions are NOT thread-safe, because they return non-thread-safe
// node structure. (That is, the user may access node_ptr in non-locked environment).
// To use them in a thread-safe environment, you have to lock their access manually.


/// Get the root node.
inline node_ptr get_root()
{
	if (!RootHolder::root_node)
		init_root();
	return RootHolder::root_node;
}


/// Get the config branch node
inline node_ptr get_config_branch()
{
	if (!RootHolder::root_node)
		init_root();
	return RootHolder::config_node;
}


/// Get the default branch node
inline node_ptr get_default_branch()
{
	if (!RootHolder::root_node)
		init_root();
	return RootHolder::default_node;
}



// --------------------------------- Getting nodes by config path.


/// Get a node by path (relative or absolute). If relative, look in /config, then /default.
/// Useful for searching values (for reading).
inline node_ptr get_node(const std::string& path)
{
	if (node_t::is_abs_path(path)) {  // absolute path, start from root
		node_ptr node = get_root()->find_node(path);
		if (node)
			return node;
		return node_ptr(0);  // not found
	}

	// search by relative path in /config
	node_ptr cnode = get_config_branch()->find_node(path);
	if (cnode)
		return cnode;

	// search by relative path in /default
	node_ptr dnode = get_default_branch()->find_node(path);
	if (dnode)
		return dnode;

	return node_ptr(0);  // not found
}



/// Get a node by path (relative or absolute). If relative, look in /config.
/// If the path doesn't exist, it can be created.
inline node_ptr get_config_node(std::string path, bool create_if_not_exists = false)
{
	if (node_t::is_abs_path(path)) {  // absolute path, start from root
		node_ptr node = get_root()->find_node(path);
		if (node)
			return node;
		if (create_if_not_exists) {
			if (get_root()->build_nodes(path))
				return get_root()->find_node(path);
		}

		return node_ptr(0);  // not found
	}

	// start from /config/
// 	return get_config_node(std::string(rmn::PATH_DELIMITER_S)
// 		+ s_config_name + rmn::PATH_DELIMITER_S + path);

	node_ptr node = get_config_branch()->find_node(path);
	if (node)
		return node;
	if (create_if_not_exists) {
		if (get_config_branch()->build_nodes(path))
			return get_config_branch()->find_node(path);
	}

	return node_ptr(0);  // not found
}



/// Get a node by path (relative or absolute). If relative, look in /default.
/// If the path doesn't exist, it can be created.
inline node_ptr get_default_node(std::string path, bool create_if_not_exists = false)
{
	if (node_t::is_abs_path(path)) {  // absolute path, start from root
		node_ptr node = get_root()->find_node(path);
		if (node)
			return node;
		if (create_if_not_exists) {
			if (get_root()->build_nodes(path))
				return get_root()->find_node(path);
		}

		return node_ptr(0);  // not found
	}

	// start from /default/
// 	return get_default_node(std::string(rmn::PATH_DELIMITER_S)
// 		+ s_default_name + rmn::PATH_DELIMITER_S + path);

	node_ptr node = get_default_branch()->find_node(path);
	if (node)
		return node;
	if (create_if_not_exists) {
		if (get_default_branch()->build_nodes(path))
			return get_default_branch()->find_node(path);
	}

	return node_ptr(0);  // not found
}




// --------------------------------- Root node manipulation


/// Clear everything, including /config and /default.
inline void clear_root_all()
{
	RootHolder::root_node = 0;  // this will delete everything.
}


/// Clear /config
inline void clear_config_all()
{
	if (RootHolder::config_node)
		RootHolder::config_node->clear_children();
}


/// Clear /default
inline void clear_default_all()
{
	if (RootHolder::default_node)
		RootHolder::default_node->clear_children();
}




// --------------------------------- Data manipulation

// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").



/// Clear the data in path (the node becomes empty), or "/config" if the path is relative.
inline void clear_data(const std::string& path)
{
	node_ptr cnode = get_config_node(path);
	if (cnode)
		cnode->clear_data();
}


/// Clear the data in path (the node becomes empty), or "/default" if the path is relative.
inline void clear_default_data(const std::string& path)
{
	node_ptr cnode = get_default_node(path);
	if (cnode)
		cnode->clear_data();
}



/// Set the data in path, or "/config" if the path is relative.
/// Note: This will convert "const char*" data to std::string.
template<typename T> inline
bool set_data(const std::string& path, T data)
{
	// Verify that the default's type matches T
	if (!node_t::is_abs_path(path)) {
		node_ptr def_node = get_default_node(path);
		if (def_node && !def_node->data_is_empty()) {  // if exists and not empty
			// template is needed for gcc3.3
			if (!def_node->template data_is_type<T>())
				throw std::runtime_error(std::string(
						"rconfig::set_data(): Error: Type mismatch between default and config value for \"") + path + "\"!");
		}
	}

	node_ptr cnode = get_config_node(path, true);  // auto-create. note that it still may fail.
	if (cnode)
		return cnode->set_data(data);
	return false;
}



/// Set the data in path, or "/default" if the path is relative.
/// Note: This will convert "const char*" data to std::string.
template<typename T> inline
bool set_default_data(const std::string& path, T data)
{
	node_ptr cnode = get_default_node(path, true);  // auto-create. note that it still may fail.
	if (cnode)
		return cnode->set_data(data);
	return false;
}




// --------------------------------- Data Reading

// These functions search in "/config" _or_ "/default" only.
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").


/// Get the data in path, or "/config" if the path is relative.
/// \return false if cast failed or no such node.
template<typename T> inline
bool get_config_data(const std::string& path, T& put_it_here)
{
	node_ptr node = get_config_node(path);
	if (!node)
		return false;  // no such node
	return node->get_data(put_it_here);
}



/// Get the data in path, or "/default" if the path is relative.
/// \return false if cast failed or no such node.
template<typename T> inline
bool get_default_data(const std::string& path, T& put_it_here)
{
	node_ptr node = get_default_node(path);
	if (!node)
		return false;  // no such node
	return node->get_data(put_it_here);
}




// --------------------------------- Merged Data Reading

// These functions search in "/config", _then_ in "/default".
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").



/// Check if data at path is empty. If the path is relative, look in /config, then /default.
inline bool data_is_empty(const std::string& path)
{
	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->data_is_empty();
}


/// Check if data at path is of type \c T. If the path is relative, look in /config, then /default.
/// This function works only if either RTTI or type tracking is enabled
template<typename T> inline
bool data_is_type(const std::string& path)
{
	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->data_is_type<T>();
}



/// Get data at path. If the path is relative, look in /config, then /default.
/// \return false if cast failed or no such node.
template<typename T> inline
bool get_data(const std::string& path, T& put_it_here)
{
	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->get_data(put_it_here);
}



/// Get data at path. If the path is relative, look in /config, then /default.
/// \throw rmn::no_such_node No such node
/// \throw rmn::empty_data_retrieval Data is empty
/// \throw rmn::type_mismatch Type mismatch
template<typename T> inline
T get_data(const std::string& path)
{
	node_ptr node = get_node(path);
	if (!node)
		throw rmn::no_such_node(path);  // no such node
	return node->get_data<T>();
}






}  // ns



#endif

/// @}
