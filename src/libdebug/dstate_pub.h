/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#ifndef LIBDEBUG_DSTATE_PUB_H
#define LIBDEBUG_DSTATE_PUB_H

#include <string>
#include <map>
#include <vector>

#include "dflags.h"
#include "dchannel.h"

/**
\file
Public interface of dstate.h
*/


/// Register a libdebug output domain. Domains are like components
/// of an application, and libdebug flags for each one can be manipulated
/// separately. This will use the "default" domain as a template.
/// \return false if the domain is registered already.
bool debug_register_domain(const std::string& domain);

/// Unregister a previously registered domain.
/// \return false if no such domain
bool debug_unregister_domain(const std::string& domain);

/// Get a list of registered domain
std::vector<std::string> debug_get_registered_domains();


/// Enable/disable output streams. Set the domain to "all" for all domains.
/// Multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_set_enabled(const std::string& domain, const debug_level::flags& levels, bool enabled);

/// See which levels are enabled for domain.
debug_level::flags debug_get_enabled(const std::string& domain);


/// Set format flags for domain / level. Set the domain to "all" for all domains.
/// Multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_set_format(const std::string& domain, const debug_level::flags& levels, const debug_format::flags& format);

/// Get all enabled format flags for each level in a domain
std::map<debug_level::flag, debug_format::flags> debug_get_formats(const std::string& domain);


/// Add a new output channel to domain. Set the domain to "all" for all domains.
/// Multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_add_channel(const std::string& domain, const debug_level::flags& levels, const DebugChannelBasePtr& channel);

/// Remove all output channels from domain. Set the domain to "all" for all domains.
/// Multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_clear_channels(const std::string& domain, const debug_level::flags& levels);








#endif

/// @}
