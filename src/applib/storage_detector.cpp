/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <ios>  // std::boolalpha
// #include <glibmm/fileutils.h>  // Dir
#include <algorithm>  // std::find

#include "hz/debug.h"
#include "hz/fs_dir.h"
#include "rconfig/rconfig_mini.h"

#include "app_pcrecpp.h"
#include "storage_detector.h"



std::string StorageDetector::detect(std::vector<StorageDeviceRefPtr>& drives)
{
//	std::vector<std::string> all_devices;
// 	try {
// 		Glib::Dir dir("/sys/block");  // lists all block devices which may have filesystems
// 		all_devices.insert(all_devices.begin(), dir.begin(), dir.end());
// 	}
// 	catch (Glib::FileError& e) {
// 		std::string msg = std::string("Error while detecting storage devices: ") + e.what();
// 		debug_out_error("app", DBG_FUNC_MSG << msg << "\n");
// 		return msg;
// 	}
	// hz::Dir dir("/sys/block");


	// this defaults to "/dev/disk/by-id"
	std::string dev_dir;
	if (!rconfig::get_data("system/device_scan_path", dev_dir) || dev_dir.empty()) {
		return "Device scan directory not set.";
	}

	hz::Dir dir(dev_dir);

	std::vector<std::string> all_devices;
	if (!dir.entry_list(all_devices, false)) {
		return dir.get_error_utf8();
	}

	// filter-out the ones with "partN" in them
	std::vector<std::string> devices;
	for (unsigned int i = 0; i < all_devices.size(); ++i) {
		std::string entry = all_devices[i];
		if (entry == "." || entry == ".." || app_pcre_match("/-part[0-9]+$/", entry))
			continue;

		hz::FsPath path = dev_dir + hz::DIR_SEPARATOR_S + all_devices[i];

		// those are usually relative links, so find out where they are pointing to.
		std::string link_dest;
		if (path.get_link_destination(link_dest)) {
			path = (hz::path_is_absolute(link_dest) ? link_dest : (dev_dir + hz::DIR_SEPARATOR_S + link_dest));
			path.compress();  // compress in case there are ../-s.
		}

		// skip number-ended devices, in case we couldn't filter out the scan directory correctly
		// (it's user-settable after all).
		if (app_pcre_match("/[0-9]+$/", path.str()))
			continue;

		if (std::find(devices.begin(), devices.end(), path.str()) == devices.end())  // there may be duplicates
			devices.push_back(path.str());
	}


	for (std::vector<std::string>::const_iterator iter = devices.begin(); iter != devices.end(); ++iter) {
		std::string dev = *iter;

		// try to match against patterns
		for (unsigned int i = 0; i < match_patterns_.size(); i++) {
			// try to match against general filter
			if (!app_pcre_match(match_patterns_[i], dev))
				continue;

			// matched, check the blacklist
			bool blacked = false;
			for (unsigned int j = 0; j < blacklist_patterns_.size(); j++) {
				if (app_pcre_match(blacklist_patterns_[j], dev)) {  // matched the blacklist too
					blacked = true;
					break;
				}
			}

			StorageDeviceRefPtr drive(new StorageDevice(dev));
			debug_out_info("app", "Found device: \"" << drive->get_device() << "\".\n");

			if (!blacked) {
				drives.push_back(drive);
				break;  // no need to match other patters, go to next device

			} else {  // blacklisted
				debug_out_info("app", "Device \"" << drive->get_device() << "\" is blacklisted, ignoring.\n");
				break;  // go to next device
			}
		}

	}

	return std::string();
}



std::string StorageDetector::fetch_basic_data(std::vector<StorageDeviceRefPtr>& drives,
		hz::intrusive_ptr<CmdexSync> smartctl_ex, bool return_first_error)
{
	fetch_data_errors_.clear();
	fetch_data_error_outputs_.clear();

	for (unsigned int i = 0; i < drives.size(); ++i) {
		StorageDeviceRefPtr drive = drives[i];
		debug_out_info("app", "Retrieving basic information about the device...\n");

		smartctl_ex->set_running_msg("Running %s on " + drive->get_device() + "...");

		// don't show any errors here - we don't want a screen flood.
		// no need for gui-based executors here, we already show the message in
		// iconview background (if called from main window)
		std::string error_msg = drive->fetch_basic_data_and_parse(smartctl_ex);

		// normally we skip drives with errors - possibly scsi, etc...
		if (return_first_error && !error_msg.empty())
			return error_msg;

		if (!error_msg.empty()) {
			// use original executor error if present (permits matches by our users).
			// if (!smartctl_ex->get_error_msg().empty())
			//	error_msg = smartctl_ex->get_error_msg();

			fetch_data_errors_.push_back(error_msg);
			fetch_data_error_outputs_.push_back(smartctl_ex->get_stdout_str());
		}

		debug_out_dump("app", "Device information for " << drive->get_device() << ":\n"
				<< "\tModel: " << drive->get_model_name() << "\n"
				<< "\tType: " << StorageDevice::get_type_name(drive->get_type()) << "\n"
				<< "\tSMART status: " << StorageDevice::get_status_name(drive->get_smart_status()) << "\n"
				);

	}

	return std::string();
}



std::string StorageDetector::detect_and_fetch_basic_data(std::vector<StorageDeviceRefPtr>& put_drives_here,
		hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	std::string error_msg = detect(put_drives_here);

	if (error_msg.empty())
		fetch_basic_data(put_drives_here, smartctl_ex);  // ignore its errors, there may be plenty of them.

	return error_msg;
}






