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

#ifndef RCONFIG_RCLOADSAVE_H
#define RCONFIG_RCLOADSAVE_H

#include <string>

#include "rmn/resource_serialization.h"  // serialize / unserialize nodes

#include "rcmain.h"



namespace rconfig {



/// Load the "/config" branch from file.
inline bool load_from_file(const std::string& file)
{
	return rmn::unserialize_nodes_from_file(get_config_branch(), file);
}



/// Load the "/config" branch from string.
inline bool load_from_string(const std::string& str)
{
	return rmn::unserialize_nodes_from_string(get_config_branch(), str);
}



/// Save the "/config" branch to a file.
inline bool save_to_file(const std::string& file)
{
	return rmn::serialize_node_to_file_recursive(get_config_branch(), file);
}


/// Save the "/config" branch to a string.
inline bool save_to_string(std::string& put_here)
{
	return rmn::serialize_node_to_string_recursive(get_config_branch(), put_here);
}





}  // ns



#endif

/// @}
