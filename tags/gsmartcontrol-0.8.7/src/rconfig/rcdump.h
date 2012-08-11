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

#ifndef RCONFIG_RCDUMP_H
#define RCONFIG_RCDUMP_H

#include <string>
#include <iosfwd>  // std::ostream

#include "hz/debug.h"  // debug_*
#include "rmn/resource_node_dump.h"  // node dumper functions

#include "rcmain.h"



namespace rconfig {



/// Dump a config tree to libdebug stream (in displayable format). This function is thread-safe.
inline void dump_tree()
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	debug_begin();
	// includes the trailing newline
	debug_out_dump("rconfig", rmn::resource_node_dump_recursive(get_root()));
	debug_end();
}


/// Dump a config tree to std::ostream (in displayable format). This function is thread-safe.
inline void dump_tree_to_stream(std::ostream& os)
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	debug_begin();
	// includes the trailing newline
	os << rmn::resource_node_dump_recursive(get_root());
	debug_end();
}



/// Dump a config tree to a string (in displayable format). This function is thread-safe.
inline std::string dump_tree_to_string()
{
	ConfigLockPolicy::ScopedLock locker(RootHolder::mutex);

	return rmn::resource_node_dump_recursive(get_root());
}







}  // ns



#endif

/// @}
