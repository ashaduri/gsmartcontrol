/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include "hz/hz_config.h"  // CONFIG_*

#include <ios>  // std::boolalpha
#include <algorithm>  // std::find
#include <cstdio>  // std::fgets(), std::FILE
#include <cerrno>  // ENXIO

#if defined CONFIG_KERNEL_OPENBSD || defined CONFIG_KERNEL_NETBSD
	#include <util.h>  // getrawpartition()
#endif

#if defined CONFIG_KERNEL_FAMILY_WINDOWS
	#include <windows.h>  // CreateFileA(), CloseHandle(), etc...
#endif

#include "hz/debug.h"
#include "hz/fs_dir.h"
#include "hz/fs_file.h"
#include "hz/string_sprintf.h"
#include "hz/local_algo.h"  // hz::shell_sort()
#include "rconfig/rconfig_mini.h"

#include "app_pcrecpp.h"
#include "smartctl_executor.h"
#include "storage_detector.h"


namespace {


	// Note: Since 3ware RAID controllers need additional flags on most (all?) OSes,
	// and they may ignore the actual devices names, I have no idea how to support them.


#if defined CONFIG_KERNEL_LINUX


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



	// Procfs files don't support SEEK_END or ftello() (I think). Anyway, they can't
	// be read through hz::File::get_contents, so use this function instead.
	inline bool read_proc_partitions_file(hz::File& file, std::vector<std::string>& lines)
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



	// Linux (tested with 2.4 and 2.6) /proc/partitions. Parses the file, appends /dev to each entry.
	// Note that file format changed from 2.4 to 2.6 (some statistics fields moved to another file).
	// No /proc/partitions on at least freebsd, solaris and osx, afaik.
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
		debug_out_info("app", DBG_FUNC_MSG << "Detecting through /proc/partitions...\n");

		std::string parts_file;
		if (!rconfig::get_data("system/linux_proc_partitions_path", parts_file) || parts_file.empty()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Partitions file path is not set.\n");
			return "Partitions file path is not set.";
		}

		hz::File file(parts_file);
// 		std::string contents;
		std::vector<std::string> lines;
// 		if (!f.get_contents(contents)) {
		if (!read_proc_partitions_file(file, lines)) {  // this outputs to debug too
			std::string error_msg = file.get_error_utf8();  // save before calling other file functions
			if (!file.exists()) {
				debug_out_warn("app", DBG_FUNC_MSG << "Partitions file doesn't exist.\n");
			} else {
				debug_out_error("app", DBG_FUNC_MSG << "Partitions file exists but cannot be read.\n");
			}
			return error_msg;
		}

// 		debug_out_dump("app", DBG_FUNC_MSG << "Dumping partitions file:\n" << contents << "\n");

// 		hz::string_split(contents, '\n', lines, true);

		std::vector<std::string> blacklist;
		// fixme: not sure about how partitions are visible with twa0.
		blacklist.push_back("/d[a-z][0-9]+$/");  // sda1, hdb2 - partitions. twa0 and twe1 are drives, not partitions.
		blacklist.push_back("/ram[0-9]+$/");  // ramdisks?
		blacklist.push_back("/loop[0-9]*$/");  // not sure if loop devices go there, but anyway...
		blacklist.push_back("/part[0-9]+$/");  // devfs had them
		blacklist.push_back("/p[0-9]+$/");  // partitions are usually marked this way
		blacklist.push_back("/md[0-9]*$/");  // linux software raid


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

			std::string path = "/dev/" + dev;  // let's just hope the it's /dev.
			if (std::find(devices.begin(), devices.end(), path) == devices.end())  // there may be duplicates
				devices.push_back(path);
		}

		return std::string();
	}



