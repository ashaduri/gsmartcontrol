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
#ifndef STORAGE_DEVICE_DETECTED_TYPE_H
#define STORAGE_DEVICE_DETECTED_TYPE_H

#include "local_glibmm.h"

#include "hz/enum_helper.h"


/// These may be used to force smartctl to a special type, as well as
/// to display the correct icon
enum class StorageDeviceDetectedType {
	Unknown,  ///< Unknown, default state.
	NeedsExplicitType,  ///< This is set by smartctl executor if it detects the need for -d option.
	AtaAny,  ///< Any ATA device (HDD or SSD), before it is detected whether it's HDD or SSD.
	AtaHdd,  ///< ATA HDD
	AtaSsd,  ///< ATA SSD
	Nvme,  ///< NVMe device (SSD)
	BasicScsi,  ///< Basic SCSI device (no smart data). Usually flash drives, etc.
	CdDvd,  ///< CD/DVD/Blu-Ray. Blu-ray is not always detected.
	UnsupportedRaid,  ///< RAID controller or volume. Unsupported by smartctl, only basic info is given.
};



/// Helper structure for enum-related functions
struct StorageDeviceDetectedTypeExt
		: public hz::EnumHelper<
				StorageDeviceDetectedType,
				StorageDeviceDetectedTypeExt,
				Glib::ustring>
{
	static constexpr inline StorageDeviceDetectedType default_value = StorageDeviceDetectedType::Unknown;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{StorageDeviceDetectedType::Unknown, {"unknown", _("Unknown")}},
			{StorageDeviceDetectedType::NeedsExplicitType, {"needs_explicit_type", _("Needs Explicit Type")}},
			{StorageDeviceDetectedType::AtaAny, {"ata_any", _("ATA Device (HDD or SSD)")}},
			{StorageDeviceDetectedType::AtaHdd, {"ata_hdd", _("ATA HDD")}},
			{StorageDeviceDetectedType::AtaSsd, {"ata_ssd", _("ATA SSD")}},
			{StorageDeviceDetectedType::Nvme, {"nvme", _("NVMe Device")}},
			{StorageDeviceDetectedType::BasicScsi, {"basic_scsi", _("Basic SCSI Device")}},
			{StorageDeviceDetectedType::CdDvd, {"cd_dvd", _("CD/DVD/Blu-Ray")}},
			{StorageDeviceDetectedType::UnsupportedRaid, {"unsupported_raid", _("Unsupported RAID Controller or Volume")}},
		};
	}

};







#endif

/// @}
