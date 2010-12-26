/**************************************************************************
 Copyright:
      (C) 2008 - 2009 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef LIBDEBUG_DSTATE_PUB_H
#define LIBDEBUG_DSTATE_PUB_H

#include <string>
#include <map>
#include <vector>

#include "dflags.h"
#include "dchannel.h"




// returns false if domain is registered already. It will use the "default"
// domain as a template.
bool debug_register_domain(const std::string& domain);

bool debug_unregister_domain(const std::string& domain);

std::vector<std::string> debug_get_registered_domains();


bool debug_set_enabled(const std::string& domain, const debug_level::type& levels, bool enabled);

debug_level::type debug_get_enabled(const std::string& domain);


bool debug_set_format(const std::string& domain, const debug_level::type& levels, const debug_format::type& format);

std::map<debug_level::flag, debug_format::type> debug_get_formats(const std::string& domain);


bool debug_add_channel(const std::string& domain, const debug_level::type& levels, debug_channel_base_ptr channel);

bool debug_clear_channels(const std::string& domain, const debug_level::type& levels);








#endif
