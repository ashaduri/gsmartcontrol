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
#include <set>

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
			if (obj.contains("device") && obj.contains("type") && obj.contains("options")) {
				devmap.value.insert_or_assign(
					{obj.at("device").get<std::string>(), obj.at("type").get<std::string>()},
					obj.at("options").get<std::string>()
				);
			}
		}
		catch(std::exception& e) {
			// ignore "not found"
		}
	}
}



/// Get per-device options from config
inline AppDeviceOptionMap app_config_get_device_option_map()
{
	return rconfig::get_data<AppDeviceOptionMap>("system/smartctl_device_options");
}



/// Read device option map from config and get the options for (dev, type_arg) pair.
inline std::vector<std::string> app_get_device_options(const std::string& dev, const std::string& type_arg)
{
	if (dev.empty())
		return {};

	auto devmap = app_config_get_device_option_map().value;
	if (auto iter = devmap.find(std::pair(dev, type_arg)); iter != devmap.end()) {
		if (iter->second.empty()) {
			return {};
		}
		try {
			return Glib::shell_parse_argv(iter->second);
		}
		catch(Glib::ShellError& e)
		{
			// TODO report error
			return {};
		}
	}
	return {};
}



/// Device information used when manually adding devices
struct AppAddDeviceOption {
	AppAddDeviceOption() = default;

	AppAddDeviceOption(std::string device, std::string type, std::string options)
			: device(std::move(device)), type(std::move(type)), options(std::move(options))
	{ }

	std::string device;  ///< Device name, e.g. /dev/sda
	std::string type;  ///< Smartctl type
	std::string options;  ///< Additional smartctl options


	/// Compare two AppAddDeviceOption objects
	bool operator==(const AppAddDeviceOption& other) const
	{
		return device == other.device && type == other.type && options == other.options;
	}

	/// Compare two AppAddDeviceOption objects
	bool operator!=(const AppAddDeviceOption& other) const
	{
		return !(*this == other);
	}

	/// Compare two AppAddDeviceOption objects
	bool operator<(const AppAddDeviceOption& other) const
	{
		if (device != other.device)
			return device < other.device;
		if (type != other.type)
			return type < other.type;
		return options < other.options;
	}
};



/// json serializer for rconfig
inline void to_json(rconfig::json& j, const std::set<AppAddDeviceOption>& devices)
{
	for (const auto& dev : devices) {
		if (!dev.device.empty()) {
			j.push_back(rconfig::json {
				{"device", dev.device},
				{"type", dev.type},
				{"options", dev.options}
			});
		}
	}
}



/// json deserializer for rconfig
inline void from_json(const rconfig::json& j, std::set<AppAddDeviceOption>& devices)
{
	if (!j.is_array()) {
		return;
	}
	for (const auto& obj : j) {
		try {
			if (obj.contains("device") && obj.contains("type") && obj.contains("options")) {
				devices.emplace(
					obj.at("device").get<std::string>(),
					obj.at("type").get<std::string>(),
					obj.at("options").get<std::string>()
				);
			}
		}
		catch(std::exception& e) {
			// ignore "not found"
		}
	}
}



/// Get devices to manually add on startup
inline std::set<AppAddDeviceOption> app_get_startup_manual_devices()
{
	return rconfig::get_data<std::set<AppAddDeviceOption>>("system/startup_manual_devices");
}



/// Set devices to manually add on startup
inline void app_set_startup_manual_devices(std::set<AppAddDeviceOption> devices)
{
	rconfig::set_data("system/startup_manual_devices", std::move(devices));
}




#endif

/// @}
