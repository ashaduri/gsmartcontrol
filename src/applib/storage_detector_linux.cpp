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

#include "build_config.h"

#include "local_glibmm.h"
#include <algorithm>  // std::find
#include <cstdio>  // std::fgets(), std::FILE
// #include <cerrno>  // ENXIO
#include <set>
#include <map>
#include <vector>
#include <utility>  // std::pair
#include <string>

#include "hz/debug.h"
#include "hz/fs.h"
#include "hz/string_num.h"
#include "rconfig/rconfig.h"
#include "app_pcrecpp.h"
#include "storage_detector_linux.h"
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

	// this defaults to "/dev/disk/by-id"
	auto dir = hz::fs_path_from_string(rconfig::get_data<std::string>("system/linux_udev_byid_path"));
	if (dir.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Device directory path is not set.\n");
		return "Device directory path is not set.";
	}

	std::error_code dummy_ec;
	if (!hz::fs::exists(dir, dummy_ec)) {
		debug_out_warn("app", DBG_FUNC_MSG << "Device directory doesn't exist.\n");
		return "Device directory does not exist.";
	}

	// filter-out the ones with "partN" in them
	static const std::vector<std::string> blacklist = {
			"/-part[0-9]+$/"
	};

	std::error_code ec;
	for (const auto& entry : hz::fs::directory_iterator(dir, ec)) {  // this outputs to debug too.
		auto path = entry.path();

		// platform blacklist
		bool blacked = false;
		for (const auto& bl_pattern : blacklist) {
			if (app_pcre_match(bl_pattern, path.string())) {
				blacked = true;
				break;
			}
		}
		if (blacked)
			continue;

		// those are usually relative links, so find out where they are pointing to.
		if (hz::fs::is_symlink(path, dummy_ec)) {
			if (!hz::fs::exists(path)) {  // broken symlink
				continue;
			}
			path = hz::fs::read_symlink(path, dummy_ec);
			if (path.is_relative()) {
				path = hz::fs::canonical(entry.path() / path);  // remove ../-s
			}
		}

		if (std::find(devices.begin(), devices.end(), path.string()) == devices.end())  // there may be duplicates
			devices.push_back(path.string());
	}
	if (ec) {
		debug_out_error("app", DBG_FUNC_MSG << "Cannot list device directory entries.\n");
		return hz::string_sprintf("Cannot list device directory entries: %s", ec.message().c_str());
	}

	return std::string();
}
*/



/// Cache of file->contents, caches read files.
inline std::map<hz::fs::path, std::string>& get_read_file_cache_ref()
{
	static std::map<hz::fs::path, std::string> cache;
	return cache;
}



/// Clear the read file cache.
inline void clear_read_file_cache()
{
	get_read_file_cache_ref().clear();
}



/// Read procfs file without using seeking.
inline std::error_code read_proc_file(const hz::fs::path& file, std::string& contents)
{
	auto& cache = get_read_file_cache_ref();
	if (auto iter = cache.find(file); iter != cache.end()) {
		contents = iter->second;
		return {};
	}

	auto ec = hz::fs_file_get_contents_unseekable(file, contents);
	if (ec) {
		return ec;
	}

	cache[file] = contents;

	debug_begin();  // avoiding printing prefix on every line
	debug_out_dump("app", DBG_FUNC_MSG << "File contents (\"" << file.string() << "\"):\n" << contents << "\n");
	debug_end();

	return {};
}



/// Same as read_proc_file(), but returns a vector of lines
inline std::error_code read_proc_file_lines(const hz::fs::path& file, std::vector<std::string>& lines)
{
	std::string contents;
	auto ec = read_proc_file(file, contents);
	if (!ec) {
		hz::string_split(contents, '\n', lines, true);
	}
	return ec;
}




/// Read /proc/partitions file. Return error message on error.
inline std::string read_proc_partitions_file(std::vector<std::string>& lines)
{
	auto file = hz::fs_path_from_string(rconfig::get_data<std::string>("system/linux_proc_partitions_path"));
	if (file.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Partitions file path is not set.\n");
		return _("Partitions file path is not set.");
	}

	auto ec = read_proc_file_lines(file, lines);
	if (ec) {
		std::error_code dummy_ec;
		if (!hz::fs::exists(file, dummy_ec)) {
			debug_out_warn("app", DBG_FUNC_MSG << "Partitions file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Partitions file exists but cannot be read.\n");
		}
		return ec.message();
	}

	return {};
}



/// Read /proc/devices file. Return error message on error.
inline std::string read_proc_devices_file(std::vector<std::string>& lines)
{
	auto file = hz::fs_path_from_string(rconfig::get_data<std::string>("system/linux_proc_devices_path"));
	if (file.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Devices file path is not set.\n");
		return _("Devices file path is not set.");
	}

	auto ec = read_proc_file_lines(file, lines);
	if (ec) {
		std::error_code dummy_ec;
		if (!hz::fs::exists(file, dummy_ec)) {
			debug_out_warn("app", DBG_FUNC_MSG << "Devices file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Devices file exists but cannot be read.\n");
		}
		return ec.message();
	}

	return {};
}



