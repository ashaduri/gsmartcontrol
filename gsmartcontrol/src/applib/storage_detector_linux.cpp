/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "storage_detector_linux.h"

#if defined CONFIG_KERNEL_LINUX


#include <algorithm>  // std::find
#include <cstdio>  // std::fgets(), std::FILE
#include <cerrno>  // ENXIO

#include "hz/debug.h"
#include "hz/fs_path_utils.h"
#include "hz/fs_file.h"
#include "rconfig/rconfig_mini.h"
#include "app_pcrecpp.h"
#include "storage_detector_helpers.h"




namespace {


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
/*
// We don't use udev anymore - not all distros have it, and e.g. Ubuntu
// has it all wrong (two symlinks (sda, sdb) pointing both to sdb).
inline std::string detect_drives_linux_udev_byid(std::vector<std::string>& devices)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting through device scan directory /dev/disk/by-id...\n");

	std::string dev_dir;  // this defaults to "/dev/disk/by-id"
	if (!rconfig::get_data("system/linux_udev_byid_path", dev_dir) || dev_dir.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Device scan directory path is not set.\n");
		return "Device scan directory path is not set.";
	}

	hz::Dir dir(dev_dir);

	std::vector<std::string> all_devices;
	if (!dir.list(all_devices, false)) {  // this outputs to debug too.
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
*/



/// Procfs files don't support SEEK_END or ftello() (I think). They can't
/// be read through hz::File::get_contents, so use this function instead.
inline bool read_proc_file(hz::File& file, std::vector<std::string>& lines)
{
	if (!file.open("rb"))  // closed automatically
		return false;  // the error message is in File itself.

	std::FILE* fp = file.get_handle();
	if (!fp)
		return false;

	char line[256];
	while (std::fgets(line, static_cast<int>(sizeof(line)), fp) != NULL) {
		if (*line != '\0')
			lines.push_back(line);
	}

	return true;
}



/// Read /proc/partitions file. Return error message on error.
inline std::string read_proc_partitions_file(std::vector<std::string>& lines)
{
	std::string path;
	if (!rconfig::get_data("system/linux_proc_partitions_path", path) || path.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Partitions file path is not set.\n");
		return "Partitions file path is not set.";
	}

	hz::File file(path);
	if (!read_proc_file(file, lines)) {  // this outputs to debug too
		std::string error_msg = file.get_error_utf8();  // save before calling other file functions
		if (!file.exists()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Partitions file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Partitions file exists but cannot be read.\n");
		}
		return error_msg;
	}

	return std::string();
}



/// Read /proc/devices file. Return error message on error.
inline std::string read_proc_devices_file(std::vector<std::string>& lines)
{
	std::string path;
	if (!rconfig::get_data("system/linux_proc_devices_path", path) || path.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Devices file path is not set.\n");
		return "Devices file path is not set.";
	}

	hz::File file(path);
	if (!read_proc_file(file, lines)) {  // this outputs to debug too
		std::string error_msg = file.get_error_utf8();  // save before calling other file functions
		if (!file.exists()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Devices file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Devices file exists but cannot be read.\n");
		}
		return error_msg;
	}

	return std::string();
}



// Read /proc/scsi/scsi file. Return error message on error.
inline std::string read_proc_scsi_scsi_file(std::vector<std::string>& lines)
{
	std::string path;
	if (!rconfig::get_data("system/linux_proc_scsi_scsi_path", path) || path.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "SCSI file path is not set.\n");
		return "SCSI file path is not set.";
	}

