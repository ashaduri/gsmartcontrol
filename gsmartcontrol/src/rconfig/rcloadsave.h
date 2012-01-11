/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef RCONFIG_RCLOADSAVE_H
#define RCONFIG_RCLOADSAVE_H

#include <string>

#include "rmn/resource_serialization.h"  // serialize / unserialize nodes

#include "rcmain.h"



namespace rconfig {



// --------------------------------- save / load "/config" branch (thread-safe)


inline bool load_from_file(const std::string& file)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	return rmn::unserialize_nodes_from_file(get_config_branch(), file);
}



inline bool load_from_string(const std::string& str)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	return rmn::unserialize_nodes_from_string(get_config_branch(), str);
}



#if defined RMN_SERIALIZE_AVAILABLE && RMN_SERIALIZE_AVAILABLE

inline bool save_to_file(const std::string& file)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	return rmn::serialize_node_to_file_recursive(get_config_branch(), file);
}


inline bool save_to_string(std::string& put_here)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	return rmn::serialize_node_to_string_recursive(get_config_branch(), put_here);
}

#else  // no save function

inline bool save_to_file(const std::string& file)
{
	return false;
}


inline bool save_to_string(std::string& put_here)
{
	return false;
}

#endif




}  // ns



#endif