/// Read /proc/scsi/scsi file. Return error message on error.
/// \c vendors_models is filled with (scsi host #, trimmed vendors line) pairs.
/// Note that scsi host # is not unique.
inline std::string read_proc_scsi_scsi_file(std::vector< std::pair<int, std::string> >& vendors_models)
{
	auto file = hz::fs_path_from_string(rconfig::get_data<std::string>("system/linux_proc_scsi_scsi_path"));
	if (file.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "SCSI file path is not set.\n");
		return _("SCSI file path is not set.");
	}

	std::vector<std::string> lines;
	auto ec = read_proc_file_lines(file, lines);
	if (ec) {
		std::error_code dummy_ec;
		if (!hz::fs::exists(file, dummy_ec)) {
			debug_out_warn("app", DBG_FUNC_MSG << "SCSI file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "SCSI file exists but cannot be read.\n");
		}
		return ec.message();
	}

	int last_scsi_host = -1;
	const pcrecpp::RE host_re = app_pcre_re("^Host: scsi([0-9]+)");

	for (auto line : lines) {
		hz::string_trim(line);
		// The format of this file is scsi host number on one line, vendor on another,
		// some other info on the third.
		// debug_out_error("app", "SCSI Line: " << hz::string_trim_copy(lines[i]) << "\n");
		std::string scsi_host_str;
		if (host_re.PartialMatch(line, &scsi_host_str)) {
			// debug_out_dump("app", "SCSI Host " << scsi_host_str << " found.\n");
			hz::string_is_numeric_nolocale(scsi_host_str, last_scsi_host);

		} else if (last_scsi_host != -1 && app_pcre_match("/Vendor: /i", line)) {
			vendors_models.emplace_back(last_scsi_host, line);
		}
	}

	return {};
}



/// host	chan	id	lun	type	opens	qdepth	busy	online
/// Read /proc/scsi/sg/devices file. Return error message on error.
/// \c sg_entries is filled with lines parsed as ints.
/// Each line index corresponds to N in /dev/sgN.
inline std::string read_proc_scsi_sg_devices_file(std::vector<std::vector<int>>& sg_entries)
{
	auto file = hz::fs_path_from_string(rconfig::get_data<std::string>("system/linux_proc_scsi_sg_devices_path"));
	if (file.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Sg devices file path is not set.\n");
		return _("SCSI sg devices file path is not set.");
	}

	std::vector<std::string> lines;
	auto ec = read_proc_file_lines(file, lines);
	if (ec) {
		std::error_code dummy_ec;
		if (!hz::fs::exists(file, dummy_ec)) {
			debug_out_warn("app", DBG_FUNC_MSG << "Sg devices file doesn't exist.\n");
		} else {
			debug_out_error("app", DBG_FUNC_MSG << "Sg devices file exists but cannot be read.\n");
		}
		return ec.message();
	}

	pcrecpp::RE parse_re = app_pcre_re(
			R"(^([0-9-]+)\s+([0-9-]+)\s+([0-9-]+)\s+([0-9-]+)\s+([0-9-]+)\s+([0-9-]+)\s+([0-9-]+)\s+([0-9-]+)\s+([0-9-]+))");

	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string trimmed = hz::string_trim_copy(lines[i]);
		std::vector<std::string> line(9);
		if (parse_re.PartialMatch(trimmed, &line[0], &line[1], &line[2], &line[3], &line[4], &line[5], &line[6], &line[7], &line[8])) {
			std::vector<int> line_num(line.size(), -1);
			for (std::size_t j = 0; j < line_num.size(); ++j) {
				hz::string_is_numeric_nolocale(line[j], line_num[j]);
			}
			sg_entries.resize(i+1);  // maintain the line indices
			sg_entries[i] = line_num;

		} else {
			debug_out_warn("app", DBG_FUNC_MSG << "Sg devices line offset " << i << " has invalid format.\n");
		}
	}

	return {};
}




