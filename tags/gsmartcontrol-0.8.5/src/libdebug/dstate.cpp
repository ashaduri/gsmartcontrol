/**************************************************************************
 Copyright:
      (C) 2008 - 2009 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#include <iostream>  // std::cerr, the default output stream
#include <map>

#include "hz/hz_config.h"  // DEBUG_BUILD

#include "hz/exceptions.h"  // THROW_FATAL

#include "dstate.h"
#include "dflags.h"
#include "dchannel.h"
#include "dstream.h"



namespace debug_internal {



	// initialize the "default" template domain, set the default enabled levels / format flags
	void DebugState::setup_default_state()
	{
		// defaults:
		debug_level::type levels_enabled = debug_level::warn | debug_level::error | debug_level::fatal;
#ifdef DEBUG_BUILD
		levels_enabled = debug_level::all;
#endif

		// default format
		debug_format::type format_flags = debug_format::level | debug_format::domain | debug_format::indent;
#ifndef _WIN32
		format_flags |= debug_format::color;
#endif

		std::map<debug_level::flag, bool> levels;
		unsigned long levels_enabled_ulong = levels_enabled.to_ulong();
		levels[debug_level::dump] = (levels_enabled_ulong & debug_level::dump);
		levels[debug_level::info] = (levels_enabled_ulong & debug_level::info);
		levels[debug_level::warn] = (levels_enabled_ulong & debug_level::warn);
		levels[debug_level::error] = (levels_enabled_ulong & debug_level::error);
		levels[debug_level::fatal] = (levels_enabled_ulong & debug_level::fatal);

		domain_map_t& dm = get_domain_map();

		dm["default"] = level_map_t();

		level_map_t& level_map = dm.find("default")->second;

		// we add the same copy to save memory and to ensure proper std::cerr locking.
		debug_channel_base_ptr channel(new DebugChannelOStream(std::cerr));

		for (std::map<debug_level::flag, bool>::const_iterator iter = levels.begin(); iter != levels.end(); ++iter) {
			debug_level::flag level = iter->first;
			level_map[level] = out_stream_ptr(new DebugOutStream(level, "default", format_flags));

			level_map[level]->add_channel(channel);  // add by smartpointer
			level_map[level]->set_enabled(iter->second);
		}

	}



	// this will initialize the default domain and channels automatically.
	static DebugState s_debug_state;


	DebugState& get_debug_state()
	{
		return s_debug_state;
	}


}




// returns false if domain is registered already. It will use the "default"
// domain as a template.
bool debug_register_domain(const std::string& domain)
{
	using namespace debug_internal;

	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (dm.find(domain) != dm.end())  // already exists
		return false;

	// copy the "default" domain - use it as a template
	DebugState::domain_map_t::iterator def_iter = dm.find("default");
	if (def_iter == dm.end()) {
		THROW_FATAL(debug_internal_error(("debug_register_domain(\"" + domain
			+ "\"): Domain \"default\" doesn't exist.").c_str()));
	}

	DebugState::level_map_t& def_level_map = def_iter->second;

	dm[domain] = DebugState::level_map_t();
	DebugState::level_map_t& level_map = dm.find(domain)->second;

	for (DebugState::level_map_t::const_iterator iter = def_level_map.begin(); iter != def_level_map.end(); ++iter) {
		level_map[iter->first] = DebugState::out_stream_ptr(new DebugOutStream(*(iter->second), domain));
	}

	return true;
}



bool debug_unregister_domain(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	dm.erase(found);  // this should clear everything - it's all smartpointers
	return true;
}



std::vector<std::string> debug_get_registered_domains()
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	std::vector<std::string> domains;
	domains.reserve(dm.size());
	for (DebugState::domain_map_t::iterator iter = dm.begin(); iter != dm.end(); ++iter)
		domains.push_back(iter->first);

	return domains;
}




// enable/disable outstreams. domain "all" means all domains.
// multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_set_enabled(const std::string& domain, const debug_level::type& levels, bool enabled)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (DebugState::domain_map_t::iterator iter = dm.begin(); iter != dm.end(); ++iter)
			status = status && debug_set_enabled(iter->first, levels, enabled);
		return status;
	}

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (unsigned int i = 0; i < matched_levels.size(); ++i) {
		found->second[matched_levels[i]]->set_enabled(enabled);
	}

	return true;
}



// see which levels are enabled for domain
debug_level::type debug_get_enabled(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	debug_level::type levels;

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return levels;

	DebugState::level_map_t& level_map = found->second;

	for (DebugState::level_map_t::const_iterator iter = level_map.begin(); iter != level_map.end(); ++iter) {
		if (iter->second->get_enabled())
			levels |= iter->first;
	}

	return levels;
}




// set format flags for domain / level. domain "all" means all domains.
// multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_set_format(const std::string& domain, const debug_level::type& levels, const debug_format::type& format)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (DebugState::domain_map_t::iterator iter = dm.begin(); iter != dm.end(); ++iter)
			status = status && debug_set_format(iter->first, levels, format);
		return status;
	}

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (unsigned int i = 0; i < matched_levels.size(); ++i) {
		found->second[matched_levels[i]]->set_format(format);
	}

	return true;
}



// get all enabled format flags for each level in a domain
std::map<debug_level::flag, debug_format::type> debug_get_formats(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	std::map<debug_level::flag, debug_format::type> formats;

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return formats;

	for (DebugState::level_map_t::const_iterator iter = found->second.begin(); iter != found->second.end(); ++iter)
		formats[iter->first] = iter->second->get_format();

	return formats;
}




// enable/disable outstreams. domain "all" means all domains.
// multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_add_channel(const std::string& domain, const debug_level::type& levels, debug_channel_base_ptr channel)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (DebugState::domain_map_t::iterator iter = dm.begin(); iter != dm.end(); ++iter)
			status = status && debug_add_channel(iter->first, levels, channel);
		return status;
	}

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (unsigned int i = 0; i < matched_levels.size(); ++i) {
		found->second[matched_levels[i]]->add_channel(channel);
	}

	return true;
}




// enable/disable outstreams. domain "all" means all domains.
// multiple levels may be passed (OR'ed), as well as debug_level::all.
bool debug_clear_channels(const std::string& domain, const debug_level::type& levels)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (DebugState::domain_map_t::iterator iter = dm.begin(); iter != dm.end(); ++iter)
			status = status && debug_clear_channels(iter->first, levels);
		return status;
	}

	DebugState::domain_map_t::iterator found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (unsigned int i = 0; i < matched_levels.size(); ++i) {
		found->second[matched_levels[i]]->set_channels(channel_list_t());
	}

	return true;
}






