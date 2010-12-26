/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_SETTINGS_H
#define STORAGE_SETTINGS_H

#include <string>
#include <vector>
#include <map>

#include "rconfig/rconfig_mini.h"
#include "hz/string_algo.h"
#include "hz/bin2ascii_encoder.h"




typedef std::map<std::string, std::string> device_option_map_t;



inline device_option_map_t app_unserialize_device_option_map(const std::string& str)
{
	hz::Bin2AsciiEncoder enc;

	std::vector<std::string> pairs;
	hz::string_split(str, ';', pairs, true);

	device_option_map_t option_map;
	for (std::vector<std::string>::const_iterator iter = pairs.begin(); iter != pairs.end(); ++iter) {
		std::vector<std::string> dev_entry;
		hz::string_split(*iter, ':', dev_entry, true);

		if (dev_entry.size() == 2) {
			std::string dev = hz::string_trim_copy(enc.decode(dev_entry[0]));
			std::string opt = hz::string_trim_copy(enc.decode(dev_entry[1]));

			// ignore potentially harmful chars
			if (!dev.empty() && !opt.empty()
					&& dev.find_first_of(";><|&") == std::string::npos
					&& opt.find_first_of(";><|&") == std::string::npos) {
				option_map[dev] = opt;
			}
		}
	}

	return option_map;
}



inline std::string app_serialize_device_option_map(const device_option_map_t& option_map)
{
	hz::Bin2AsciiEncoder enc;
	std::vector<std::string> pairs;

	for (device_option_map_t::const_iterator iter = option_map.begin(); iter != option_map.end(); ++iter) {
		if (!iter->first.empty())
			pairs.push_back(enc.encode(iter->first) + ":" + enc.encode(iter->second));
	}

	return hz::string_join(pairs, ';');
}




inline std::string app_get_device_option(const std::string& dev)
{
	if (dev.empty())
		return std::string();

	std::string devmap_str;
	if (!rconfig::get_data("system/smartctl_device_options", devmap_str))
		return std::string();

	device_option_map_t devmap = app_unserialize_device_option_map(devmap_str);

	device_option_map_t::const_iterator iter = devmap.find(dev);
	if (iter == devmap.end())
		return std::string();

	return iter->second;
}





#endif