/**
<pre>
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
</pre> */
inline hz::ExpectedVoid<StorageDetectorError> detect_drives_linux_proc_partitions(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives through partitions file (/proc/partitions by default; set \"system/linux_proc_partitions_path\" config key to override).\n");

	std::vector<std::string> lines;
	std::string error_msg = read_proc_partitions_file(lines);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	static const std::vector<std::string> blacklist = {
		"/d[a-z][0-9]+$/",  // sda1, hdb2 - partitions. twa0 and twe1 are drives, not partitions.
		"/ram[0-9]+$/",  // ramdisks?
		"/loop[0-9]*$/",  // not sure if loop devices go there, but anyway...
		"/part[0-9]+$/",  // devfs had them
		"/p[0-9]+$/",  // partitions are usually marked this way
		"/md[0-9]*$/",  // linux software raid
		"/dm-[0-9]*$/",  // linux device mapper
	};

	std::vector<std::string> devices;

	for (auto line : lines) {
		hz::string_trim(line);
		if (line.empty() || app_pcre_match("/^major/", line))  // file header
			continue;

		std::string dev;
		if (!app_pcre_match(R"(/^[ \t]*[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+([^ \t\n]+)/)", line, &dev) || dev.empty()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse line \"" << line << "\".\n");
			continue;
		}

		// platform blacklist
		bool blacked = false;
		for (const auto& bl_pattern : blacklist) {
			if (app_pcre_match(bl_pattern, dev)) {
				blacked = true;
				break;
			}
		}
		if (blacked)
			continue;

		std::string path = "/dev/" + dev;  // let's just hope it's really /dev.
		if (std::find(devices.begin(), devices.end(), path) == devices.end()) {  // there may be duplicates
			devices.push_back(path);
		}
	}

	std::shared_ptr<CommandExecutor> smartctl_ex = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::Smartctl);

	for (const auto& device : devices) {
		auto drive = std::make_shared<StorageDevice>(device);
		auto fetch_status = drive->fetch_basic_data_and_parse(smartctl_ex);
		if (!fetch_status) {
			continue;
		}

		// 3ware controllers also export themselves as sd*. Smartctl detects that,
		// so we can avoid adding them. Older smartctl (5.38) prints "AMCC", newer one
		// prints "AMCC/3ware controller". It's better to search it this way.
		if (app_pcre_match("/try adding '-d 3ware,N'/im", drive->get_basic_output())) {
			debug_out_dump("app", "Drive " << drive->get_device_with_type() << " seems to be a 3ware controller, ignoring.\n");
		} else {
			drives.push_back(drive);
			debug_out_info("app", "Added drive " << drive->get_device_with_type() << ".\n");
		}
	}

	return {};
}



/** <pre>
Detect drives behind 3ware RAID controller.

3ware Linux (3w-9xxx, 3w-xxxx, 3w-sas drivers):
Call as: smartctl -i -d 3ware,[0-127] /dev/twa[0-15]  (or twe[0-15], or twl[0-15])
Use twe* for [678]xxx series (), twa* for 9xxx series, twl* for 9750 series.
Note: twe* devices are limited to [0-15] ports (not sure about this).
Note: /dev/tw* devices may not exist, they are created by smartctl on the first run.
Note: for twe*, /dev/sda may also exist (to be used with -d 3ware,N), we should
	somehow detect and ignore them.
Note: when specifying non-existent port, either a "Device Read Identity Failed"
	error, or a "blank" info may be returned.

Detection:
Grep /proc/devices for "twa", "twe" or "twl" (e.g. "251 twa"). Use this for /dev/tw* part.
Grep /proc/scsi/scsi for AMCC or 3ware (LSI too?), use number of matched lines N
	for /dev/tw*[0, N-1].
For detecting the number of ports, use "tw_cli /cK show all", K being the controller
	scsi number, which is displayed as scsiK in the scsi file.
	If there's no tw_cli, we'll have to scan all the ports (up to supported maximum).
	This is too slow however, so scan only 24.

Implementation notes: it seems that twe uses "3ware" and twa uses "AMCC"
(not sure about twl).
We can't handle a situation with mixed twa/twe/twl systems, since we don't know
how they will be ordered for tw_cli.
</pre> */
inline hz::ExpectedVoid<StorageDetectorError> detect_drives_linux_3ware(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives behind 3ware controller(s)...\n");

	std::vector<std::string> lines;
	std::string error_msg = read_proc_devices_file(lines);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	bool twa_found = false;
	bool twe_found = false;
	bool twl_found = false;

	// Check /proc/devices for twa or twe
	for (const auto& line : lines) {
		std::string dev;
		if (app_pcre_match(R"(/^[ \t]*[0-9]+[ \t]+(tw[ael])(?:[ \t]*|$)/)", hz::string_trim_copy(line), &dev)) {
			debug_out_dump("app", DBG_FUNC_MSG << "Found 3ware " << dev << " entry in devices file.\n");
			if (dev == "twa") {
				twa_found = true;
			} else if (dev == "twe") {
				twe_found = true;
			} else if (dev == "twl") {
				twl_found = true;
			} else {
				DBG_ASSERT(0);  // error in regexp?
			}
		}
	}

	if (!twa_found && !twe_found && !twl_found) {
		debug_out_info("app", DBG_FUNC_MSG << "No 3ware-specific entries found in devices file.\n");
		return {};  // no controllers
	}

	lines.clear();

	debug_out_dump("app", DBG_FUNC_MSG << "Checking scsi file for 3ware controllers.\n");
	std::vector< std::pair<int, std::string> > vendors_models;
	error_msg = read_proc_scsi_scsi_file(vendors_models);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}


	std::set<int> controller_hosts;  // scsi hosts (controllers) found for tw*
	std::map<std::string, int> device_numbers;  // device base (e.g. twa) -> number of times found

	for (auto& vendor_model : vendors_models) {
		// twe: 3ware, twa: AMCC, twl: LSI.
		std::string vendor;
		if (!app_pcre_match("/Vendor: (AMCC)|(3ware)|(LSI) /i", vendor_model.second, &vendor)) {
			continue;  // not a supported controller
		}

		int host_num = vendor_model.first;

		debug_out_dump("app", "Found LSI/AMCC/3ware controller in SCSI file, SCSI host " << host_num << ".\n");

		// Skip additional adapters with the same host, since they are the same adapters
		// with different LUNs.
		if (controller_hosts.find(host_num) != controller_hosts.end()) {
			debug_out_dump("app", "Skipping adapter with SCSI host " << host_num << ", host already found.\n");
			continue;
		}
		controller_hosts.insert(host_num);

		std::string dev_base = "twe";
		if (twa_found) {
			dev_base = "twa";
		} else if (twl_found) {
			dev_base = "twl";
		}

		// If there are several different tw* devices present (like 1 twa and 1 twe), we
		// use the vendor name to differentiate them.
		if (int(twa_found) + int(twe_found) + int(twl_found) > 1) {
			if (twa_found && hz::string_to_lower_copy(vendor) == "amcc") {
				dev_base = "twa";
			} else if (twe_found && hz::string_to_lower_copy(vendor) == "3ware") {
				dev_base = "twe";
			} else if (twl_found && hz::string_to_lower_copy(vendor) == "lsi") {
				dev_base = "twl";
			}
			// else we default to twl, twa, twe (in this order)
		}

		// We can't map twaX to scsiY, so lets assume the relative order is the same.
		std::string dev = std::string("/dev/") + dev_base + hz::number_to_string_nolocale(device_numbers[dev_base]);
		++device_numbers[dev_base];

		auto exec_status = tw_cli_get_drives(dev, vendor_model.first, drives, ex_factory, false);
		if (!exec_status) {  // no tw_cli
			int max_ports = rconfig::get_data<int>("system/linux_3ware_max_scan_port");
			max_ports = std::max(0, std::min(max_ports, 127));  // Sanity check
			debug_out_dump("app", "Starting brute-force port scan on 0-" << max_ports << " ports, device \"" << dev
					<< "\". Change the maximum by setting \"system/linux_3ware_max_scan_port\" config key.\n");
			std::string last_output;
			exec_status = smartctl_scan_drives_sequentially(dev, "3ware,%d", 0, max_ports, drives, ex_factory, last_output);
			debug_out_dump("app", "Brute-force port scan finished.\n");
		}

		if (!exec_status) {
			debug_out_warn("app", DBG_FUNC_MSG << "Couldn't get the drives on ports of LSI/AMCC/3ware controller: " << exec_status.error().message() << "\n");
		}
	}

	if (controller_hosts.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "AMCC/LSI/3ware entry found in devices file, but SCSI file contains no known entries.\n");
	}

	return {};
}