#elif defined CONFIG_KERNEL_FAMILY_WINDOWS


	// smartctl accepts various variants, the most straight being pdN,
	// (or /dev/pdN, /dev/ being optional) where N comes from
	// "\\.\PhysicalDriveN" (winnt only).

	inline std::string detect_drives_win32(std::vector<std::string>& devices)
	{
		for (int drive_num = 0; ; ++drive_num) {
			std::string name = hz::string_sprintf("\\\\.\\PhysicalDrive%d", drive_num);

			// If the drive is openable, then it's there.
			// NOTE: Administrative privileges are required to open it.
			// Yes, CreateFile() is open, not create. Yes, it's silly (ah, win32...).
			// We don't use any long/unopenable files here, so use the ANSI version.
			HANDLE h = CreateFileA(name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, 0, NULL);

			// The numbers seem to be consecutive, so break on first invalid.
			if (h == INVALID_HANDLE_VALUE)
				break;

			CloseHandle(h);

			devices.push_back(hz::string_sprintf("pd%d", drive_num));
		}

		return std::string();
	}




#else  // FreeBSD, Solaris, etc... .


	inline std::string detect_drives_dev(std::vector<std::string>& devices)
	{
		debug_out_info("app", DBG_FUNC_MSG << "Detecting through /dev...\n");

		std::string sdev_config_path;
		#if defined CONFIG_KERNEL_SOLARIS
			sdev_config_path = "system/solaris_dev_path";
		#else  // other unixes
			sdev_config_path = "system/unix_sdev_path";
		#endif

		std::string dev_dir;  // defaults to /dev for freebsd, /dev/rdsk for solaris.
		if (!rconfig::get_data(sdev_config_path, dev_dir) || dev_dir.empty()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Device directory path is not set.\n");
			return "Device directory path is not set.";
		}

		hz::Dir dir(dev_dir);

		std::vector<std::string> all_devices;
		if (!dir.list(all_devices, false, hz::DirSortNone())) {  // this outputs to debug too.
			std::string error_msg = dir.get_error_utf8();
			if (!dir.exists()) {
				debug_out_warn("app", DBG_FUNC_MSG << "Device directory doesn't exist.\n");
			} else {
				debug_out_error("app", DBG_FUNC_MSG << "Cannot list directory entries.\n");
			}
			return error_msg;
		}


		// platform whitelist
		std::vector<std::string> whitelist;


	#if defined CONFIG_KERNEL_FREEBSD || defined CONFIG_KERNEL_DRAGONFLY
		// FreeBSD example device names:
		// * /dev/ad0 - ide disk 0. /dev/ad0a - non-slice dos partition 1.
		// * /dev/da1s1e - scsi disk 1 (second disk), slice 1 (aka dos partition 1),
		// bsd partition e (multiple bsd partitions may be inside one dos partition).
		// * /dev/acd0 - first cdrom. /dev/acd0c - ?.
		// Info: http://www.freebsd.org/doc/en/books/handbook/disks-naming.html

		// Of two machines I had access to, freebsd 6.3 had only real devices,
		// while freebsd 4.10 had lots of dummy ones (ad1, ad2, ad3, da0, da1, da2, da3, sa0, ...).

		whitelist.push_back("/^ad[0-9]+$/");  // adN without suffix - fbsd ide
		whitelist.push_back("/^da[0-9]+$/");  // daN without suffix - fbsd scsi, usb
	// 	whitelist.push_back("/	^sa[0-9]+$/");  // saN without suffix - fbsd scsi tape
	// 	whitelist.push_back("/^ast[0-9]+$/");  // astN without suffix - fbsd ide tape
		whitelist.push_back("/^aacd[0-9]+$/");  // fbsd adaptec raid
		whitelist.push_back("/^mlxd[0-9]+$/");  // fbsd mylex raid
		whitelist.push_back("/^mlyd[0-9]+$/");  // fbsd mylex raid
		whitelist.push_back("/^amrd[0-9]+$/");  // fbsd AMI raid
		whitelist.push_back("/^idad[0-9]+$/");  // fbsd compaq raid
		whitelist.push_back("/^twed[0-9]+$/");  // fbsd 3ware raid
		// these are checked by smartctl, but they are not mentioned in freebsd docs:
		whitelist.push_back("/^tw[ae][0-9]+$/");  // fbsd 3ware raid
		// unused: acd - ide cdrom, cd - scsi cdrom, mcd - mitsumi cdrom, scd - sony cdrom,
		// fd - floppy, fla - diskonchip flash.


	#elif defined CONFIG_KERNEL_SOLARIS

		// /dev/rdsk contains "raw" physical char devices, as opposed to
		// "filesystem" block devices in /dev/dsk. smartctl needs rdsk.

		// x86:

		// /dev/rdsk/c0t0d0p0:1
		// Where c0 is the controller number.
		// t0 is the target (SCSI ID number) (omit for ATAPI)
		// d0 is always 0 for SCSI, the drive # for ATAPI
		// p0 is the partition (p0 is the entire disk, or p1 - p4)
		// :1 is the logical drive (c - z or 1 - 24) (that is, logical partitions inside extended partition)

		// /dev/rdsk/c0d0p2:1
		// where p2 means the extended partition (type 0x05) is partition 2 (out of 1-4) and
		// ":1" is the 2nd extended partition (:0 would be the first extended partition).  -- not sure about this!

		// p0 whole physical disk
		// p1 - p4 Four primary fdisk partitions
		// p5 - p30 26 logical drives in extended DOS partition (not implemented by Solaris)
		// s0 - s15 16 slices in the Solaris FDISK partition (SPARC has only 8)
		// On Solaris, by convention, s2 is the whole Solaris partition.

		// SPARC: There are no *p* devices on sparc afaik, only slices. By convention,
		// s2 is used as a "whole" disk there (but not with EFI partitions!).
		// So, c0d0s2 is the whole disk there.
		// x86 has both c0d0s2 and c0d0p2, where s2 is a whole solaris partition.

		// NOTE: It seems that smartctl searches for s0 (aka root partition) in the end. I'm not sure
		// what implications this has for x86, but we replicate this behaviour.

		// Note: Cdroms are ATAPI, so they pose as SCSI, as opposed to IDE hds.

		// TODO: No idea how to implement /dev/rmt (scsi tape devices),
		// I have no files in that directory.

		whitelist.push_back("/^c[0-9]+(?:t[0-9]+)?d[0-9]+s0$/");



	#elif defined CONFIG_KERNEL_OPENBSD || defined CONFIG_KERNEL_NETBSD

		// OpenBSD / NetBSD have /dev/wdNc for IDE/ATA, /dev/sdNc for SCSI disk,
		// /dev/stNc for scsi tape. N is [0-9]+. "c" means "whole disk" (not sure about
		// different architectures though). There are no "sdN" devices, only "sdNP".
		// Another manual says that wd0d would be a whole disk, while wd0c is its
		// bsd part only (on x86) (only on netbsd?). Anyway, getrawpartition() gives
		// us the letter we need.
		// There is no additional level of names for BSD subpartitions (unlike freebsd).
		// cd0a is cdrom.
		// Dummy devices are present.

		// Note: This detection may take a while (probably due to open() check).

		char whole_part = 'a' + getrawpartition();  // from bsd's util.h

		whitelist.push_back(hz::string_sprintf("/^wd[0-9]+%c$/", whole_part));
		whitelist.push_back(hz::string_sprintf("/^sd[0-9]+%c$/", whole_part));
		whitelist.push_back(hz::string_sprintf("/^st[0-9]+%c$/", whole_part));



	#elif defined CONFIG_KERNEL_DARWIN

		// Darwin has /dev/disk0, /dev/disk0s1, etc...
		// Only real devices are present, so no need for additional checks.

		whitelist.push_back("/^disk[0-9]+$/");



	#elif defined CONFIG_KERNEL_QNX

		// QNX has /dev/hd0, /dev/hd0t78 (partition?).
		// Afaik, IDE and SCSI have the same prefix. fd for floppy, cd for cdrom.
		// Not sure about the tapes.
		// Only real devices are present, so no need for additional checks.

		whitelist.push_back("/^hd[0-9]+$/");


	#endif  // unix platforms


		std::vector<std::string> matched_devices;

		for (unsigned int i = 0; i < all_devices.size(); ++i) {
			std::string entry = all_devices[i];
			if (entry == "." || entry == "..")
				continue;

			bool matched = false;
			for (std::vector<std::string>::const_iterator iter = whitelist.begin(); iter != whitelist.end(); ++iter) {
				if (app_pcre_match(*iter, entry)) {
					matched = true;
					break;
				}
			}
			if (!matched)
				continue;

			hz::FsPath path = dev_dir + hz::DIR_SEPARATOR_S + all_devices[i];

			// In case these are links, trace them to originals.
			// We don't replace /dev files with real devices - it leads to really bad paths (pci ids for solaris, etc...).
			std::string link_dest;
			hz::FsPath real_dev_path;
			if (path.get_link_destination(link_dest)) {
				real_dev_path = (hz::path_is_absolute(link_dest) ? link_dest : (dev_dir + hz::DIR_SEPARATOR_S + link_dest));
				real_dev_path.compress();  // compress in case there are ../-s.

				// solaris has dangling links for non-existant devices, so filter them out.
				if (!real_dev_path.exists())
					continue;
			}

			matched_devices.push_back(path.str());
		}


		// List ones who need dummy device filtering
		#if defined CONFIG_KERNEL_FREEBSD || defined CONFIG_KERNEL_DRAGONFLY \
				|| defined CONFIG_KERNEL_OPENBSD || defined CONFIG_KERNEL_NETBSD
			// Since we may have encountered dummy devices, we should check
			// if they really exist. Unfortunately, this involves opening them, which
			// is not good with some media (e.g. it may hang on audio cd (freebsd, maybe others), etc...).
			// That's why we don't whitelist cd devices. Let's hope usb and others are
			// ok with this. Dummy devices give errno ENXIO (6, Device not configured) on *bsd.
			// In Linux ENXIO is (6, No such device or address).

			// Don't do this on solaris - we can't distinguish between cdroms and hds there.

			// If there are less than 4 devices, they are probably not dummy (newer freebsd).
			bool open_needed = (matched_devices.size() >= 4);
			if (open_needed) {
				debug_out_info("app", DBG_FUNC_MSG << "Number of matched devices is "
						<< matched_devices.size() << ", will try to filter non-existent ones out.\n");
			} else {
				debug_out_info("app", DBG_FUNC_MSG << "Number of matched devices is "
						<< matched_devices.size() << ", no need for filtering them out.\n");
			}

			for (unsigned int i = 0; i < matched_devices.size(); ++i) {
				hz::File dev_file(matched_devices[i]);

				// we check errno because OS may deny us for some other reason on valid devices.
				if (open_needed && !dev_file.open("rb") && dev_file.get_errno() == ENXIO) {
					debug_out_dump("app", DBG_FUNC_MSG << "Device \"" << dev_file.str() << "\" failed to open, ignoring.\n");
					continue;
				}

				if (open_needed)
					debug_out_dump("app", DBG_FUNC_MSG << "Device \"" << dev_file.str() << "\" opened successfully, adding to device list.\n");

				devices.push_back(dev_file.str());
			}

		#else  // not *BSD
			for (unsigned int i = 0; i < matched_devices.size(); ++i) {
				devices.push_back(matched_devices[i]);
			}

		#endif

		hz::shell_sort(devices.begin(), devices.end());

		return std::string();
	}


#endif  // platforms



}  // anon. ns




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
		error_msg = detect_drives_dev(devices);  // bsd, etc... . scans /dev.
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
				<< "\tType: " << StorageDevice::get_type_readable_name(drive->get_type()) << "\n"
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