	hz::File file(path);
	if (!read_proc_file(file, lines)) {  // this outputs to debug too
		std::string error_msg = file.get_error_utf8();  // save before calling other file functions
		if (!file.exists()) {
			debug_out_warn("app", DBG_FUNC_MSG << "SCSI file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "SCSI file exists but cannot be read.\n");
		}
		return error_msg;
	}

	return std::string();
}



/// Get number of ports by sequentially running smartctl on each port, until
/// one of the gives an error. Return -1 on error.
inline std::string smartctl_get_drives(const std::string& dev, const std::string& type,
	  int from, int to, std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	for (int i = from; i <= to; ++i) {
		std::string type_arg = hz::string_sprintf(type.c_str(), i);
		StorageDeviceRefPtr drive(new StorageDevice(dev, type_arg));

// 		std::string error_msg = execute_smartctl(dev, "-d " + type_arg, "-i", smartctl_ex, output);
		std::string error_msg = drive->fetch_basic_data_and_parse(smartctl_ex);
		std::string output = drive->get_info_output();

		// if we've reached smartctl port limit (older versions may have smaller limits), abort.
		if (app_pcre_match("/VALID ARGUMENTS ARE/mi", output)) {
			break;
		}

		if (!error_msg.empty()) {
			debug_out_info("app", "Smartctl returned with an error: " << error_msg << "\n");
		} else {
			drives.push_back(drive);
		}
	}

	return std::string();
}




/**
Linux (tested with 2.4 and 2.6) /proc/partitions. Parses the file, appends /dev to each entry.
Note that file format changed from 2.4 to 2.6 (some statistics fields moved to another file).
No /proc/partitions on freebsd, solaris or osx, afaik.

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
inline std::string detect_drives_linux_proc_partitions(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting through /proc/partitions...\n");

	std::vector<std::string> lines;
	std::string error_msg = read_proc_partitions_file(lines);
	if (!error_msg.empty()) {
		return error_msg;
	}

	std::vector<std::string> blacklist;
	// fixme: not sure about how partitions are visible with twa0.
	blacklist.push_back("/d[a-z][0-9]+$/");  // sda1, hdb2 - partitions. twa0 and twe1 are drives, not partitions.
	blacklist.push_back("/ram[0-9]+$/");  // ramdisks?
	blacklist.push_back("/loop[0-9]*$/");  // not sure if loop devices go there, but anyway...
	blacklist.push_back("/part[0-9]+$/");  // devfs had them
	blacklist.push_back("/p[0-9]+$/");  // partitions are usually marked this way
	blacklist.push_back("/md[0-9]*$/");  // linux software raid
	blacklist.push_back("/dm-[0-9]*$/");  // linux device mapper

	std::vector<std::string> devices;

	for (std::size_t i = 0; i < lines.size(); ++i) {
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

		std::string path = "/dev/" + dev;  // let's just hope the it's /dev.
		if (std::find(devices.begin(), devices.end(), path) == devices.end())  // there may be duplicates
			devices.push_back(path);
	}

	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	for (std::size_t i = 0; i < devices.size(); ++i) {
		StorageDeviceRefPtr drive(new StorageDevice(devices.at(i)));
		drive->fetch_basic_data_and_parse(smartctl_ex);

		// 3ware controllers also export themselves as sd*. Smartctl detects that,
		// so we can avoid adding them. Older smartctl (5.38) prints "AMCC", newer one
		// prints "AMCC/3ware controller". It's better to search it this way.
		if (!app_pcre_match("/try adding '-d 3ware,N'/im", drive->get_info_output())) {
			drives.push_back(drive);
		}
	}

	return std::string();
}



/**
Detect drives behind 3ware RAID controller.

3ware Linux:
Call as: smartctl -i -d 3ware,[0-127] /dev/twa[0-15]  (or twe[0-15])
Use twe* for [678]xxx series, and twa* for 9xxx series.
Note: twe* devices are limited to [0-15] ports (not sure about this).
Note: /dev/tw* devices may not exist, they are created by smartctl on the first run.
Note: for twe*, /dev/sda may also exist (to be used with -d 3ware,N), we should
	somehow detect and ignore them.
Note: when specifying non-existent port, either a "Device Read Identity Failed"
	error, or a "blank" info may be returned.
Detection:
Grep /proc/devices for "twa" or "twe" (e.g. "251 twa"). Use this for /dev/tw* part.
Grep /proc/scsi/scsi for AMCC or 3ware (LSI too?), use number of matched lines N
	for /dev/tw*[0, N-1].
For detecting the number of ports, use "tw_cli /cK show all", K being the controller
	scsi number, which is displayed as scsiK in the scsi file.
	If there's no tw_cli, we'll have to scan all the ports (up to supported maximum).
	This is too slow however, so scan only 24.

Implementation notes: it seems that twe uses "3ware" and twa uses "AMCC".
We can't handle a situation with both twa and twe present, since we don't know
how they will be ordered for tw_cli.
*/
inline std::string detect_drives_linux_3ware(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	std::vector<std::string> lines;
	std::string error_msg = read_proc_devices_file(lines);
	if (!error_msg.empty()) {
		return error_msg;
	}

	bool twa_found = false;
	bool twe_found = false;

	// Check /proc/devices for twa or twe
	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string dev;
		if (app_pcre_match("/^[ \\t]*[0-9]+[ \\t]+(tw[ae])/", hz::string_trim_copy(lines[i]), &dev)) {
			debug_out_dump("app", DBG_FUNC_MSG << "Found " << dev << " entry in devices file.\n");
			if (dev == "twa") {
				twa_found = true;
			} else if (dev == "twe") {
				twe_found = true;
			} else {
				DBG_ASSERT(0);  // error in regexp?
			}
		}
	}