/** <pre>
Detect drives behind Adaptec RAID controller (aacraid driver).
Tested using Adaptec RAID 5805 (SAS / SATA controller).
Uses /dev/sgN devices (with -d sat for SATA and -d scsi (default) for SCSI).

Adaptec Linux detection strategy:
Check /proc/devices for "aac"; if it's there, continue.
Check /proc/scsi/scsi for "Vendor: Adaptec"; get its host (e.g. scsi6).
Check which lines correspond to e.g. host 6 in /proc/scsi/sg/devices; the
line order (e.g. fourth and fifth lines) correspond to /dev/sg* device (e.g.
/dev/sg3 and /dev/sg4).
Note: This will get us at least 2 unusable devices - /dev/sdc (the volume)
and /dev/sg3 (the controller). I think the controller can be filtered out
using "id > 0" requirement (the third column of /proc/scsi/sg/devices.
Try "-d sat" by default. If it fails ("Device Read Identity Failed:" ? not
sure how to detect the failure), fall back to "-d scsi".
</pre> */
inline hz::ExpectedVoid<StorageDetectorError> detect_drives_linux_adaptec(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives behind Adaptec controller(s)...\n");

	std::vector<std::string> lines;
	std::string error_msg = read_proc_devices_file(lines);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	bool aac_found = false;

	// Check /proc/devices for "aac"
	for (const auto& line : lines) {
		if (app_pcre_match(R"(/^[ \t]*[0-9]+[ \t]+aac(?:[ \t]*|$)/)", hz::string_trim_copy(line))) {
			debug_out_dump("app", DBG_FUNC_MSG << "Found aac entry in devices file.\n");
			aac_found = true;
		}
	}
	if (!aac_found) {
		debug_out_info("app", DBG_FUNC_MSG << "No Adaptec-specific entries found in devices file.\n");
		return {};  // no controllers
	}

	lines.clear();

	debug_out_dump("app", DBG_FUNC_MSG << "Checking scsi file for Adaptec controllers.\n");
	std::vector< std::pair<int, std::string> > vendors_models;
	error_msg = read_proc_scsi_scsi_file(vendors_models);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::vector< std::vector<int> > sg_entries;
	error_msg = read_proc_scsi_sg_devices_file(sg_entries);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::shared_ptr<CommandExecutor> smartctl_ex = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::Smartctl);

	std::set<int> controller_hosts;

	for (const auto& vendors_model : vendors_models) {
		if (!app_pcre_match("/Vendor: Adaptec /i", vendors_model.second)) {
			continue;  // not a supported controller
		}
		int host_num = vendors_model.first;
		debug_out_dump("app", "Found Adaptec controller in SCSI file, SCSI host " << host_num << ".\n");

		// Skip additional adapters with the same host, since they are the same adapters
		// with different LUNs.
		if (controller_hosts.find(host_num) != controller_hosts.end()) {
			debug_out_dump("app", "Skipping adapter with SCSI host " << host_num << ", host already found.\n");
			continue;
		}

		controller_hosts.insert(host_num);

		for (std::size_t sg_num = 0; sg_num < sg_entries.size(); ++sg_num) {
			if (sg_entries[sg_num].size() < 3) {
				// We need at least 3 columns in that file
				continue;
			}
			if (sg_entries[sg_num][0] != host_num || sg_entries[sg_num][2] <= 0) {
				// Different scsi host, or scsi id is 0 (the controller, probably)
				continue;
			}

			std::string dev = std::string("/dev/sg") + hz::number_to_string_nolocale(sg_num);
			auto drive = std::make_shared<StorageDevice>(dev, std::string("sat"));

			auto fetch_status = drive->fetch_basic_data_and_parse(smartctl_ex);
			std::string output = drive->get_basic_output();

			// Note: Not sure about this, have to check with real SAS drives
			if (app_pcre_match("/Device Read Identity Failed/mi", output)) {
				// "-d sat" didn't work, default back to smartctl's "-d scsi"
				drive->clear_fetched();
				drive->set_type_argument("");
				fetch_status = drive->fetch_basic_data_and_parse(smartctl_ex);
			}

			if (!fetch_status) {
				debug_out_info("app", "Smartctl returned with an error: " << fetch_status.error().message() << "\n");
				debug_out_dump("app", "Skipping drive " << drive->get_device_with_type() << ".\n");
			} else {
				drives.push_back(drive);
				debug_out_info("app", "Added drive " << drive->get_device_with_type() << ".\n");
			}
		}
	}

	if (controller_hosts.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Adaptec entry found in devices file, but SCSI file contains no known entries.\n");
	}

	return {};
}



