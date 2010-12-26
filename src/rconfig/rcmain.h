/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef RCONFIG_RCMAIN_H
#define RCONFIG_RCMAIN_H

#include <string>

#include "hz/hz_config.h"  // DISABLE_RTTI, RMN_TYPE_TRACKING

// if there's RTTI or type tracking enabled
#if !defined DISABLE_RTTI || defined RMN_TYPE_TRACKING
	#include <stdexcept>  // std::runtime_error
#endif

#include "hz/sync.h"
#include "hz/exceptions.h"  // THROW_FATAL

#include "rmn/resource_node.h"  // resource_node type
#include "rmn/resource_data_any.h"  // any_type data provider
#include "rmn/resource_data_locking.h"  // locking policies for data
#include "rmn/resource_exception.h"  // rmn::no_such_node



namespace rconfig {



typedef rmn::resource_node< rmn::ResourceDataAny<rmn::ResourceSyncPolicyNone> > node_t;
typedef node_t::node_ptr node_ptr;

typedef hz::SyncPolicyMtDefault ConfigLockPolicy;



static const char* const s_config_name = "config";
static const char* const s_default_name = "default";



// Note: C++ standard allows multiple _definitions_ of static class
// template member variables. This means that they may be defined
// in headers, as opposed to cpp files (as with static non-template class
// members and other static variables).
// This gives us opportunity to get rid of the cpp file.


// specify the same type to get the same set of variables.
template<typename Dummy>
struct NodeStaticHolder {
	static node_ptr root_node;  // "/"
	static node_ptr config_node;  // "/config"
	static node_ptr default_node;  // "/default"
	static ConfigLockPolicy::Mutex mutex;  // mutex for static variables above
};

// definitions
template<typename Dummy> node_ptr NodeStaticHolder<Dummy>::root_node = 0;
template<typename Dummy> node_ptr NodeStaticHolder<Dummy>::config_node = 0;
template<typename Dummy> node_ptr NodeStaticHolder<Dummy>::default_node = 0;
template<typename Dummy> ConfigLockPolicy::Mutex NodeStaticHolder<Dummy>::mutex;


typedef NodeStaticHolder<void> RootHolder;  // one (and only) instantiation.



// This is called automatically. This function is thread-safe.
inline bool init_root(bool do_lock = true)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex, do_lock);

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


inline node_ptr get_root()
{
	if (!RootHolder::root_node)
		init_root(false);
	return RootHolder::root_node;
}


inline node_ptr get_config_branch()
{
	if (!RootHolder::root_node)
		init_root(false);
	return RootHolder::config_node;
}


inline node_ptr get_default_branch()
{
	if (!RootHolder::root_node)
		init_root(false);
	return RootHolder::default_node;
}



// --------------------------------- Getting nodes by config path.

// Note: These functions are NOT thread-safe, because they return non-thread-safe
// node structure. (That is, the user may access node_ptr in non-locked environment).
// To use them in a thread-safe environment, you have to lock their access manually.


// Looks in /config, then /default. Useful for searching values (for reading).
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").
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



// same as above, but only for "/config". additionally, you may create it.
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").
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



// same as above, but only for "/default". additionally, you may create it.
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").
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




// --------------------------------- (Thread-safe) Root node manipulation


// Note: This function will clear everything, including /config and /default.
inline void clear_root_all()
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);
	RootHolder::root_node = 0;  // this will delete everything.
}


inline void clear_config_all()
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);
	if (RootHolder::config_node)
		RootHolder::config_node->clear_children();
}


inline void clear_default_all()
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);
	if (RootHolder::default_node)
		RootHolder::default_node->clear_children();
}




// --------------------------------- (Thread-safe) Data manipulation

// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").



// clear data in "/config".
inline void clear_data(const std::string& path)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr cnode = get_config_node(path);
	if (cnode)
		cnode->clear_data();
}


// clear data in "/default".
inline void clear_default_data(const std::string& path)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr cnode = get_default_node(path);
	if (cnode)
		cnode->clear_data();
}



// Set data in "/config".
// Note: This will convert "const char*" to std::string!
template<typename T> inline
bool set_data(const std::string& path, T data)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	// Verify that the default's type matches T,
	// if there's RTTI or type tracking enabled.
#if !defined DISABLE_RTTI || defined RMN_TYPE_TRACKING
	if (!node_t::is_abs_path(path)) {
		node_ptr def_node = get_default_node(path);
		if (def_node && !def_node->data_is_empty()) {  // if exists and not empty
			// template is needed for gcc3.3
			if (!def_node->template data_is_type<T>())
				THROW_FATAL(std::runtime_error(std::string(
						"rconfig::set_data(): Error: Type mismatch between default and config value for \"") + path + "\"!"));
		}
	}
#endif

	node_ptr cnode = get_config_node(path, true);  // auto-create. note that it still may fail.
	if (cnode)
		return cnode->set_data(data);
	return false;
}


// Set data in "/default".
// Note: This will convert "const char*" to std::string!
template<typename T> inline
bool set_default_data(const std::string& path, T data)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr cnode = get_default_node(path, true);  // auto-create. note that it still may fail.
	if (cnode)
		return cnode->set_data(data);
	return false;
}




// --------------------------------- (Thread-safe) Data Reading

// These functions search in "/config" _or_ "/default" only.
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").


// returns false if cast failed or no such node.
template<typename T> inline
bool get_config_data(const std::string& path, T& put_it_here)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_config_node(path);
	if (!node)
		return false;  // no such node
	return node->get_data(put_it_here);
}



// returns false if cast failed or no such node.
template<typename T> inline
bool get_default_data(const std::string& path, T& put_it_here)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_default_node(path);
	if (!node)
		return false;  // no such node
	return node->get_data(put_it_here);
}




// --------------------------------- (Thread-safe) Merged Data Reading

// These functions search in "/config", _then_ in "/default".
// Note: if path is absolute (starts with "/"), then it's looked up in root ("/").



inline bool data_is_empty(const std::string& path)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->data_is_empty();
}


// this function works only if either RTTI or type tracking is enabled
#if !defined DISABLE_RTTI || defined RMN_TYPE_TRACKING
template<typename T> inline
bool data_is_type(const std::string& path)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->data_is_type<T>();
}
#endif


// returns false if cast failed or no such node.
template<typename T> inline
bool get_data(const std::string& path, T& put_it_here)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->get_data(put_it_here);
}


// This function throws if:
// 	* no such node (rmn::no_such_node);
//	* data empty (rmn::empty_data_retrieval);
//	* type mismatch (rmn::type_mismatch).
template<typename T> inline
T get_data(const std::string& path)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_node(path);
	if (!node)
		THROW_FATAL(rmn::no_such_node(path));  // no such node
	return node->get_data<T>();
}


// more loose conversion - can convert between C++ built-in types and std::string.
template<typename T> inline
bool convert_data(const std::string& path, T& put_it_here)  // returns false if cast failed
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_node(path);
	if (!node)
		return false;  // no such node
	return node->convert_data(put_it_here);
}


// This function throws if:
// 	* no such node (rmn::no_such_node);
//	* data empty (rmn::empty_data_retrieval);
//	* type conversion error (rmn::type_convert_error).
template<typename T> inline
T convert_data(const std::string& path)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	node_ptr node = get_node(path);
	if (!node)
		THROW_FATAL(rmn::no_such_node(path));
	return node->convert_data<T>();
}






}  // ns



#endif
