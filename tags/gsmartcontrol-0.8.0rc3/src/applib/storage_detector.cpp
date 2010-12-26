/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <ios>  // std::boolalpha
// #include <glibmm/fileutils.h>  // Dir
#include <algorithm>  // std::find
#include <cstdio>  // std::fgets()

#include "hz/debug.h"
#include "hz/fs_dir.h"
#include "hz/fs_file.h"
#include "rconfig/rconfig_mini.h"

#include "app_pcrecpp.h"
#include "storage_detector.h"


namespace {


// inline std::string detect_drives_linux_sys_block(std::vector<StorageDeviceRefPtr>& drives)
// {
	// hz::Dir dir("/sys/block");
// }




// Linux 2.6 with udev. Scan /dev/disk/by-id - the directory entries there
// are symlinks to respective /dev devices. Some devices have multiple
// links pointing to them, so unique filter is needed.
/*
Sample listing:
------------------------------------------------------------
# ls -1 /dev/disk/by-id
ata-ST31000340AS_9QJ0FFG7
ata-ST31000340AS_9QJ0FFG7-part1
ata-ST31000340AS_9QJ0FFG7-part2
ata-ST3500630AS_9QG0R38D
ata-ST3500630AS_9QG0R38D-part1
scsi-SATA_ST31000340AS_9QJ0FFG7
scsi-SATA_ST31000340AS_9QJ0FFG7-part1
scsi-SATA_ST31000340AS_9QJ0FFG7-part2
scsi-SATA_ST3500630AS_9QG0R38D
scsi-SATA_ST3500630AS_9QG0R38D-part1
*/
inline std::string detect_drives_linux_udev_byid(std::vector<std::string>& devices)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting through device scan directory...\n");

	std::string dev_dir;  // this defaults to "/dev/disk/by-id"
	if (!rconfig::get_data("system/linux_udev_byid_path", dev_dir) || dev_dir.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Device scan directory path is not set.\n");
		return "Device scan directory path is not set.";
	}

	hz::Dir dir(dev_dir);

	std::vector<std::string> all_devices;
	if (!dir.entry_list(all_devices, false)) {  // this outputs to debug too.
		std::string error_msg = dir.get_error_utf8();
		if (!dir.exists()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Device scan directory doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Cannot list directory entries.\n");
		}
		return error_msg;
	}

	std::vector<std::string> blacklist;
	blacklist.push_back("/-part[0-9]+$/");

	// filter-out the ones with "partN" in them
	for (unsigned int i = 0; i < all_devices.size(); ++i) {
		std::string entry = all_devices[i];
		if (entry == "." || entry == "..")
			continue;

		// platform blacklist
		bool blacked = false;
		for (std::vector<std::string>::const_iterator iter = blacklist.begin(); iter != blacklist.end(); ++iter) {
			if (app_pcre_match(*iter, entry)) {
				blacked = true;
				break;
			}
		}
		if (blacked)
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
// 		if (app_pcre_match("/[0-9]+$/", path.str()))
// 			continue;

		if (std::find(devices.begin(), devices.end(), path.str()) == devices.end())  // there may be duplicates
			devices.push_back(path.str());
	}

	return std::string();
}



// Procfs files don't support SEEK_END or ftello() (I think). Anyway, they can't
// be read through hz::File::get_contents, so use this function instead.
inline bool read_proc_partitions_file(hz::File& file, std::vector<std::string>& lines)
{
	if (!file.open("rb"))  // closed automatically
		return false;  // the error message is in File itself.

	FILE* fp = file.get_handle();
	if (!fp)
		return false;

	char line[256];
	while (std::fgets(line, sizeof(line), fp) != NULL) {
		if (*line != '\0')
			lines.push_back(line);
	}

	return true;
}