/** <pre>
Areca Linux (arcmsr driver):
Call as:
	- smartctl -i -d areca,N /dev/sg0  (N is [1,24]).
	- smartctl -i -d areca,N/E /dev/sg0  (N is [1,128], E is [1,8]).
The models that have "ix" (case-insensitive) in their model names
have the enclosures and require the N,E syntax.
Note: Using N/E syntax with non-enclosure cards seems to work
regardless of the value of E.

Detection:
	There don't seem to be any entries in /proc/devices.
	Check /proc/scsi/scsi for "Vendor: Areca".
	(Alternatively, /proc/scsi/sg/device_strs should contain "Areca       RAID controller").
Detect SCSI host (N in hostN below):
	It's N of scsiN in /proc/scsi/sg/device_strs of the entry with "Vendor: Areca",
	which has type 3 in /proc/scsi/sg/devices (to filter out logical volumes) and id 16.
	The line number in /proc/scsi/sg/devices is X in /dev/sgX.
Check /sys/bus/scsi/devices/hostN/scsi_host/hostN/host_fw_hd_channels, set
	its contents as the number of ports. If not present, use 24 (maximum).
	No-enclosure models only, since we have no info yet for the enclosure ones.
Probe each port for a valid drive: If the output contains "Device Read Identity Failed"
	or "empty IDENTIFY data", then there is no drive on that port (or Areca has an
	old firmware, nothing we can do there).
Notification: If /sys/bus/scsi/devices/hostN/scsi_host/hostN/host_fw_version
	is older than "V1.46 2009-01-06", (1.51 for enclosure-having cards) notify the user
	(maybe its better to grep the smartctl output for that on port 0?). NOT IMPLEMENTED YET.
</pre> */
inline hz::ExpectedVoid<StorageDetectorError> detect_drives_linux_areca(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives behind Areca controller(s)...\n");

	std::vector< std::pair<int, std::string> > vendors_models;
	std::string error_msg = read_proc_scsi_scsi_file(vendors_models);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::map<int, bool> controller_hosts;  // controller # -> has_enclosure

	for (auto& vendor_model : vendors_models) {
		std::string vendor_line = vendor_model.second;
		if (!app_pcre_match("/Vendor: Areca /i", vendor_line)) {
			continue;  // not a supported controller
		}
		int host_num = vendor_model.first;
		debug_out_dump("app", "Found Areca controller in SCSI file, SCSI host " << host_num << ".\n");

		// Skip additional adapters with the same host (they are volumes)
		if (controller_hosts.find(host_num) != controller_hosts.end()) {
			debug_out_dump("app", "Skipping adapter with SCSI host " << host_num << ", host already found.\n");
			continue;
		}

		// Check if it contains the "ix" string in its model name. It may not be exactly
		// a suffix (there may be "-VOL#00" or something like that appended as well).
		bool has_enclosure = app_pcre_match("/Model:.+ix.+Rev:/i", vendor_line);
		debug_out_dump("app", DBG_FUNC_MSG << "Areca controller (SCSI host " << host_num
				<< ") seems to " << (has_enclosure ? "have enclosure(s)" : "have no enclosures") << ".\n");

		controller_hosts[host_num] = has_enclosure;
	}

	if (controller_hosts.empty()) {
		debug_out_info("app", DBG_FUNC_MSG << "No Areca-specific entries found in SCSI file.\n");
		return {};
	}

	std::vector< std::vector<int> > sg_entries;
	error_msg = read_proc_scsi_sg_devices_file(sg_entries);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::shared_ptr<CommandExecutor> smartctl_ex = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::Smartctl);

	for (auto& iter : controller_hosts) {
		const int host_num = iter.first;
		const bool has_enclosure = iter.second;

		for (std::size_t sg_num = 0; sg_num < sg_entries.size(); ++sg_num) {
			if (sg_entries[sg_num].size() < 5) {
				// We need at least 5 columns in that file
				continue;
			}
			const int type = sg_entries[sg_num][4];  // type 3 is Areca controller
			const int id = sg_entries[sg_num][2];  // id should be 16 (as per smartmontools)
			if (sg_entries[sg_num][0] != host_num || id != 16 || type != 3) {
				continue;
			}

			hz::ExpectedVoid<StorageDetectorError> exec_status;

			if (has_enclosure) {
				// TODO We have no information on what "/sys/bus/scsi/devices/host%d/scsi_host/host%d/host_fw_hd_channels"
				// contains in case of enclosure-having cards.

				int max_enclosures = rconfig::get_data<int>("system/linux_areca_enc_max_enclosure");
				max_enclosures = std::max(1, std::min(8, max_enclosures));  // 1-8

				int max_ports = rconfig::get_data<int>("system/linux_areca_enc_max_scan_port");
				max_ports = std::max(1, std::min(128, max_ports));  // 1-128 sanity check

				std::string dev = std::string("/dev/sg") + hz::number_to_string_nolocale(sg_num);

				debug_out_dump("app", "Starting brute-force port/enclosure scan on 1-" << max_ports << " ports, 1-" << max_enclosures << " enclosures, device \"" << dev
						<< "\". Change the maximums by setting \"system/linux_areca_enc_max_scan_port\" and \"system/linux_areca_enc_max_enclosure\" config keys.\n");
				std::string last_output;
				for (int enclosure_no = 1; enclosure_no < max_enclosures; ++enclosure_no) {
					exec_status = smartctl_scan_drives_sequentially(dev, "areca,%d/" + hz::number_to_string_nolocale(enclosure_no), 1, max_ports, drives, ex_factory, last_output);
				}
				debug_out_dump("app", "Brute-force port/enclosure scan finished.\n");

			} else {
				int max_ports = 0;

				// Read the number of ports.
				auto ports_file = hz::fs_path_from_string(hz::string_sprintf("/sys/bus/scsi/devices/host%d/scsi_host/host%d/host_fw_hd_channels", host_num, host_num));
				std::string ports_file_contents;
				auto ec = read_proc_file(ports_file, ports_file_contents);
				if (ec) {
					debug_out_warn("app", DBG_FUNC_MSG << "Couldn't read the number of ports on Areca controller (\"" << ports_file.string() << "\"): "
							<< ec.message() << ", trying manually.\n");
				} else {
					hz::string_is_numeric_nolocale(hz::string_trim_copy(ports_file_contents), max_ports);
					debug_out_dump("app", DBG_FUNC_MSG << "Detected " << max_ports << " ports, through \"" << ports_file.string() << "\".\n");
				}
				if (max_ports == 0) {
					max_ports = rconfig::get_data<int>("system/linux_areca_noenc_max_scan_port");
				}
				max_ports = std::max(1, std::min(24, max_ports));  // 1-24 sanity check

				std::string dev = std::string("/dev/sg") + hz::number_to_string_nolocale(sg_num);
				debug_out_dump("app", "Starting brute-force port scan on 1-" << max_ports << " ports, device \"" << dev
						<< "\". Change the maximum by setting \"system/linux_areca_noenc_max_scan_port\" config key.\n");
				std::string last_output;
				exec_status = smartctl_scan_drives_sequentially(dev, "areca,%d", 1, max_ports, drives, ex_factory, last_output);
				debug_out_dump("app", "Brute-force port scan finished.\n");
			}

			if (!exec_status) {
				debug_out_warn("app", DBG_FUNC_MSG << "Couldn't get the drives on ports of Areca controller: " << exec_status.error().message() << "\n");
			}
		}
	}

	return {};
}



