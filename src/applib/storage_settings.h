/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_SETTINGS_H
#define STORAGE_SETTINGS_H

#include <string>
#include <map>
#include <utility>

#include "rconfig/rconfig.h"


/// Device name, device type (optional)
using AppDeviceWithType = std::pair<std::string, std::string>;

/// Device name + type -> options.
/// We need a separate struct for this, to work with json (otherwise it gives errors with std::map).
struct AppDeviceOptionMap {
	std::map<AppDeviceWithType, std::string> value;

	bool operator==(const AppDeviceOptionMap& other) const
	{
		return value == other.value;
	}

	bool operator!=(const AppDeviceOptionMap& other) const
	{
		return !(*this == other);
	}
};


/// json serializer for rconfig
inline void to_json(rconfig::json& j, const AppDeviceOptionMap& devmap)
{
	for (const auto& iter : devmap.value) {
		std::string dev = iter.first.first, type = iter.first.second, options = iter.second;
		if (!dev.empty() && !options.empty()) {
			j.push_back(rconfig::json {
				{"device", dev},
				{"type", type},
				{"options", options}
			});
		}
	}
}


/// json deserializer for rconfig
inline void from_json(const rconfig::json& j, AppDeviceOptionMap& devmap)
{
	for (const auto& obj : j) {
		try {
			devmap.value.insert_or_assign(
				{obj.at("device").get<std::string>(), obj.at("type").get<std::string>()},
				obj.at("options").get<std::string>()
			);
		}
		catch(std::exception& e) {
			// ignore "not found"
		}
	}
}



/// Get device option map from config
inline AppDeviceOptionMap app_config_get_device_option_map()
{
	return rconfig::get_data<AppDeviceOptionMap>("system/smartctl_device_options");
}



/// Read device option map from config and get the options for (dev, type_arg) pair.
inline std::string app_get_device_option(const std::string& dev, const std::string& type_arg)
{
	if (dev.empty())
		return std::string();

	auto devmap = app_config_get_device_option_map().value;
	if (auto iter = devmap.find(std::pair(dev, type_arg)); iter != devmap.end()) {
		return iter->second;
	}
	return std::string();
}





#endif

/// @}