// Linux (tested with 2.4 and 2.6) /proc/partitions. Parses the file, appends /dev to each entry.
// Note that file format changed from 2.4 to 2.6 (some statistics fields moved to another file).
/*
Sample 1 (2.4, devfs, with statistics):
------------------------------------------------------------
# cat /proc/partitions
major minor  #blocks  name     rio rmerge rsect ruse wio wmerge wsect wuse running use aveq

   3     0   60051600 ide/host0/bus0/target0/lun0/disc 72159 136746 1668720 467190 383039 658435 8342136 1659840 -429 17038808 22703194
   3     1    1638598 ide/host0/bus0/target0/lun0/part1 0 0 0 0 0 0 0 0 0 0 0
   3     2    1028160 ide/host0/bus0/target0/lun0/part2 0 0 0 0 0 0 0 0 0 0 0
   3     3    3092512 ide/host0/bus0/target0/lun0/part3 0 0 0 0 0 0 0 0 0 0 0
   3     4          1 ide/host0/bus0/target0/lun0/part4 0 0 0 0 0 0 0 0 0 0 0
   3     5     514048 ide/host0/bus0/target0/lun0/part5 1458 7877 74680 9070 2140 18285 166272 42490 0 10100 52000
------------------------------------------------------------
Sample 2 (2.6):
------------------------------------------------------------
# cat /proc/partitions
major minor  #blocks  name

    1     0       4096 ram0
    8     0  156290904 sda
    8     1   39070048 sda1
    8     2       7560 sda2
    8     3          1 sda3
------------------------------------------------------------
Sample 3 (2.6 on nokia n8xx, spaces may be missing):
------------------------------------------------------------
# cat /proc/partitions
major minor  #blocks  name

31  0     128 mtdblock0
31  1     384 mtdblock1
31  2    2048 mtdblock2
31  3    2048 mtdblock3
31  4  257536 mtdblock4
254 0 3932160 mmcblk0
254 1 3928064 mmcblk0p1
254 8 1966080 mmcblk1
254 9 2007032 mmcblk1p1
*/
inline std::string detect_drives_linux_proc_partitions(std::vector<std::string>& devices)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting through partitions file...\n");

	std::string parts_file;
	if (!rconfig::get_data("system/linux_proc_partitions_path", parts_file) || parts_file.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Partitions file path is not set.\n");
		return "Partitions file path is not set.";
	}

	hz::File file(parts_file);
// 	std::string contents;
	std::vector<std::string> lines;
// 	if (!f.get_contents(contents)) {
	if (!read_proc_partitions_file(file, lines)) {  // this outputs to debug too
		std::string error_msg = file.get_error_utf8();  // save before calling other file functions
		if (!file.exists()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Partitions file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Partitions file exists but cannot be read.\n");
		}
		return error_msg;
	}

// 	debug_out_dump("app", DBG_FUNC_MSG << "Dumping partitions file:\n" << contents << "\n");

// 	hz::string_split(contents, '\n', lines, true);

	std::vector<std::string> blacklist;
	// fixme: not sure about how partitions are visible with twa0.
	blacklist.push_back("/d[a-z][0-9]+$/");  // sda1, hdb2 - partitions. twa0 and twe1 are drives, not partitions.
	blacklist.push_back("/ram[0-9]+$/");
	blacklist.push_back("/loop[0-9]*$/");
	blacklist.push_back("/part[0-9]+$/");
	blacklist.push_back("/p[0-9]+$/");


	for (unsigned int i = 0; i < lines.size(); ++i) {
		std::string line = hz::string_trim_copy(lines[i]);
		if (line.empty() || app_pcre_match("/^major/", line))  // file header
			continue;

		std::string dev;
		if (!app_pcre_match("/^[ \\t]*[^ \\t\\n]+[ \\t]+[^ \\t\\n]+[ \\t]+[^ \\t\\n]+[ \\t]+([^ \\t\\n]+)/", line, &dev) || dev.empty()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse line \"" << line << "\".\n");
			continue;
		}

		// platform blacklist
		bool blacked = false;
		for (std::vector<std::string>::const_iterator iter = blacklist.begin(); iter != blacklist.end(); ++iter) {
			if (app_pcre_match(*iter, dev)) {
				blacked = true;
				break;
			}
		}
		if (blacked)
			continue;

		std::string path = "/dev/" + dev;
		if (std::find(devices.begin(), devices.end(), path) == devices.end())  // there may be duplicates
			devices.push_back(path);
	}

	return std::string();
}


/*
// FreeBSD, etc... .
// Linux devices are not listed because /proc/partitions should always exist there.

inline std::string detect_drives_dev(std::vector<std::string>& devices)
{

	return std::string();
}



inline std::string detect_drives_win32(std::vector<std::string>& devices)
{

	return std::string();
}
*/


}




std::string StorageDetector::detect(std::vector<StorageDeviceRefPtr>& drives)
{
	debug_out_info("app", DBG_FUNC_MSG << "Starting drive detection.\n");

	std::vector<std::string> devices;
	std::string error_msg;
	bool found = false;

	// Try each one and move to next if it fails.

	if (!found) {
		error_msg = detect_drives_linux_udev_byid(devices);  // linux udev
		// we check for devices vector emptiness because it could be a dummy directory
		// with no files, so treat it as an error.
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}

	if (!found) {
		error_msg = detect_drives_linux_proc_partitions(devices);  // linux /proc/partitions as fallback.
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}
/*
	if (!found) {
		error_msg = detect_drives_dev(devices);  // bsd, etc... . scans /dev.
		if (error_msg.empty() && !devices.empty()) {
			found = true;
		}
	}
*/
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
		fetch_basic_data(put_drives_here, smartctl_ex, false);  // ignore its errors, there may be plenty of them.

	return error_msg;
}