/** <pre>
Detect drives behind HP RAID controller (cciss driver).
(aka CCISS (HP (Compaq) Smart Array Controller).

Note: hpsa/hpahcisr have a different method of detection.

Call as: smartctl -a -d cciss,[0-127] /dev/cciss/c0d0
	With the cciss drivers , for the path /dev/cciss/c0d0, c0 means controler 0 and d0 means LUN 1.
	So smarctl provides the same values for c0d0 and c0d1. There are just two logical drives
	on the same Smart Array controler.
To detect controller presence: cat /proc/devices, contains "cciss0"
	for controller 0.
	/proc/scsi/scsi contains no entries for it.
/dev/cciss/c0d0p1 is the first ordinary partition (used in mount, etc...).

The port limit had a brief regression in 5.39.x (limited to 15).
5.39 doesn't seem to work at all (prereleases work with P400 controller)?
	Possibly a 64-bit-only issue?
There is a cciss_vol_status utility but it doesn't report anything except "OK" status.
It may not return the "SMART Health Status: OK" line in smartctl -i for some reason?
	(It returns it in -a though, and in some freebsd output I found).
If no /dev/cciss exists, you may need to run "cd /dev; ./MAKEDEV cciss".

Detection:
	Grep /proc/devices for "cciss([0-9]+)" where the number is the controller #.
	Run "smartctl -i -d cciss,[0-127] /dev/cciss/cNd0" (where N is the controller #),
		until "No such device or address" or "VALID ARGUMENTS ARE" is encountered in the output.
	Note: We're not sure how to differentiate the outputs of free / non-existent ports,
		so scan them until 15, just in case.
</pre> */
inline hz::ExpectedVoid<StorageDetectorError> detect_drives_linux_cciss(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives behind HP RAID (CCISS) controller(s)...\n");

	std::vector<std::string> lines;
	std::string error_msg = read_proc_devices_file(lines);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::vector<int> controllers;

	// Check /proc/devices for ccissX (where X is the controller #).
	for (const auto& line : lines) {
		std::string controller_no_str;
		if (app_pcre_match(R"(/^[ \t]*[0-9]+[ \t]+cciss([0-9]+)(?:[ \t]*|$)/)", hz::string_trim_copy(line), &controller_no_str)) {
			debug_out_dump("app", DBG_FUNC_MSG << "Found cciss" << controller_no_str << " entry in devices file.\n");
			controllers.push_back(hz::string_to_number_nolocale<int>(controller_no_str));
		}
	}
	if (controllers.empty()) {
		debug_out_info("app", DBG_FUNC_MSG << "No cciss-specific entries found in devices file.\n");
		return {};  // no controllers
	}

	std::shared_ptr<CommandExecutor> smartctl_ex = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::Smartctl);

	for (int controller_no : controllers) {
		std::string dev = std::string("/dev/cciss/c") + hz::number_to_string_nolocale(controller_no) + "d0";

		const int max_port = 127;
		debug_out_dump("app", "Starting brute-force port scan on 1-" << max_port << " ports, device \"" << dev << "\".\n");

		for (int port = 0; port <= max_port; ++port) {
			auto drive = std::make_shared<StorageDevice>(dev, std::string("cciss,") + hz::number_to_string_nolocale(port));

			auto fetch_status = drive->fetch_basic_data_and_parse(smartctl_ex);
			std::string output = drive->get_basic_output();

			if (!fetch_status) {
				debug_out_info("app", "Smartctl returned with an error: " << fetch_status.error().message() << "\n");
			}

			if (app_pcre_match("/VALID ARGUMENTS ARE/mi", output)) {
				// smartctl doesn't support this many ports, return.
				debug_out_dump("app", "Reached smartctl port limit with port " << port << ", stopping port scan.\n");
				break;
			}
			if (app_pcre_match("/No such device or address/mi", output) && port > 15) {
				// we've reached the controller port limit
				debug_out_dump("app", "Reached controller port limit with port " << port << ", stopping port scan.\n");
				break;
			}

			if (fetch_status) {
				drives.push_back(drive);
				debug_out_info("app", "Added drive " << drive->get_device_with_type() << ".\n");
			} else {
				debug_out_dump("app", "Skipping drive " << drive->get_device_with_type() << " due to smartctl error.\n");
			}
		}

		debug_out_dump("app", "Brute-force port scan finished.\n");
	}

	return {};
}



