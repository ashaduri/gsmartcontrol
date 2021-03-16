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

#include "build_config.h"  // CONFIG_*

#if !defined CONFIG_KERNEL_LINUX && !defined CONFIG_KERNEL_FAMILY_WINDOWS

#include "local_glibmm.h"
#include <algorithm>  // std::sort

#if defined CONFIG_KERNEL_OPENBSD || defined CONFIG_KERNEL_NETBSD
	#include <util.h>  // getrawpartition()
#endif

#include "hz/debug.h"
#include "hz/fs.h"
#include "rconfig/rconfig.h"
#include "app_pcrecpp.h"
#include "storage_detector_other.h"




std::string detect_drives_other(std::vector<StorageDevicePtr>& drives,
		[[maybe_unused]] const CommandExecutorFactoryPtr& ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives through /dev...\n");

	std::vector<std::string> devices;

	std::string sdev_config_path;
	#if defined CONFIG_KERNEL_SOLARIS
		sdev_config_path = "system/solaris_dev_path";
	#else  // other unixes
		sdev_config_path = "system/unix_sdev_path";
	#endif

	// defaults to /dev for freebsd, /dev/rdsk for solaris.
	auto dev_dir = rconfig::get_data<std::string>(sdev_config_path);
	if (dev_dir.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Device directory path is not set.\n");
		return _("Device directory path is not set.");
	}

	auto dir = hz::fs::u8path(dev_dir);
	std::error_code dummy_ec;
	if (!hz::fs::exists(dir, dummy_ec)) {
		debug_out_warn("app", DBG_FUNC_MSG << "Device directory doesn't exist.\n");
		return _("Device directory does not exist.");
	}


#if defined CONFIG_KERNEL_FREEBSD || defined CONFIG_KERNEL_DRAGONFLY
	// FreeBSD example device names:
	// * /dev/ad0 - ide disk 0. /dev/ad0a - non-slice dos partition 1.
	// * /dev/da1s1e - scsi disk 1 (second disk), slice 1 (aka dos partition 1),
	// bsd partition e (multiple bsd partitions may be inside one dos partition).
	// * /dev/acd0 - first cdrom. /dev/acd0c - ?.
	// Info: http://www.freebsd.org/doc/en/books/handbook/disks-naming.html

	// Of two machines I had access to, freebsd 6.3 had only real devices,
	// while freebsd 4.10 had lots of dummy ones (ad1, ad2, ad3, da0, da1, da2, da3, sa0, ...).

	static const std::vector<std::string> whitelist = {
		"/^ad[0-9]+$/",  // adN without suffix - fbsd ide
		"/^da[0-9]+$/",  // daN without suffix - fbsd scsi, usb
		"/^ada[0-9]+$/",  // adaN without suffix - fbsd ata cam
	// 	"/	^sa[0-9]+$/",  // saN without suffix - fbsd scsi tape
	// 	"/^ast[0-9]+$/",  // astN without suffix - fbsd ide tape
		"/^aacd[0-9]+$/",  // fbsd adaptec raid
		"/^mlxd[0-9]+$/",  // fbsd mylex raid
		"/^mlyd[0-9]+$/",  // fbsd mylex raid
		"/^amrd[0-9]+$/",  // fbsd AMI raid
		"/^idad[0-9]+$/",  // fbsd compaq raid
		"/^twed[0-9]+$/",  // fbsd 3ware raid
		// these are checked by smartctl, but they are not mentioned in freebsd docs:
		"/^tw[ae][0-9]+$/",  // fbsd 3ware raid
	};
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

	static const std::vector<std::string> whitelist = {
		"/^c[0-9]+(?:t[0-9]+)?d[0-9]+s0$/"
	};



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

	static const std::vector<std::string> whitelist = {
		hz::string_sprintf("/^wd[0-9]+%c$/", whole_part),
		hz::string_sprintf("/^sd[0-9]+%c$/", whole_part),
		hz::string_sprintf("/^st[0-9]+%c$/", whole_part),
	};



#elif defined CONFIG_KERNEL_DARWIN

	// Darwin has /dev/disk0, /dev/disk0s1, etc...
	// Only real devices are present, so no need for additional checks.

	static const std::vector<std::string> whitelist = {
		"/^disk[0-9]+$/"
	};


#elif defined CONFIG_KERNEL_QNX

	// QNX has /dev/hd0, /dev/hd0t78 (partition?).
	// Afaik, IDE and SCSI have the same prefix. fd for floppy, cd for cdrom.
	// Not sure about the tapes.
	// Only real devices are present, so no need for additional checks.

	static const std::vector<std::string> whitelist = {
		"/^hd[0-9]+$/",
	};

#else  // unsupported OS

	static const std::vector<std::string> whitelist;

#endif  // unix platforms


	std::vector<hz::fs::path> matched_devices;
	std::error_code ec;
	for(const auto& entry : hz::fs::directory_iterator(dir, ec)) {
		const auto& path = entry.path();

		bool matched = false;
		for (const auto& wl_pattern : whitelist) {
			if (app_pcre_match(wl_pattern, path.filename().string())) {
				matched = true;
				break;
			}
		}
		if (!matched)
			continue;

		// In case these are links, check if the originals exists (solaris has dangling links, filter them out).
		// We don't replace /dev files with real devices - it leads to really bad paths (pci ids for solaris, etc...).
		if (!hz::fs::exists(path)) {
			continue;
		}
		matched_devices.push_back(path);
	}
	if (ec) {
		debug_out_error("app", DBG_FUNC_MSG << "Cannot list device directory entries.\n");
		return Glib::ustring::compose(_("Cannot list device directory entries: %1"), ec.message());
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

		for (const auto& dev : matched_devices) {
			if (open_needed) {
				std::FILE* fp = hz::fs_platform_fopen(dev, "rb");
				if (!fp && errno == ENXIO) {
					debug_out_dump("app", DBG_FUNC_MSG << "Device \"" << dev.string() << "\" failed to open, ignoring.\n");
					continue;
				}
				if (fp) {
					std::fclose(fp);
				}
				debug_out_info("app", DBG_FUNC_MSG << "Device \"" << dev.string() << "\" opened successfully, adding to device list.\n");
			}
			devices.push_back(dev.string());
		}

	#else  // not *BSD
		for (const auto& dev : matched_devices) {
			debug_out_info("app", DBG_FUNC_MSG << "Device \"" << dev << "\" matched the whitelist, adding to device list.\n");
			devices.push_back(dev);
		}

	#endif

	for (auto& device : devices) {
		drives.emplace_back(std::make_shared<StorageDevice>(device));
	}

	return std::string();
}





#endif  // !defined CONFIG_KERNEL_LINUX && !defined CONFIG_KERNEL_FAMILY_WINDOWS

/// @}
