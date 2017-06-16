/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include <algorithm>

#include "hz/hz_config.h"  // CONFIG_*

#include "hz/debug.h"

#include "app_pcrecpp.h"
#include "smartctl_executor.h"
#include "storage_detector.h"

#include "storage_detector_linux.h"
#include "storage_detector_win32.h"
#include "storage_detector_other.h"




std::string StorageDetector::detect(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Starting drive detection.\n");

	std::vector<StorageDeviceRefPtr> all_detected;
	std::string error_msg;

	// Try each one and move to next if it fails.

#if defined CONFIG_KERNEL_LINUX

	error_msg = detect_drives_linux(all_detected, ex_factory);  // linux /proc/partitions as fallback.

#elif defined CONFIG_KERNEL_FAMILY_WINDOWS

	error_msg = detect_drives_win32(all_detected, ex_factory);  // win32

#else  // freebsd, etc...

	error_msg = detect_drives_other(all_detected, ex_factory);  // bsd, etc... . scans /dev.

#endif

	if (all_detected.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot detect drives: None of the drive detection methods returned any drives.\n");
		return error_msg;  // last error message should be ok.
	}


	for (std::vector<StorageDeviceRefPtr>::iterator iter = all_detected.begin(); iter != all_detected.end(); ++iter) {
		StorageDeviceRefPtr drive = *iter;

		// try to match against patterns
// 		for (unsigned int i = 0; i < match_patterns_.size(); i++) {
			// try to match against general filter
// 			if (!app_pcre_match(match_patterns_[i], dev))
// 				continue;

			// matched, check the blacklist
			bool blacked = false;
			for (unsigned int j = 0; j < blacklist_patterns_.size(); j++) {
				if (app_pcre_match(blacklist_patterns_[j], drive->get_device())) {  // matched the blacklist too
					blacked = true;
					break;
				}
			}

			debug_out_info("app", "Found device: " << drive->get_device_with_type() << ".\n");

			if (!blacked) {
				drives.push_back(drive);
// 				break;  // no need to match other patters, go to next device

			} else {  // blacklisted
				debug_out_info("app", "Device " << drive->get_device_with_type() << " is blacklisted, ignoring.\n");
// 				break;  // go to next device
			}
// 		}
	}

	// Sort the drives, because their order is not quite defined.
	// TODO Sort using natural sort
	std::sort(drives.begin(), drives.end());

	debug_out_info("app", DBG_FUNC_MSG << "Drive detection finished.\n");
	return std::string();
}



std::string StorageDetector::fetch_basic_data(std::vector<StorageDeviceRefPtr>& drives,
		ExecutorFactoryRefPtr ex_factory, bool return_first_error)
{
	fetch_data_errors_.clear();
	fetch_data_error_outputs_.clear();

	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	for (unsigned int i = 0; i < drives.size(); ++i) {
		StorageDeviceRefPtr drive = drives[i];
		debug_out_info("app", "Retrieving basic information about the device...\n");

		smartctl_ex->set_running_msg("Running %s on " + drive->get_device_with_type() + "...");

		// don't show any errors here - we don't want a screen flood.
		// no need for gui-based executors here, we already show the message in
		// iconview background (if called from main window)
		std::string error_msg;
		if (drive->get_info_output().empty()) {  // if not fetched during detection
			error_msg = drive->fetch_basic_data_and_parse(smartctl_ex);
		}

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

		debug_out_dump("app", "Device information for " << drive->get_device()
				<< " (type: \"" << drive->get_type_argument() << "\"):\n"
				<< "\tModel: " << drive->get_model_name() << "\n"
				<< "\tDetected type: " << StorageDevice::get_type_readable_name(drive->get_detected_type()) << "\n"
				<< "\tSMART status: " << StorageDevice::get_status_name(drive->get_smart_status()) << "\n"
				);

	}

	return std::string();
}



std::string StorageDetector::detect_and_fetch_basic_data(std::vector<StorageDeviceRefPtr>& put_drives_here,
		ExecutorFactoryRefPtr ex_factory)
{
	std::string error_msg = detect(put_drives_here, ex_factory);

	if (error_msg.empty())
		fetch_basic_data(put_drives_here, ex_factory, false);  // ignore its errors, there may be plenty of them.

	return error_msg;
}







/// @}