/** <pre>
Detect drives behind HP RAID controller (hpsa / hpahcisr drivers).

May need "-d sat+cciss,N" for SATA, but is usually auto-detected.

smartctl -a -d cciss,0 /dev/sg2    (hpsa or hpahcisr drivers under Linux)
("lsscsi -g" is helpful in determining which scsi generic device node corresponds to which device.)
Use the nodes corresponding to the RAID controllers, not the nodes corresponding to logical drives.
No entry in /proc/devices.
/dev/sda also seems to work?

Detection:
	Check /proc/scsi for "Vendor: HP", and NOT "Model: LOGICAL VOLUME". Take its scsi host number,
		match against /proc/scsi/sg/devices (first column). The matched line number
		is the one to use in /dev/sgN.
	Run smartctl -i -d cciss,[0-127] /dev/cciss/cNd0
		until "No such device or address" or "VALID ARGUMENTS ARE" is encountered in output.
</pre> */
inline hz::ExpectedVoid<StorageDetectorError> detect_drives_linux_hpsa(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives behind HP RAID (hpsa/hpahcisr) controller(s)...\n");

	std::vector< std::pair<int, std::string> > vendors_models;
	std::string  error_msg = read_proc_scsi_scsi_file(vendors_models);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::vector< std::vector<int> > sg_entries;
	error_msg = read_proc_scsi_sg_devices_file(sg_entries);
	if (!error_msg.empty()) {
		return hz::Unexpected(StorageDetectorError::ProcReadError, error_msg);
	}

	std::shared_ptr<CommandExecutor> smartctl_ex = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::Smartctl);

	std::set<int> controller_hosts;

	for (auto& vendor_model : vendors_models) {
		if (!app_pcre_match("/Vendor: HP /i", vendor_model.second) || app_pcre_match("/LOGICAL VOLUME/i",
				vendor_model.second)) {
			continue;  // not a supported controller, or a logical drive.
		}
		int host_num = vendor_model.first;
		debug_out_dump("app", "Found HP controller in SCSI file, SCSI host " << host_num << ".\n");

		// Skip additional adapters with the same host (not sure if this is needed, but it won't hurt).
		if (controller_hosts.find(host_num) != controller_hosts.end()) {
			debug_out_dump("app", "Skipping adapter with SCSI host " << host_num << ", host already found.\n");
			continue;
		}

		controller_hosts.insert(host_num);

		for (std::size_t sg_num = 0; sg_num < sg_entries.size(); ++sg_num) {
			if (sg_entries[sg_num].size() < 3) {
				// We need at least 3 columns in that file
				continue;
			}
			if (sg_entries[sg_num][0] != host_num) {
				// Different scsi host
				continue;
			}

			std::string dev = std::string("/dev/sg") + hz::number_to_string_nolocale(sg_num);

			const int max_port = 127;
			debug_out_dump("app", "Starting brute-force port scan on 0-" << max_port << " ports, device \"" << dev << "\".\n");

			for (int port = 0; port <= max_port; ++port) {
				auto drive = std::make_shared<StorageDevice>(dev, std::string("cciss,") + hz::number_to_string_nolocale(port));

				auto fetch_status = drive->fetch_basic_data_and_parse(smartctl_ex);
				std::string output = drive->get_basic_output();

				if (app_pcre_match("/No such device or address/mi", output) || app_pcre_match("/VALID ARGUMENTS ARE/mi", output)) {
					// We reached the controller port limit, or smartctl-supported port limit.
					debug_out_dump("app", "Reached controller or smartctl port limit with port " << port << ", stopping port scan.\n");
					break;
				}
				if (!fetch_status) {
					debug_out_info("app", "Smartctl returned with an error: " << fetch_status.error().message() << "\n");
					debug_out_dump("app", "Skipping drive " << drive->get_device_with_type() << " due to smartctl error.\n");
				} else {
					drives.push_back(drive);
					debug_out_info("app", "Added drive " << drive->get_device_with_type() << ".\n");
				}
			}

			debug_out_dump("app", "Brute-force port scan finished.\n");
		}
	}

	if (controller_hosts.empty()) {
		debug_out_info("app", DBG_FUNC_MSG << "No hpsa/hpahcisr-specific entries found in Sg devices file.\n");
	}

	return {};
}



}  // anon ns




