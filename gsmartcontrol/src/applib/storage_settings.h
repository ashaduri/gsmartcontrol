/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_SETTINGS_H
#define STORAGE_SETTINGS_H

#include <string>
#include <vector>
#include <map>

#include "rconfig/config.h"
#include "hz/string_algo.h"
#include "hz/bin2ascii_encoder.h"



/// A map of Device =\> Options
using device_option_map_t = std::map<std::string, std::string>;



/// Unserialize device option map from a string
inline device_option_map_t app_unserialize_device_option_map(const std::string& str)
{
	hz::Bin2AsciiEncoder enc;

	std::vector<std::string> pairs;
	hz::string_split(str, ";", pairs, true);

	device_option_map_t option_map;
	for (const auto& p : pairs) {
		std::vector<std::string> dev_entry;
		hz::string_split(p, ":", dev_entry, true, 2);

		std::string dev, opt;
		if (dev_entry.size() == 2) {
			dev = dev_entry.at(0);  // includes type (separated by encoded "::")
			opt = dev_entry.at(1);
		}

		if (!dev.empty()) {
			dev = hz::string_trim_copy(enc.decode(dev));
			opt = hz::string_trim_copy(enc.decode(opt));

			// ignore potentially harmful chars
			if (!dev.empty() && !opt.empty()  // this discards the entries with empty options
					&& dev.find_first_of(";><|&") == std::string::npos
					&& opt.find_first_of(";><|&") == std::string::npos) {
				option_map[dev] = opt;
			}
		}
	}

	return option_map;
}



/// Serialize device option map from a string (to store it in config file, for example)
inline std::string app_serialize_device_option_map(const device_option_map_t& option_map)
{
	hz::Bin2AsciiEncoder enc;
	std::vector<std::string> pairs;

	for (const auto& iter : option_map) {
		if (!iter.first.empty() && !iter.second.empty())  // discard the ones with empty device name or options
			pairs.push_back(enc.encode(iter.first) + ":" + enc.encode(iter.second));
	}

	return hz::string_join(pairs, ";");
}



/// Read device option map from config and get the options for (dev, type_arg) pair.
inline std::string app_get_device_option(const std::string& dev, const std::string& type_arg)
{
	if (dev.empty())
		return std::string();

	std::string devmap_str = rconfig::get_data<std::string>("system/smartctl_device_options");
	device_option_map_t devmap = app_unserialize_device_option_map(devmap_str);

	// try the concrete type first
	if (!type_arg.empty()) {
		auto iter = devmap.find(dev + "::" + type_arg);
		if (iter != devmap.end()) {
			return iter->second;
		}
	}

	// in case there's a trailing delimiter
	if (auto iter = devmap.find(dev + "::" + type_arg); iter != devmap.end()) {
		return iter->second;
	}

	// just the device name
	if (auto iter = devmap.find(dev); iter != devmap.end()) {
		return iter->second;
	}

	return std::string();
}





#endif

/// @}
