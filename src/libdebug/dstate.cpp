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
		debug_level::flags levels_enabled;
		levels_enabled.set(debug_level::warn);
		levels_enabled.set(debug_level::error);
		levels_enabled.set(debug_level::fatal);
#ifdef DEBUG_BUILD
		levels_enabled.set(debug_level::dump);
		levels_enabled.set(debug_level::info);
#endif

		// default format
		debug_format::flags format_flags;
		format_flags.set(debug_format::level);
		format_flags.set(debug_format::domain);
		format_flags.set(debug_format::indent);
#ifndef _WIN32
		format_flags.set(debug_format::color);
#endif

		std::map<debug_level::flag, bool> levels = {
				{debug_level::dump, levels_enabled.test(debug_level::dump) },
				{debug_level::info, levels_enabled.test(debug_level::info) },
				{debug_level::warn, levels_enabled.test(debug_level::warn) },
				{debug_level::error, levels_enabled.test(debug_level::error) },
				{debug_level::fatal, levels_enabled.test(debug_level::fatal) },
		};

		DomainMap& dm = get_domain_map_ref();

		dm["default"] = {};

		LevelMap& level_map = dm.find("default")->second;

		// we add the same copy to save memory and to ensure proper std::cerr locking.
		auto channel = std::make_shared<DebugChannelOStream>(std::cerr);

		for (const auto& [level, enabled] : levels) {
			level_map[level] = std::make_shared<DebugOutStream>(level, "default", format_flags);

			level_map[level]->add_channel(channel);  // add by smartpointer
			level_map[level]->set_enabled(enabled);
		}
	}



	/// Global libdebug state.
	/// This will initialize the default domain and channels automatically.
	DebugState& get_debug_state_ref()
	{
		static DebugState state;
		return state;
	}


}





bool debug_register_domain(const std::string& domain)
{
	using namespace debug_internal;

	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

	if (dm.find(domain) != dm.end())  // already exists
		return false;

	// copy the "default" domain - use it as a template
	auto def_iter = dm.find("default");
	if (def_iter == dm.end()) {
		throw debug_internal_error(("debug_register_domain(\"" + domain
			+ "\"): Domain \"default\" doesn't exist.").c_str());
	}

	DebugState::LevelMap& def_level_map = def_iter->second;

	dm[domain] = DebugState::LevelMap();
	DebugState::LevelMap& level_map = dm.find(domain)->second;

	for (const auto& iter : def_level_map) {
		level_map[iter.first] = std::make_shared<DebugOutStream>(*(iter.second), domain);
	}

	return true;
}



bool debug_unregister_domain(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return false;

	dm.erase(found);  // this should clear everything - it's all smartpointers
	return true;
}



std::vector<std::string> debug_get_registered_domains()
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

	std::vector<std::string> domains;
	domains.reserve(dm.size());
	for (const auto& iter : dm)
		domains.push_back(iter.first);

	return domains;
}





bool debug_set_enabled(const std::string& domain, const debug_level::flags& levels, bool enabled)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

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



debug_level::flags debug_get_enabled(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

	debug_level::flags levels;

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exist
		return levels;

	DebugState::LevelMap& level_map = found->second;

	for (const auto& [level, stream] : level_map) {
		if (stream->get_enabled())
			levels.set(level);
	}

	return levels;
}




bool debug_set_format(const std::string& domain, const debug_level::flags& levels, const debug_format::flags& format)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

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



std::map<debug_level::flag, debug_format::flags> debug_get_formats(const std::string& domain)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

	std::map<debug_level::flag, debug_format::flags> formats;

	auto found = dm.find(domain);
	if (found == dm.end())  // doesn't exists
		return formats;

	for (const auto& iter : found->second)
		formats[iter.first] = iter.second->get_format();

	return formats;
}




bool debug_add_channel(const std::string& domain, const debug_level::flags& levels, const DebugChannelBasePtr& channel)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

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




bool debug_clear_channels(const std::string& domain, const debug_level::flags& levels)
{
	using namespace debug_internal;
	DebugState::DomainMap& dm = get_debug_state_ref().get_domain_map_ref();

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