hz::ExpectedVoid<StorageDetectorError> detect_drives_linux(
		std::vector<StorageDevicePtr>& drives, const CommandExecutorFactoryPtr& ex_factory)
{
	clear_read_file_cache();

	std::vector<std::string> error_msgs;
	hz::ExpectedVoid<StorageDetectorError> status;

	// Disable by-id detection - it's unreliable on broken systems.
	// For example, on Ubuntu 8.04, /dev/disk/by-id contains two device
	// links for two drives, but both point to the same sdb (instead of
	// sda and sdb). Plus, there are no "*-partN" files (not that we need them).
// 	error_message = detect_drives_linux_udev_byid(devices);  // linux udev

	status = detect_drives_linux_proc_partitions(drives, ex_factory);
	if (!status) {
		error_msgs.push_back(status.error().message());
	}

	status = detect_drives_linux_3ware(drives, ex_factory);
	if (!status) {
		error_msgs.push_back(status.error().message());
	}

	status = detect_drives_linux_areca(drives, ex_factory);
	if (!status) {
		error_msgs.push_back(status.error().message());
	}

	status = detect_drives_linux_adaptec(drives, ex_factory);
	if (!status) {
		error_msgs.push_back(status.error().message());
	}

	status = detect_drives_linux_cciss(drives, ex_factory);
	if (!status) {
		error_msgs.push_back(status.error().message());
	}

	status = detect_drives_linux_hpsa(drives, ex_factory);
	if (!status) {
		error_msgs.push_back(status.error().message());
	}

	if (!error_msgs.empty()) {
		return hz::Unexpected(StorageDetectorError::GeneralDetectionErrors, hz::string_join(error_msgs, "\n"));
	}

	return {};
}





/// @}
