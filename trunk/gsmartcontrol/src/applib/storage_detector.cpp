/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include "hz/hz_config.h"  // CONFIG_*

#include "hz/debug.h"

#include "app_pcrecpp.h"
#include "smartctl_executor.h"
#include "storage_detector.h"

#include "storage_detector_linux.h"
#include "storage_detector_win32.h"
#include "storage_detector_other.h"




std::string StorageDetector::detect(std::vector<StorageDeviceRefPtr>& drives)
{
	debug_out_info("app", DBG_FUNC_MSG << "Starting drive detection.\n");

	std::vector<std::string> devices;
	std::string error_msg;
	bool found = false;

	// Try each one and move to next if it fails.

#if defined CONFIG_KERNEL_LINUX

	// Disable by-id detection - it's unreliable on broken systems.
	// For example, on Ubuntu 8.04, /dev/disk/by-id contains two device
	// links for two drives, but both point to the same sdb (instead of
	// sda and sdb). Plus, there are no "*-partN" files (not that we need them).
/*
	if (!found) {
		error_msg = detect_drives_linux_udev_byid(devices);  // linux udev
		// we check for devices vector emptiness because it could be a dummy directory
		// with no files, so treat it as an error.
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}
*/
	if (!found) {
		error_msg = detect_drives_linux_proc_partitions(devices);  // linux /proc/partitions as fallback.
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}

#elif defined CONFIG_KERNEL_FAMILY_WINDOWS

	if (!found) {
		error_msg = detect_drives_win32(devices);  // win32
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}


#else  // freebsd, etc...

	if (!found) {
		error_msg = detect_drives_other(devices);  // bsd, etc... . scans /dev.
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}

#endif

	if (!found) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot detect drives: None of the drive detection methods returned any drives.\n");
		return error_msg;  // last error message should be ok.
	}


	for (std::vector<std::string>::const_iterator iter = devices.begin(); iter != devices.end(); ++iter) {
		std::string dev = *iter;

		// try to match against patterns
// 		for (unsigned int i = 0; i < match_patterns_.size(); i++) {
			// try to match against general filter
// 			if (!app_pcre_match(match_patterns_[i], dev))
// 				continue;

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
// 				break;  // no need to match other patters, go to next device

			} else {  // blacklisted
				debug_out_info("app", "Device \"" << drive->get_device() << "\" is blacklisted, ignoring.\n");
// 				break;  // go to next device
			}
// 		}

	}

	debug_out_info("app", DBG_FUNC_MSG << "Drive detection finished.\n");
	return std::string();
}



std::string StorageDetector::fetch_basic_data(std::vector<StorageDeviceRefPtr>& drives,
		hz::intrusive_ptr<CmdexSync> smartctl_ex, bool return_first_error)
{
	fetch_data_errors_.clear();
	fetch_data_error_outputs_.clear();


	// If it doesn't exist, create a default one. Even though it will be auto-created later,
	// we need it here to get its errors afterwards.
	if (!smartctl_ex)
		smartctl_ex = new SmartctlExecutor();  // will be auto-deleted

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
				<< "\tDetected type: " << StorageDevice::get_type_readable_name(drive->get_detected_type()) << "\n"
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
		fetch_basic_data(put_drives_here, smartctl_ex, false);  // ignore its errors, there may be plenty of them.

	return error_msg;
}