	if (!twa_found && !twe_found) {
		return std::string();  // no controllers
	}

	lines.clear();
	error_msg = read_proc_scsi_scsi_file(lines);
	if (!error_msg.empty()) {
		return error_msg;
	}


	int num_controllers = 0;
	int last_scsi_host = 0;

	pcrecpp::RE host_re = app_pcre_re("^Host: scsi([0-9]+)");

	for (std::size_t i = 0; i < lines.size(); ++i) {
		// The format of this file is scsi host number on one line, vendor on another.
		// debug_out_error("app", "SCSI Line: " << hz::string_trim_copy(lines[i]) << "\n");
		std::string scsi_host_str;
		if (host_re.PartialMatch(hz::string_trim_copy(lines[i]), &scsi_host_str)) {
			// debug_out_dump("app", "SCSI Host " << scsi_host_str << " found.\n");
			hz::string_is_numeric(scsi_host_str, last_scsi_host);
		}
		if (!app_pcre_match("/Vendor: (AMCC)|(3ware) /i", hz::string_trim_copy(lines[i]))) {
			continue;  // not a supported controller
		}

		debug_out_dump("app", "Found AMCC/3ware controller in SCSI file, SCSI host " << last_scsi_host << ".\n");

		// We can't handle both twa and twe in one system, so assume twa by default.
		// We can't map twaX to scsiY, so lets assume the relative order is the same.
		std::string dev = std::string("/dev/") + (twa_found ? "twa" : "twe") + hz::number_to_string(num_controllers);

		error_msg = tw_cli_get_drives(dev, last_scsi_host, drives, ex_factory, false);
		if (!error_msg.empty()) {  // no tw_cli
			int max_ports = rconfig::get_data<int32_t>("system/linux_max_scan_ports");
			max_ports = std::max(max_ports, 23);  // 128 smartctl calls are too much (it's too slow). Settle for 24.
			error_msg = smartctl_get_drives(dev, "3ware,%d", 0, max_ports, drives, ex_factory);
		}

		if (!error_msg.empty()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Couldn't get number of ports on a 3ware controller.\n");
		}

		++num_controllers;
	}

	if (num_controllers == 0) {
		debug_out_warn("app", DBG_FUNC_MSG << "3ware entry found in devices file, but SCSI file contains no known entries.\n");
	}

	return error_msg;
}


}  // anon ns




std::string detect_drives_linux(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	std::vector<std::string> error_msgs;
	std::string error_msg;

	// Disable by-id detection - it's unreliable on broken systems.
	// For example, on Ubuntu 8.04, /dev/disk/by-id contains two device
	// links for two drives, but both point to the same sdb (instead of
	// sda and sdb). Plus, there are no "*-partN" files (not that we need them).
// 	error_msg = detect_drives_linux_udev_byid(devices);  // linux udev

	error_msg = detect_drives_linux_proc_partitions(drives, ex_factory);
	if (!error_msg.empty()) {
		error_msgs.push_back(error_msg);
	}

	error_msg = detect_drives_linux_3ware(drives, ex_factory);
	if (!error_msg.empty()) {
		error_msgs.push_back(error_msg);
	}

	return hz::string_join(error_msgs, "\n");
}





#endif  // CONFIG_KERNEL_LINUX

/// @}
