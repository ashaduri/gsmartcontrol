/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
License: Zlib
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug
/// \weakgroup libdebug
/// @{

#include <iostream>  // std::cerr, the default output stream
#include <map>

#include "dstate.h"
#include "dflags.h"
#include "dchannel.h"
#include "dstream.h"



namespace debug_internal {



	void DebugState::setup_default_state() noexcept
	{
		// defaults:
		debug_level::types levels_enabled = debug_level::warn | debug_level::error | debug_level::fatal;
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
		levels[debug_level::dump] = bool(levels_enabled_ulong & debug_level::dump);
		levels[debug_level::info] = bool(levels_enabled_ulong & debug_level::info);
		levels[debug_level::warn] = bool(levels_enabled_ulong & debug_level::warn);
		levels[debug_level::error] = bool(levels_enabled_ulong & debug_level::error);
		levels[debug_level::fatal] = bool(levels_enabled_ulong & debug_level::fatal);

		domain_map_t& dm = get_domain_map();

		dm["default"] = level_map_t();

		level_map_t& level_map = dm.find("default")->second;

		// we add the same copy to save memory and to ensure proper std::cerr locking.
		auto channel = std::make_shared<DebugChannelOStream>(std::cerr);

		for (auto& iter : levels) {
			debug_level::flag level = iter.first;
			level_map[level] = std::make_shared<DebugOutStream>(level, "default", format_flags);

			level_map[level]->add_channel(channel);  // add by smartpointer
			level_map[level]->set_enabled(iter.second);
		}
	}



	/// Global libdebug state.
	/// This will initialize the default domain and channels automatically.
	static DebugState s_debug_state;


	DebugState& get_debug_state()
	{
		return s_debug_state;
	}


}





bool debug_register_domain(const std::string& domain)
{
	using namespace debug_internal;

	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (dm.find(domain) != dm.end())  // already exists
		return false;

	// copy the "default" domain - use it as a template
	auto def_iter = dm.find("default");
	if (def_iter == dm.end()) {
		throw debug_internal_error(("debug_register_domain(\"" + domain
			+ "\"): Domain \"default\" doesn't exist.").c_str());
	}

	DebugState::level_map_t& def_level_map = def_iter->second;

	dm[domain] = DebugState::level_map_t();
	DebugState::level_map_t& level_map = dm.find(domain)->second;

	for (const auto& iter : def_level_map) {
		level_map[iter.first] = std::make_shared<DebugOutStream>(*(iter.second), domain);
	}

	return true;
}



bool debug_unregister_domain(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	auto found = dm.find(domain);
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
	for (const auto& iter : dm)
		domains.push_back(iter.first);

	return domains;
}





bool debug_set_enabled(const std::string& domain, const debug_level::types& levels, bool enabled)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (auto& iter : dm)
			status = status && debug_set_enabled(iter.first, levels, enabled);
		return status;
	}

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (auto matched_level : matched_levels) {
		found->second[matched_level]->set_enabled(enabled);
	}

	return true;
}



debug_level::types debug_get_enabled(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	debug_level::types levels;

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return levels;

	DebugState::level_map_t& level_map = found->second;

	for (const auto& iter : level_map) {
		if (iter.second->get_enabled())
			levels |= iter.first;
	}

	return levels;
}




bool debug_set_format(const std::string& domain, const debug_level::types& levels, const debug_format::type& format)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (const auto& iter : dm)
			status = status && debug_set_format(iter.first, levels, format);
		return status;
	}

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (auto matched_level : matched_levels) {
		found->second[matched_level]->set_format(format);
	}

	return true;
}



std::map<debug_level::flag, debug_format::type> debug_get_formats(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	std::map<debug_level::flag, debug_format::type> formats;

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return formats;

	for (const auto& iter : found->second)
		formats[iter.first] = iter.second->get_format();

	return formats;
}




bool debug_add_channel(const std::string& domain, const debug_level::types& levels, const DebugChannelBasePtr& channel)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (const auto& iter : dm)
			status = status && debug_add_channel(iter.first, levels, channel);
		return status;
	}

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (auto matched_level : matched_levels) {
		found->second[matched_level]->add_channel(channel);
	}

	return true;
}




bool debug_clear_channels(const std::string& domain, const debug_level::types& levels)
{
	using namespace debug_internal;
	DebugState::domain_map_t& dm = get_debug_state().get_domain_map();

	if (domain == "all") {
		bool status = true;
		for (const auto& iter : dm)
			status = status && debug_clear_channels(iter.first, levels);
		return status;
	}

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	std::vector<debug_level::flag> matched_levels;
	debug_level::get_matched_levels_array(levels, matched_levels);
	for (auto matched_level : matched_levels) {
		found->second[matched_level]->set_channels(std::vector<DebugChannelBasePtr>());
	}

	return true;
}







/// @}
