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

#include "local_glibmm.h"
#include <utility>
#include <vector>
#include <map>
#include <unordered_map>

#include "hz/string_algo.h"  // string_replace_copy
#include "applib/app_pcrecpp.h"

#include "ata_storage_property_descr.h"



namespace {


	/// Get text related to "uncorrectable sectors"
	const std::string& get_uncorrectable_text()
	{
		static const std::string text = Glib::Markup::escape_text(
				_("When a drive encounters a surface error, it marks that sector as \"unstable\" (also known as \"pending reallocation\"). "
				"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
				"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
				"transferring the data so that no changes are reported to the operating system. This is why you generally don't see \"bad blocks\" "
				"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area."
				"\n\nNote: SSDs reallocate blocks as part of their normal operation, so low reallocation counts are not critical for them."));
		return text;
	}


	/// Attribute description for attribute database
	struct AttributeDescription {
		/// Constructor
		AttributeDescription() = default;

		/// Constructor
		AttributeDescription(int32_t id_, AtaStorageAttribute::DiskType type, std::string reported_name_,
				std::string displayable_name_, std::string generic_name_, std::string description_)
				: id(id_), disk_type(type), reported_name(std::move(reported_name_)), displayable_name(std::move(displayable_name_)),
				generic_name(std::move(generic_name_)), description(std::move(description_))
		{ }

		int32_t id = -1;  ///< e.g. 190
		AtaStorageAttribute::DiskType disk_type = AtaStorageAttribute::DiskType::Any;  ///< HDD-only, SSD-only or universal attribute
		std::string reported_name;  ///< e.g. Airflow_Temperature_Cel
		std::string displayable_name;  ///< e.g. Airflow Temperature (C). This is a translatable string.
		std::string generic_name;  ///< Generic name to be set on the property, e.g. "airflow_temperature". For lookups.
		std::string description;  ///< Attribute description, can be empty.
	};



	/// Attribute description database
	class AttributeDatabase {
		public:

			/// Constructor
			AttributeDatabase()
			{
				// Note: The first one with the same ID is the one displayed in case smartctl
				// doesn't return a name. See atacmds.cpp (get_default_attr_name()) in smartmontools.
				// The rest are from drivedb.h, which contains overrides.
				// Based on: smartmontools r4430, 2017-05-03.

				// "smartctl" means it's in smartmontools' drivedb.h.
				// "custom" means it's somewhere else.

				// Descriptions are based on:
				// http://en.wikipedia.org/wiki/S.M.A.R.T.
				// http://kb.acronis.com/taxonomy/term/1644
				// http://www.ariolic.com/activesmart/smart-attributes/
				// http://www.ocztechnologyforum.com/staff/ryderocz/misc/Sandforce.jpg
				// Intel Solid-State Drive Toolbox User Guide
				// as well as various other sources.

				// Raw read error rate (smartctl)
				add(1, "Raw_Read_Error_Rate", "Raw Read Error Rate", "",
						"Indicates the rate of read errors that occurred while reading the data. A non-zero Raw value may indicate a problem with either the disk surface or read/write heads. "
						"<i>Note:</i> Some drives (e.g. Seagate) are known to report very high Raw values for this attribute; this is not an indication of a problem.");
				// Throughput Performance (smartctl)
				add(2, "Throughput_Performance", "Throughput Performance", "",
						"Average efficiency of a drive. Reduction of this attribute value can signal various internal problems.");
				// Spin Up Time (smartctl) (some say it can also happen due to bad PSU or power connection (?))
				add(3, "Spin_Up_Time", "Spin-Up Time", "",
						"Average time of spindle spin-up time (from stopped to fully operational). Raw value may show this in milliseconds or seconds. "
						"Changes in spin-up time can reflect problems with the spindle motor or power.");
				// Start/Stop Count (smartctl)
				add(4, "Start_Stop_Count", "Start / Stop Count", "",
						"Number of start/stop cycles of a spindle (Raw value). That is, number of drive spin-ups.");
				// Reallocated Sector Count (smartctl)
				add(5, AtaStorageAttribute::DiskType::Hdd, "Reallocated_Sector_Ct", "Reallocated Sector Count", "attr_reallocated_sector_count",
						"Number of reallocated sectors (Raw value). Non-zero Raw value indicates a disk surface failure."
						"\n\n" + get_uncorrectable_text());
				// SSD: Reallocated Sector Count (smartctl)
				add(5, AtaStorageAttribute::DiskType::Ssd, "Reallocated_Sector_Ct", "Reallocated Sector Count", "attr_reallocated_sector_count",
						"Number of reallocated sectors (Raw value). High Raw value indicates an old age for an SSD.");
				// SandForce SSD: Retired_Block_Count (smartctl)
				add(5, AtaStorageAttribute::DiskType::Ssd, "Retired_Block_Count", "Retired Block Rate", "attr_ssd_life_left",
						"Indicates estimated remaining life of the drive. Normalized value is (100-100*RBC/MRB) where RBC is the number of retired blocks "
						"and MRB is the minimum required blocks.");
				// Crucial/Micron SSD: Reallocate_NAND_Blk_Cnt (smartctl)
				add(5, AtaStorageAttribute::DiskType::Ssd, "Reallocate_NAND_Blk_Cnt", "Reallocated NAND Block Count", "",
						"Number of reallocated blocks (Raw value). High Raw value indicates an old age for an SSD.");
				// Micron SSD: Reallocate_NAND_Blk_Cnt (smartctl)
				add(5, AtaStorageAttribute::DiskType::Ssd, "Reallocated_Block_Count", "Reallocated Block Count", "",
						"Number of reallocated blocks (Raw value). High Raw value indicates an old age for an SSD.");
				// OCZ SSD (smartctl)
				add(5, AtaStorageAttribute::DiskType::Ssd, "Runtime_Bad_Block", "Runtime Bad Block Count", "",
						"");
				// Innodisk SSD (smartctl)
				add(5, AtaStorageAttribute::DiskType::Ssd, "Later_Bad_Block", "Later Bad Block", "",
						"");
				// Read Channel Margin (smartctl)
				add(6, AtaStorageAttribute::DiskType::Hdd, "Read_Channel_Margin", "Read Channel Margin", "",
						"Margin of a channel while reading data. The function of this attribute is not specified.");
				// Seek Error Rate (smartctl)
				add(7, AtaStorageAttribute::DiskType::Hdd, "Seek_Error_Rate", "Seek Error Rate", "",
						"Frequency of errors appearance while positioning. When a drive reads data, it positions heads in the needed place. "
						"If there is a failure in the mechanical positioning system, a seek error arises. More seek errors indicate worse condition "
						"of a disk surface and disk mechanical subsystem. The exact meaning of the Raw value is manufacturer-dependent.");
				// Seek Time Performance (smartctl)
				add(8, AtaStorageAttribute::DiskType::Hdd, "Seek_Time_Performance", "Seek Time Performance", "",
						"Average efficiency of seek operations of the magnetic heads. If this value is decreasing, it is a sign of problems in the hard disk drive mechanical subsystem.");
				// Power-On Hours (smartctl) (Maxtor may use minutes, Fujitsu may use seconds, some even temperature?)
				add(9, "Power_On_Hours", "Power-On Time", "",
						"Number of hours in power-on state. Raw value shows total count of hours (or minutes, or half-minutes, or seconds, depending on manufacturer) in power-on state.");
				// SandForce, Intel SSD: Power_On_Hours_and_Msec (smartctl) (description?)
				add(9, AtaStorageAttribute::DiskType::Ssd, "Power_On_Hours_and_Msec");
				// Smart Storage Systems SSD (smartctl)
				add(9, AtaStorageAttribute::DiskType::Ssd, "Proprietary_9", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Spin-up Retry Count (smartctl)
				add(10, AtaStorageAttribute::DiskType::Hdd, "Spin_Retry_Count", "Spin-Up Retry Count", "attr_spin_up_retry_count",
						"Number of retries of spin start attempts (Raw value). An increase of this attribute value is a sign of problems in the hard disk mechanical subsystem.");
				// Calibration Retry Count (smartctl)
				add(11, AtaStorageAttribute::DiskType::Hdd, "Calibration_Retry_Count", "Calibration Retry Count", "",
						"Number of times recalibration was requested, under the condition that the first attempt was unsuccessful (Raw value). "
								"A decrease is a sign of problems in the hard disk mechanical subsystem.");
				// Power Cycle Count (smartctl)
				add(12, "Power_Cycle_Count", "Power Cycle Count", "",
						"Number of complete power start / stop cycles of a drive.");
				// Soft Read Error Rate (smartctl) (same as 201 ?) (description sounds lame, fix?)
				add(13, "Read_Soft_Error_Rate", "Soft Read Error Rate", "attr_soft_read_error_rate",
						"Uncorrected read errors reported to the operating system (Raw value). If the value is non-zero, you should back up your data.");
				// Sandforce SSD: Soft_Read_Error_Rate (smartctl)
				add(13, AtaStorageAttribute::DiskType::Ssd, "Soft_Read_Error_Rate");
				// Maxtor: Average FHC (custom) (description?)
				add(99, AtaStorageAttribute::DiskType::Hdd, "", "Average FHC (Flying Height Control)", "",
						"");
				// Sandforce SSD: Gigabytes_Erased (smartctl) (description?)
				add(100, AtaStorageAttribute::DiskType::Ssd, "Gigabytes_Erased", "GiB Erased", "",
						"Number of GiB erased.");
				// OCZ SSD (smartctl)
				add(100, AtaStorageAttribute::DiskType::Ssd, "Total_Blocks_Erased", "Total Blocks Erased", "",
						"Number of total blocks erased.");
				// STEC CF: (custom)
				add(100, AtaStorageAttribute::DiskType::Ssd, "", "Erase / Program Cycles", "",  // unused
						"Number of Erase / Program cycles of the entire drive.");
				// Maxtor: Maximum FHC (custom) (description?)
				add(101, AtaStorageAttribute::DiskType::Hdd, "", "Maximum FHC (Flying Height Control)", "",
						"");
				// Unknown (source says Maxtor, but it's an SSD thing and Maxtor doesn't have them at this point).
	// 			add(101, "", "Translation Table Rebuild", "",
	// 					"Indicates power backup fault or internal error resulting in loss of system unit tables.");
				// STEC CF: Translation Table Rebuild (custom)
				add(103, AtaStorageAttribute::DiskType::Ssd, "", "Translation Table Rebuild", "",
						"Indicates power backup fault or internal error resulting in loss of system unit tables.");
				// Smart Storage Systems SSD (smartctl) (description?)
				add(130, AtaStorageAttribute::DiskType::Ssd, "Minimum_Spares_All_Zs", "Minimum Spares All Zs", "",
						"");
				// SiliconMotion SSDs (description?) (smartctl)
				add(148, AtaStorageAttribute::DiskType::Ssd, "Total_SLC_Erase_Ct", "Total SLC Erase Count", "",
						"");
				// SiliconMotion SSDs (description?) (smartctl)
				add(149, AtaStorageAttribute::DiskType::Ssd, "Max_SLC_Erase_Ct", "Maximum SLC Erase Count", "",
						"");
				// SiliconMotion SSDs (description?) (smartctl)
				add(150, AtaStorageAttribute::DiskType::Ssd, "Min_SLC_Erase_Ct", "Minimum SLC Erase Count", "",
						"");
				// SiliconMotion SSDs (description?) (smartctl)
				add(151, AtaStorageAttribute::DiskType::Ssd, "Average_SLC_Erase_Ct", "Average SLC Erase Count", "",
						"");
				// Apacer Flash (description?) (smartctl)
				add(160, AtaStorageAttribute::DiskType::Ssd, "Initial_Bad_Block_Count", "Initial Bad Block Count", "",
						"");
				// Samsung SSD, Intel SSD: Reported Uncorrectable (smartctl)
				add(160, AtaStorageAttribute::DiskType::Ssd, "Uncorrectable_Error_Cnt", "Uncorrectable Error Count", "",
						"");
				// Apacer Flash (description?) (smartctl)
				add(161, AtaStorageAttribute::DiskType::Ssd, "Bad_Block_Count", "Bad Block Count", "",
						"Number of bad blocks. SSDs reallocate blocks as part of their normal operation, so low bad block counts are not critical for them.");
				// Innodisk (description?) (smartctl)
				add(161, AtaStorageAttribute::DiskType::Ssd, "Number_of_Pure_Spare", "Number of Pure Spare", "",
						"");
				// Innodisk CF (description?) (smartctl)
				add(161, AtaStorageAttribute::DiskType::Ssd, "Valid_Spare_Block_Cnt", "Valid Spare Block Count", "",
						"Number of available spare blocks. Spare blocks are used when bad blocks develop.");
				// Apacer Flash (description?) (smartctl)
				add(162, AtaStorageAttribute::DiskType::Ssd, "Spare_Block_Count", "Spare Block Count", "",
						"Number of spare blocks which are used when bad blocks develop.");
				// Innodisk CF (smartctl)
				add(162, AtaStorageAttribute::DiskType::Ssd, "Child_Pair_Count", "Child Pair Count", "",
						"");
				// Apacer Flash (description?) (smartctl)
				add(163, AtaStorageAttribute::DiskType::Ssd, "Max_Erase_Count", "Maximum Erase Count", "",
						"The maximum of individual erase counts of all the blocks.");
				// Innodisk SSD: (smartctl)
				add(163, AtaStorageAttribute::DiskType::Ssd, "Initial_Bad_Block_Count", "Initial Bad Block Count", "",
						"Factory-determined number of initial bad blocks.");
				// Innodisk SSD: (smartctl)
				add(163, AtaStorageAttribute::DiskType::Ssd, "Total_Bad_Block_Count", "Total Bad Block Count", "",
						"Number of bad blocks. SSDs reallocate blocks as part of their normal operation, so low bad block counts are not critical for them.");
				// Apacer Flash (description?) (smartctl)
				add(164, AtaStorageAttribute::DiskType::Ssd, "Average_Erase_Count", "Average Erase Count", "",
						"");
				// Innodisk SSD (description?) (smartctl)
				add(164, AtaStorageAttribute::DiskType::Ssd, "Total_Erase_Count", "Total Erase Count", "",
						"");
				// Apacer Flash (description?) (smartctl)
				add(165, AtaStorageAttribute::DiskType::Ssd, "Average_Erase_Count", "Average Erase Count", "",
						"");
				// Innodisk SSD (description?) (smartctl)
				add(165, AtaStorageAttribute::DiskType::Ssd, "Max_Erase_Count", "Maximum Erase Count", "",
						"");
				// Sandisk SSD (description?) (smartctl)
				add(165, AtaStorageAttribute::DiskType::Ssd, "Total_Write/Erase_Count", "Total Write / Erase Count", "",
						"");
				// Apacer Flash (description?) (smartctl)
				add(166, AtaStorageAttribute::DiskType::Ssd, "Later_Bad_Block_Count", "Later Bad Block Count", "",
						"");
				// Innodisk SSD (description?) (smartctl)
				add(166, AtaStorageAttribute::DiskType::Ssd, "Min_Erase_Count", "Minimum Erase Count", "",
						"");
				// Sandisk SSD (description?) (smartctl)
				add(166, AtaStorageAttribute::DiskType::Ssd, "Min_W/E_Cycle", "Minimum Write / Erase Cycles", "",
						"");
				// Apacer Flash, OCZ (description?) (smartctl)
				add(167, AtaStorageAttribute::DiskType::Ssd, "SSD_Protect_Mode", "SSD Protect Mode", "",
						"");
				// Innodisk SSD (description?) (smartctl)
				add(167, AtaStorageAttribute::DiskType::Ssd, "Average_Erase_Count", "Average Erase Count", "",
						"");
				// Sandisk SSD (description?) (smartctl)
				add(167, AtaStorageAttribute::DiskType::Ssd, "Min_Bad_Block/Die", "Minimum Bad Block / Die", "",
						"");
				// Apacer Flash (description?) (smartctl)
				add(168, AtaStorageAttribute::DiskType::Ssd, "SATA_PHY_Err_Ct", "SATA Physical Error Count", "",
						"");
				// Various SSDs: (smartctl) (description?)
				add(168, AtaStorageAttribute::DiskType::Ssd, "SATA_Phy_Error_Count", "SATA Physical Error Count", "",
						"");
				// Innodisk SSDs: (smartctl) (description?)
				add(168, AtaStorageAttribute::DiskType::Ssd, "Max_Erase_Count_of_Spec", "Maximum Erase Count per Specification", "",
						"");
				// Sandisk SSD (description?) (smartctl)
				add(168, AtaStorageAttribute::DiskType::Ssd, "Maximum_Erase_Cycle", "Maximum Erase Cycles", "",
						"");
				// Toshiba SSDs: (smartctl) (description?)
				add(169, AtaStorageAttribute::DiskType::Ssd, "Bad_Block_Count", "Bad Block Count", "",
						"Number of bad blocks. SSDs reallocate blocks as part of their normal operation, so low bad block counts are not critical for them.");
				// Sandisk SSD (description?) (smartctl)
				add(169, AtaStorageAttribute::DiskType::Ssd, "Total_Bad_Blocks", "Total Bad Blocks", "",
						"Number of bad blocks. SSDs reallocate blocks as part of their normal operation, so low bad block counts are not critical for them.");
				// Innodisk SSDs: (smartctl) (description?)
				add(169, AtaStorageAttribute::DiskType::Ssd, "Remaining_Lifetime_Perc", "Remaining Lifetime %", "attr_ssd_life_left",
						"Remaining drive life in % (usually by erase count).");
				// Intel SSD, STEC CF: Reserved Block Count (smartctl)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Reserve_Block_Count", "Reserved Block Count", "",
						"Number of reserved (spare) blocks for bad block handling.");
				// Micron SSD: Reserved Block Count (smartctl)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Reserved_Block_Count", "Reserved Block Count", "",
						"Number of reserved (spare) blocks for bad block handling.");
				// Crucial / Marvell SSD: Grown Failing Block Count (smartctl) (description?)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Grown_Failing_Block_Ct", "Grown Failing Block Count", "",
						"");
				// Intel SSD: (smartctl) (description?)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Available_Reservd_Space", "Available Reserved Space", "",
						"");
				// Various SSDs: (smartctl) (description?)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Bad_Block_Count", "Bad Block Count", "",
						"Number of bad blocks. SSDs reallocate blocks as part of their normal operation, so low bad block counts are not critical for them.");
				// Kingston SSDs: (smartctl) (description?)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Bad_Blk_Ct_Erl/Lat", "Bad Block Early / Later", "",
						"");
				// Samsung SSDs: (smartctl) (description?)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Unused_Rsvd_Blk_Ct_Chip", "Unused Reserved Block Count (Chip)", "",
						"");
				// Innodisk Flash (description?) (smartctl)
				add(170, AtaStorageAttribute::DiskType::Ssd, "Spare_Block_Count", "Spare Block Count", "",
						"Number of spare blocks which are used in case bad blocks develop.");
				// Intel SSD, Sandforce SSD, STEC CF, Crucial / Marvell SSD: Program Fail Count (smartctl)
				add(171, AtaStorageAttribute::DiskType::Ssd, "Program_Fail_Count", "Program Fail Count", "",
						"Number of flash program (write) failures. High values may indicate old drive age or other problems.");
				// Samsung SSDs: (smartctl) (description?)
				add(171, AtaStorageAttribute::DiskType::Ssd, "Program_Fail_Count_Chip", "Program Fail Count (Chip)", "",
						"");
				// OCZ SSD (smartctl)
				add(171, AtaStorageAttribute::DiskType::Ssd, "Avail_OP_Block_Count", "Available OP Block Count", "",
						"");
				// Intel SSD, Sandforce SSD, STEC CF, Crucial / Marvell SSD: Erase Fail Count (smartctl)
				add(172, AtaStorageAttribute::DiskType::Ssd, "Erase_Fail_Count", "Erase Fail Count", "",
						"Number of flash erase command failures. High values may indicate old drive age or other problems.");
				// Various SSDs (smartctl) (description?)
				add(173, AtaStorageAttribute::DiskType::Ssd, "Erase_Count", "Erase Count", "",
						"");
				// Samsung SSDs (smartctl) (description?)
				add(173, AtaStorageAttribute::DiskType::Ssd, "Erase_Fail_Count_Chip", "Erase Fail Count (Chip)", "",
						"");
				// Kingston SSDs (smartctl) (description?)
				add(173, AtaStorageAttribute::DiskType::Ssd, "MaxAvgErase_Ct", "Maximum / Average Erase Count", "",
						"");
				// Crucial/Micron SSDs (smartctl) (description?)
				add(173, AtaStorageAttribute::DiskType::Ssd, "Ave_Block-Erase_Count", "Average Block-Erase Count", "",
						"");
				// STEC CF, Crucial / Marvell SSD: Wear Leveling Count (smartctl) (description?)
				add(173, AtaStorageAttribute::DiskType::Ssd, "Wear_Leveling_Count", "Wear Leveling Count", "",
						"Indicates the difference between the most worn block and the least worn block.");
				// Same as above, old smartctl
				add(173, AtaStorageAttribute::DiskType::Ssd, "Wear_Levelling_Count", "Wear Leveling Count", "",
						"Indicates the difference between the most worn block and the least worn block.");
				// Sandisk SSDs (smartctl) (description?)
				add(173, AtaStorageAttribute::DiskType::Ssd, "Avg_Write/Erase_Count", "Average Write / Erase Count", "",
						"");
				// Intel SSD, Sandforce SSD, Crucial / Marvell SSD: Unexpected Power Loss (smartctl)
				add(174, AtaStorageAttribute::DiskType::Ssd, "Unexpect_Power_Loss_Ct", "Unexpected Power Loss Count", "",
						"Number of unexpected power loss events.");
				// OCZ SSD (smartctl)
				add(174, AtaStorageAttribute::DiskType::Ssd, "Pwr_Cycle_Ct_Unplanned", "Unexpected Power Loss Count", "",
						"Number of unexpected power loss events.");
				// Apple SSD (smartctl)
				add(174, AtaStorageAttribute::DiskType::Ssd, "Host_Reads_MiB", "Host Read (MiB)", "",
						"Total number of sectors read by the host system. The Raw value is increased by 1 for every MiB read by the host.");
				// Program_Fail_Count_Chip (smartctl)
				add(175, AtaStorageAttribute::DiskType::Ssd, "Program_Fail_Count_Chip", "Program Fail Count (Chip)", "",
						"Number of flash program (write) failures. High values may indicate old drive age or other problems.");
				// Various SSDs: Bad_Cluster_Table_Count (smartctl) (description?)
				add(175, AtaStorageAttribute::DiskType::Ssd, "Bad_Cluster_Table_Count", "Bad Cluster Table Count", "",
						"");
				// Intel SSD (smartctl) (description?)
				add(175, AtaStorageAttribute::DiskType::Ssd, "Power_Loss_Cap_Test", "Power Loss Capacitor Test", "",
						"");
				// Intel SSD (smartctl) (description?)
				add(175, AtaStorageAttribute::DiskType::Ssd, "Host_Writes_MiB", "Host Written (MiB)", "",
						"Total number of sectors written by the host system. The Raw value is increased by 1 for every MiB written by the host.");
				// Erase_Fail_Count_Chip (smartctl)
				add(176, AtaStorageAttribute::DiskType::Ssd, "Erase_Fail_Count_Chip", "Erase Fail Count (Chip)", "",
						"Number of flash erase command failures. High values may indicate old drive age or other problems.");
				// Innodisk SSD (smartctl) (description?)
				add(176, AtaStorageAttribute::DiskType::Ssd, "Uncorr_RECORD_Count", "Uncorrected RECORD Count", "",
						"");
				// Innodisk SSD (smartctl) (description?)
				add(176, AtaStorageAttribute::DiskType::Ssd, "RANGE_RECORD_Count", "RANGE RECORD Count", "",
						"");
				// Wear_Leveling_Count (smartctl) (same as Wear_Range_Delta?)
				add(177, AtaStorageAttribute::DiskType::Ssd, "Wear_Leveling_Count", "Wear Leveling Count", "",
						"Indicates the difference (in percent) between the most worn block and the least worn block.");
				// Sandforce SSD: Wear_Range_Delta (smartctl)
				add(177, AtaStorageAttribute::DiskType::Ssd, "Wear_Range_Delta", "Wear Range Delta", "",
						"Indicates the difference (in percent) between the most worn block and the least worn block.");
				// Used_Rsvd_Blk_Cnt_Chip (smartctl)
				add(178, AtaStorageAttribute::DiskType::Ssd, "Used_Rsvd_Blk_Cnt_Chip", "Used Reserved Block Count (Chip)", "",
						"Number of a chip's used reserved blocks. High values may indicate old drive age or other problems.");
				// Innodisk SSD (smartctl)
				add(178, AtaStorageAttribute::DiskType::Ssd, "Runtime_Invalid_Blk_Cnt", "Runtime Invalid Block Count", "",
						"");
				// Used_Rsvd_Blk_Cnt_Tot (smartctl) (description?)
				add(179, AtaStorageAttribute::DiskType::Ssd, "Used_Rsvd_Blk_Cnt_Tot", "Used Reserved Block Count (Total)", "",
						"Number of used reserved blocks. High values may indicate old drive age or other problems.");
				// Unused_Rsvd_Blk_Cnt_Tot (smartctl)
				add(180, AtaStorageAttribute::DiskType::Ssd, "Unused_Rsvd_Blk_Cnt_Tot", "Unused Reserved Block Count (Total)", "",
						"Number of unused reserved blocks. High values may indicate old drive age or other problems.");
				// Crucial / Micron SSDs (smartctl) (description?)
				add(180, AtaStorageAttribute::DiskType::Ssd, "Unused_Reserve_NAND_Blk", "Unused Reserved NAND Blocks", "",
						"");
				// Program_Fail_Cnt_Total (smartctl)
				add(181, "Program_Fail_Cnt_Total", "Program Fail Count", "",
						"Number of flash program (write) failures. High values may indicate old drive age or other problems.");
				// Sandforce SSD: Program_Fail_Count (smartctl) (Sandforce says it's identical to 171)
				add(181, AtaStorageAttribute::DiskType::Ssd, "Program_Fail_Count");
				// Crucial / Marvell SSD (smartctl) (description?)
				add(181, AtaStorageAttribute::DiskType::Ssd, "Non4k_Aligned_Access", "Non-4k Aligned Access", "",
						"");
				// Erase_Fail_Count_Total (smartctl) (description?)
				add(182, AtaStorageAttribute::DiskType::Ssd, "Erase_Fail_Count_Total", "Erase Fail Count", "",
						"Number of flash erase command failures. High values may indicate old drive age or other problems.");
				// Sandforce SSD: Erase_Fail_Count (smartctl) (Sandforce says it's identical to 172)
				add(182, AtaStorageAttribute::DiskType::Ssd, "Erase_Fail_Count");
				// Runtime_Bad_Block (smartctl) (description?)
				add(183, "Runtime_Bad_Block", "Runtime Bad Blocks", "",
						"");
				// Samsung, WD, Crucial / Marvell SSD: SATA Downshift Error Count (smartctl) (description?)
				add(183, AtaStorageAttribute::DiskType::Any, "SATA_Iface_Downshift", "SATA Downshift Error Count", "",
						"");
				// Crucial / Marvell SSD: SATA Downshift Error Count (smartctl) (description?)
				add(183, AtaStorageAttribute::DiskType::Any, "SATA_Interfac_Downshift", "SATA Downshift Error Count", "",
						"");
				// Intel SSD, Ubtek SSD (smartctl) (description?)
				add(183, AtaStorageAttribute::DiskType::Ssd, "SATA_Downshift_Count", "SATA Downshift Error Count", "",
						"");
				// End to End Error (smartctl) (description?)
				add(184, "End-to-End_Error", "End to End Error", "",
						"Indicates discrepancy of data between the host and the drive cache.");
				// Sandforce SSD: IO_Error_Detect_Code_Ct (smartctl)
				add(184, AtaStorageAttribute::DiskType::Ssd, "IO_Error_Detect_Code_Ct", "Input/Output ECC Error Count", "",
						"");
				// OCZ SSD (smartctl)
				add(184, AtaStorageAttribute::DiskType::Ssd, "Factory_Bad_Block_Count", "Factory Bad Block Count", "",
						"");
				// Indilinx Barefoot SSD: IO_Error_Detect_Code_Ct (smartctl)
				add(184, AtaStorageAttribute::DiskType::Ssd, "Initial_Bad_Block_Count", "Initial Bad Block Count", "",
						"Factory-determined number of initial bad blocks.");
				// Crucial / Micron SSD (smartctl)
				add(184, AtaStorageAttribute::DiskType::Ssd, "Error_Correction_Count", "Error Correction Count", "",
						"");
				// WD: Head Stability (custom)
				add(185, AtaStorageAttribute::DiskType::Hdd, "", "Head Stability", "",
						"");
				// WD: Induced Op-Vibration Detection (custom)
				add(185, AtaStorageAttribute::DiskType::Hdd, "", "Induced Op-Vibration Detection", "",  // unused
						"");
				// Reported Uncorrectable (smartctl)
				add(187, "Reported_Uncorrect", "Reported Uncorrectable", "",
						"Number of errors that could not be recovered using hardware ECC (Error-Correcting Code).");
				// Innodisk SSD: Reported Uncorrectable (smartctl)
				add(187, AtaStorageAttribute::DiskType::Ssd, "Uncorrectable_Error_Cnt");
				// OCZ SSD (smartctl)
				add(187, AtaStorageAttribute::DiskType::Ssd, "Total_Unc_NAND_Reads", "Total Uncorrectable NAND Reads", "",
						"");
				// Command Timeout (smartctl)
				add(188, "Command_Timeout", "Command Timeout", "",
						"Number of aborted operations due to drive timeout. High values may indicate problems with cabling or power supply.");
				// Micron SSD (smartctl)
				add(188, AtaStorageAttribute::DiskType::Ssd, "Command_Timeouts", "Command Timeout", "",
						"Number of aborted operations due to drive timeout. High values may indicate problems with cabling or power supply.");
				// High Fly Writes (smartctl)
				add(189, AtaStorageAttribute::DiskType::Hdd, "High_Fly_Writes", "High Fly Writes", "",
						"Some drives can detect when a recording head is flying outside its normal operating range. "
						"If an unsafe fly height condition is encountered, the write process is stopped, and the information "
						"is rewritten or reallocated to a safe region of the drive. This attribute indicates the count of "
						"these errors detected over the lifetime of the drive.");
				// Crucial / Marvell SSD (smartctl)
				add(189, AtaStorageAttribute::DiskType::Ssd, "Factory_Bad_Block_Ct", "Factory Bad Block Count", "",
						"Factory-determined number of initial bad blocks.");
				// Various SSD (smartctl)
				add(189, "Airflow_Temperature_Cel", "Airflow Temperature", "",
						"Indicates temperature (in Celsius), 100 - temperature, or something completely different (highly depends on manufacturer and model).");
				// Airflow Temperature (smartctl) (WD Caviar (may be 50 less), Samsung). Temperature or (100 - temp.) on Seagate/Maxtor.
				add(190, "Airflow_Temperature_Cel", "Airflow Temperature", "",
						"Indicates temperature (in Celsius), 100 - temperature, or something completely different (highly depends on manufacturer and model).");
				// Samsung SSD (smartctl) (description?)
				add(190, "Temperature_Exceed_Cnt", "Temperature Exceed Count", "",
						"");
				// OCZ SSD (smartctl)
				add(190, "Temperature_Celsius", "Temperature (Celsius)", "attr_temperature_celsius",
						"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius).");
				// Intel SSD
				add(190, "Temperature_Case", "Case Temperature (Celsius)", "",
						"Drive case temperature. The Raw value shows built-in heat sensor registrations (in Celsius).");
				// G-sense error rate (smartctl) (same as 221?)
				add(191, AtaStorageAttribute::DiskType::Hdd, "G-Sense_Error_Rate", "G-Sense Error Rate", "",
						"Number of errors caused by externally-induced shock and vibration (Raw value). May indicate incorrect installation.");
				// Power-Off Retract Cycle (smartctl)
				add(192, AtaStorageAttribute::DiskType::Hdd, "Power-Off_Retract_Count", "Head Retract Cycle Count", "",
						"Number of times the heads were loaded off the media (during power-offs or emergency conditions).");
				// Intel SSD: Unsafe_Shutdown_Count (smartctl)
				add(192, AtaStorageAttribute::DiskType::Ssd, "Unsafe_Shutdown_Count", "Unsafe Shutdown Count", "",
						"Raw value indicates the number of unsafe (unclean) shutdown events over the drive lifetime. "
						"An unsafe shutdown occurs whenever the device is powered off without "
						"STANDBY IMMEDIATE being the last command.");
				// Various SSDs (smartctl)
				add(192, AtaStorageAttribute::DiskType::Ssd, "Unexpect_Power_Loss_Ct", "Unexpected Power Loss Count", "",
						"Number of unexpected power loss events.");
				// Fujitsu: Emergency Retract Cycle Count (smartctl)
				add(192, AtaStorageAttribute::DiskType::Hdd, "Emerg_Retract_Cycle_Ct", "Emergency Retract Cycle Count", "",
						"Number of times the heads were loaded off the media during emergency conditions.");
				// Load/Unload Cycle (smartctl)
				add(193, AtaStorageAttribute::DiskType::Hdd, "Load_Cycle_Count", "Load / Unload Cycle", "",
						"Number of load / unload cycles into Landing Zone position.");
				// Temperature Celsius (smartctl) (same as 231). This is the most common one. Some Samsungs: 10xTemp.
				add(194, "Temperature_Celsius", "Temperature (Celsius)", "attr_temperature_celsius",
						"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius). "
						"Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).");
				// Samsung SSD: Temperature Celsius (smartctl) (not sure about the value)
				add(194, AtaStorageAttribute::DiskType::Ssd, "Airflow_Temperature", "Airflow Temperature (Celsius)", "attr_temperature_celsius",
						"Drive temperature (Celsius)");
				// Temperature Celsius x 10 (smartctl)
				add(194, "Temperature_Celsius_x10", "Temperature (Celsius) x 10", "attr_temperature_celsius_x10",
						"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius * 10). "
						"Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).");
				// Smart Storage Systems SSD (smartctl)
				add(194, AtaStorageAttribute::DiskType::Ssd, "Proprietary_194", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Intel SSD (smartctl)
				add(194, "Temperature_Internal", "Internal Temperature (Celsius)", "attr_temperature_celsius",
						"Drive case temperature. The Raw value shows built-in heat sensor registrations (in Celsius).");
				// Hardware ECC Recovered (smartctl)
				add(195, "Hardware_ECC_Recovered", "Hardware ECC Recovered", "",
						"Number of ECC on the fly errors (Raw value). Users are advised to ignore this attribute.");
				// Fujitsu: ECC_On_The_Fly_Count (smartctl)
				add(195, AtaStorageAttribute::DiskType::Hdd, "ECC_On_The_Fly_Count");
				// Sandforce SSD: ECC_Uncorr_Error_Count (smartctl) (description?)
				add(195, AtaStorageAttribute::DiskType::Ssd, "ECC_Uncorr_Error_Count", "Uncorrected ECC Error Count", "",
						"Number of uncorrectable errors (UECC).");
				// Samsung SSD (smartctl) (description?)
				add(195, AtaStorageAttribute::DiskType::Ssd, "ECC_Rate", "Uncorrected ECC Error Rate", "",
						"");
				// OCZ SSD (smartctl)
				add(195, AtaStorageAttribute::DiskType::Ssd, "Total_Prog_Failures", "Total Program Failures", "",
						"");
				// Indilinx Barefoot SSD: Program_Failure_Blk_Ct (smartctl) (description?)
				add(195, AtaStorageAttribute::DiskType::Ssd, "Program_Failure_Blk_Ct", "Program Failure Block Count", "",
						"Number of flash program (write) failures.");
				// Micron SSD (smartctl)
				add(195, AtaStorageAttribute::DiskType::Ssd, "Cumulativ_Corrected_ECC", "Cumulative Corrected ECC Error Count", "",
						"");
				// Reallocation Event Count (smartctl)
				add(196, AtaStorageAttribute::DiskType::Any, "Reallocated_Event_Count", "Reallocation Event Count", "attr_reallocation_event_count",
						"Number of reallocation (remap) operations. Raw value <i>should</i> show the total number of attempts "
						"(both successful and unsuccessful) to reallocate sectors. An increase in Raw value indicates a disk surface failure."
						"\n\n" + get_uncorrectable_text());
				// Indilinx Barefoot SSD: Erase_Failure_Blk_Ct (smartctl) (description?)
				add(196, AtaStorageAttribute::DiskType::Ssd, "Erase_Failure_Blk_Ct", "Erase Failure Block Count", "",
						"Number of flash erase failures.");
				// OCZ SSD (smartctl)
				add(196, AtaStorageAttribute::DiskType::Ssd, "Total_Erase_Failures", "Total Erase Failures", "",
						"");
				// Current Pending Sector Count (smartctl)
				add(197, "Current_Pending_Sector", "Current Pending Sector Count", "attr_current_pending_sector_count",
						"Number of &quot;unstable&quot; (waiting to be remapped) sectors (Raw value). "
						"If the unstable sector is subsequently read from or written to successfully, this value is decreased and the sector is not remapped. "
						"An increase in Raw value indicates a disk surface failure."
						"\n\n" + get_uncorrectable_text());
				// Indilinx Barefoot SSD: Read_Failure_Blk_Ct (smartctl) (description?)
				add(197, AtaStorageAttribute::DiskType::Ssd, "Read_Failure_Blk_Ct", "Read Failure Block Count", "",
						"Number of blocks that failed to be read.");
				// Samsung: Total_Pending_Sectors (smartctl). From smartctl man page:
				// unlike Current_Pending_Sector, this won't decrease on reallocation.
				add(197, "Total_Pending_Sectors", "Total Pending Sectors", "attr_total_pending_sectors",
						"Number of &quot;unstable&quot; (waiting to be remapped) sectors and already remapped sectors (Raw value). "
						"An increase in Raw value indicates a disk surface failure."
						"\n\n" + get_uncorrectable_text());
				// OCZ SSD (smartctl)
				add(197, AtaStorageAttribute::DiskType::Ssd, "Total_Unc_Read_Failures", "Total Uncorrectable Read Failures", "",
						"");
				// Offline Uncorrectable (smartctl)
				add(198, "Offline_Uncorrectable", "Offline Uncorrectable", "attr_offline_uncorrectable",
						"Number of sectors which couldn't be corrected during Offline Data Collection (Raw value). "
						"An increase in Raw value indicates a disk surface failure. "
						"The value may be decreased automatically when the errors are corrected (e.g., when an unreadable sector is "
						"reallocated and the next Offline test is run to see the change)."
						"\n\n" + get_uncorrectable_text());
				// Samsung: Offline Uncorrectable (smartctl). From smartctl man page:
				// unlike Current_Pending_Sector, this won't decrease on reallocation.
				add(198, "Total_Offl_Uncorrectabl", "Total Offline Uncorrectable", "attr_total_attr_offline_uncorrectable",
						"Number of sectors which couldn't be corrected during Offline Data Collection (Raw value), currently and in the past. "
						"An increase in Raw value indicates a disk surface failure."
						"\n\n" + get_uncorrectable_text());
				// Sandforce SSD: Uncorrectable_Sector_Ct (smartctl) (same description?)
				add(198, AtaStorageAttribute::DiskType::Ssd, "Uncorrectable_Sector_Ct");
				// Indilinx Barefoot SSD: Read_Sectors_Tot_Ct (smartctl) (description?)
				add(198, AtaStorageAttribute::DiskType::Ssd, "Read_Sectors_Tot_Ct", "Total Read Sectors", "",
						"Total count of read sectors.");
				// OCZ SSD
				add(198, AtaStorageAttribute::DiskType::Ssd, "Host_Reads_GiB", "Host Read (GiB)", "",
						"Total number of sectors read by the host system. The Raw value is increased by 1 for every GiB read by the host.");
				// Fujitsu: Offline_Scan_UNC_SectCt (smartctl)
				add(198, AtaStorageAttribute::DiskType::Hdd, "Offline_Scan_UNC_SectCt");
				// Fujitsu version of Offline Uncorrectable (smartctl) (old, not in current smartctl)
				add(198, AtaStorageAttribute::DiskType::Hdd, "Off-line_Scan_UNC_Sector_Ct");
				// UDMA CRC Error Count (smartctl)
				add(199, "UDMA_CRC_Error_Count", "UDMA CRC Error Count", "",
						"Number of errors in data transfer via the interface cable in UDMA mode, as determined by ICRC "
						"(Interface Cyclic Redundancy Check) (Raw value).");
				// Sandforce SSD: SATA_CRC_Error_Count (smartctl) (description?)
				add(199, "SATA_CRC_Error_Count", "SATA CRC Error Count", "",
						"Number of errors in data transfer via the SATA interface cable (Raw value).");
				// Sandisk SSD: SATA_CRC_Error_Count (smartctl) (description?)
				add(199, "SATA_CRC_Error", "SATA CRC Error Count", "",
						"Number of errors in data transfer via the SATA interface cable (Raw value).");
				// Intel SSD, Samsung SSD (smartctl) (description?)
				add(199, "CRC_Error_Count", "CRC Error Count", "",
						"Number of errors in data transfer via the interface cable (Raw value).");
				// Indilinx Barefoot SSD: Write_Sectors_Tot_Ct (smartctl) (description?)
				add(199, AtaStorageAttribute::DiskType::Ssd, "Write_Sectors_Tot_Ct", "Total Written Sectors", "",
						"Total count of written sectors.");
				// OCZ SSD
				add(198, AtaStorageAttribute::DiskType::Ssd, "Host_Writes_GiB", "Host Written (GiB)", "",
						"Total number of sectors written by the host system. The Raw value is increased by 1 for every GiB written by the host.");
				// WD: Multi-Zone Error Rate (smartctl). (maybe head flying height too (?))
				add(200, AtaStorageAttribute::DiskType::Hdd, "Multi_Zone_Error_Rate", "Multi Zone Error Rate", "",
						"Number of errors found when writing to sectors (Raw value). The higher the value, the worse the disk surface condition and/or mechanical subsystem is.");
				// Fujitsu: Write Error Rate (smartctl)
				add(200, AtaStorageAttribute::DiskType::Hdd, "Write_Error_Count", "Write Error Count", "",
                        "Number of errors found when writing to sectors (Raw value). The higher the value, the worse the disk surface condition and/or mechanical subsystem is.");
				// Indilinx Barefoot SSD: Read_Commands_Tot_Ct (smartctl) (description?)
				add(200, AtaStorageAttribute::DiskType::Ssd, "Read_Commands_Tot_Ct", "Total Read Commands Issued", "",
						"Total count of read commands issued.");
				// Soft Read Error Rate (smartctl) (description?)
				add(201, AtaStorageAttribute::DiskType::Hdd, "Soft_Read_Error_Rate", "Soft Read Error Rate", "attr_soft_read_error_rate",
						"Uncorrected read errors reported to the operating system (Raw value). If the value is non-zero, you should back up your data.");
				// Sandforce SSD: Unc_Soft_Read_Err_Rate (smartctl)
				add(201, AtaStorageAttribute::DiskType::Ssd, "Unc_Soft_Read_Err_Rate");
				// Samsung SSD: (smartctl) (description?)
				add(201, AtaStorageAttribute::DiskType::Ssd, "Supercap_Status", "Supercapacitor Health", "",
						"");
				// Maxtor: Off Track Errors (custom)
// 				add(201, AtaStorageAttribute::DiskType::Hdd, "", "Off Track Errors", "",  // unused
// 						"");
				// Fujitsu: Detected TA Count (smartctl) (description?)
				add(201, AtaStorageAttribute::DiskType::Hdd, "Detected_TA_Count", "Torque Amplification Count", "",
						"Number of attempts to compensate for platter speed variations.");
				// Indilinx Barefoot SSD: Write_Commands_Tot_Ct (smartctl) (description?)
				add(201, AtaStorageAttribute::DiskType::Ssd, "Write_Commands_Tot_Ct", "Total Write Commands Issued", "",
						"Total count of write commands issued.");
				// WD: Data Address Mark Errors (smartctl)
				add(202, AtaStorageAttribute::DiskType::Hdd, "Data_Address_Mark_Errs", "Data Address Mark Errors", "",
						"Frequency of the Data Address Mark errors.");
				// Fujitsu: TA Increase Count (same as 227?)
				add(202, AtaStorageAttribute::DiskType::Hdd, "TA_Increase_Count", "TA Increase Count", "",
						"Number of attempts to compensate for platter speed variations.");
				// Indilinx Barefoot SSD: Error_Bits_Flash_Tot_Ct (smartctl) (description?)
				add(202, AtaStorageAttribute::DiskType::Ssd, "Error_Bits_Flash_Tot_Ct", "Total Count of Error Bits", "",
						"");
				// Crucial / Marvell SSD: Percent_Lifetime_Used (smartctl) (description?)
				add(202, AtaStorageAttribute::DiskType::Ssd, "Percent_Lifetime_Used", "Rated Life Used (%)", "attr_ssd_life_used",
						"Used drive life in %.");
				// Samsung SSD: (smartctl) (description?)
				add(202, AtaStorageAttribute::DiskType::Ssd, "Exception_Mode_Status", "Exception Mode Status", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(202, AtaStorageAttribute::DiskType::Ssd, "Total_Read_Bits_Corr_Ct", "Total Read Bits Corrected", "",
						"");
				// Micron SSD (smartctl) (description?)
				add(202, AtaStorageAttribute::DiskType::Ssd, "Percent_Lifetime_Remain", "Remaining Lifetime (%)", "attr_ssd_life_left",
						"Remaining drive life in %.");
				// Run Out Cancel (smartctl). (description?)
				add(203, "Run_Out_Cancel", "Run Out Cancel", "",
						"Number of ECC errors.");
				// Maxtor: ECC Errors (smartctl) (description?)
				add(203, AtaStorageAttribute::DiskType::Hdd, "Corr_Read_Errors_Tot_Ct", "ECC Errors", "",
						"Number of ECC errors.");
				// Indilinx Barefoot SSD: Corr_Read_Errors_Tot_Ct (smartctl) (description?)
				add(203, AtaStorageAttribute::DiskType::Ssd, "Corr_Read_Errors_Tot_Ct", "Total Corrected Read Errors", "",
						"Total cound of read sectors with correctable errors.");
				// Maxtor: Soft ECC Correction (smartctl)
				add(204, AtaStorageAttribute::DiskType::Hdd, "Soft_ECC_Correction", "Soft ECC Correction", "",
						"Number of errors corrected by software ECC (Error-Correcting Code).");
				// Fujitsu: Shock_Count_Write_Opern (smartctl) (description?)
				add(204, AtaStorageAttribute::DiskType::Hdd, "Shock_Count_Write_Opern", "Shock Count During Write Operation", "",
						"");
				// Sandforce SSD: Soft_ECC_Correct_Rate (smartctl) (description?)
				add(204, AtaStorageAttribute::DiskType::Ssd, "Soft_ECC_Correct_Rate", "Soft ECC Correction Rate", "",
						"");
				// Indilinx Barefoot SSD: Bad_Block_Full_Flag (smartctl) (description?)
				add(204, AtaStorageAttribute::DiskType::Ssd, "Bad_Block_Full_Flag", "Bad Block Area Is Full", "",
						"Indicates whether the bad block (reserved) area is full or not.");
				// Thermal Asperity Rate (TAR) (smartctl)
				add(205, "Thermal_Asperity_Rate", "Thermal Asperity Rate", "",
						"Number of problems caused by high temperature.");
				// Fujitsu: Shock_Rate_Write_Opern (smartctl) (description?)
				add(205, AtaStorageAttribute::DiskType::Hdd, "Shock_Rate_Write_Opern", "Shock Rate During Write Operation", "",
						"");
				// Indilinx Barefoot SSD: Max_PE_Count_Spec (smartctl) (description?)
				add(205, AtaStorageAttribute::DiskType::Ssd, "Max_PE_Count_Spec", "Maximum Program-Erase Count Specification", "",
						"Maximum Program / Erase cycle count as per specification.");
				// OCZ SSD (smartctl)
				add(205, AtaStorageAttribute::DiskType::Ssd, "Max_Rated_PE_Count", "Maximum Rated Program-Erase Count", "",
						"Maximum Program / Erase cycle count as per specification.");
				// Flying Height (smartctl)
				add(206, AtaStorageAttribute::DiskType::Hdd, "Flying_Height", "Head Flying Height", "",
						"The height of the disk heads above the disk surface. A downward trend will often predict a head crash, "
						"while high values may cause read / write errors.");
				// Indilinx Barefoot SSD, OCZ SSD: Min_Erase_Count (smartctl) (description?)
				add(206, AtaStorageAttribute::DiskType::Ssd, "Min_Erase_Count", "Minimum Erase Count", "",
						"The minimum of individual erase counts of all the blocks.");
				// Crucial / Marvell SSD: Write_Error_Rate (smartctl) (description?)
				add(206, AtaStorageAttribute::DiskType::Ssd, "Write_Error_Rate", "Write Error Rate", "",
						"");
				// Spin High Current (smartctl)
				add(207, AtaStorageAttribute::DiskType::Hdd, "Spin_High_Current", "Spin High Current", "",
						"Amount of high current needed or used to spin up the drive.");
				// Indilinx Barefoot SSD, OCZ SSD: Max_Erase_Count (smartctl) (description?)
				add(207, AtaStorageAttribute::DiskType::Ssd, "Max_Erase_Count", "Maximum Erase Count", "",
						"");
				// Spin Buzz (smartctl)
				add(208, AtaStorageAttribute::DiskType::Hdd, "Spin_Buzz", "Spin Buzz", "",
						"Number of buzz routines (retries because of low current) to spin up the drive.");
				// Indilinx Barefoot SSD, OCZ SSD: Average_Erase_Count (smartctl) (description?)
				add(208, AtaStorageAttribute::DiskType::Ssd, "Average_Erase_Count", "Average Erase Count", "",
						"The average of individual erase counts of all the blocks.");
				// Offline Seek Performance (smartctl) (description?)
				add(209, AtaStorageAttribute::DiskType::Hdd, "Offline_Seek_Performnce", "Offline Seek Performance", "",
						"Seek performance during Offline Data Collection operations.");
				// Indilinx Barefoot SSD, OCZ SSD: Remaining_Lifetime_Perc (smartctl) (description?)
				add(209, AtaStorageAttribute::DiskType::Ssd, "Remaining_Lifetime_Perc", "Remaining Lifetime (%)", "attr_ssd_life_left",
						"Remaining drive life in % (usually by erase count).");
				// Vibration During Write (custom). wikipedia says 211, but it's wrong. (description?)
				add(210, AtaStorageAttribute::DiskType::Hdd, "", "Vibration During Write", "",
						"Vibration encountered during write operations.");
				// OCZ SSD (smartctl)
				add(210, AtaStorageAttribute::DiskType::Ssd, "SATA_CRC_Error_Count", "SATA CRC Error Count", "",
						"");
				// Indilinx Barefoot SSD: Indilinx_Internal (smartctl) (description?)
				add(210, AtaStorageAttribute::DiskType::Ssd, "Indilinx_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Crucial / Micron SSD (smartctl)
				add(210, AtaStorageAttribute::DiskType::Ssd, "Success_RAIN_Recov_Cnt", "Success RAIN Recovered Count", "",
						"");
				// Vibration During Read (description?)
				add(211, AtaStorageAttribute::DiskType::Hdd, "", "Vibration During Read", "",
						"Vibration encountered during read operations.");
				// Indilinx Barefoot SSD (smartctl) (description?)
				add(211, AtaStorageAttribute::DiskType::Ssd, "SATA_Error_Ct_CRC", "SATA CRC Error Count", "",
						"Number of errors in data transfer via the SATA interface cable");
				// OCZ SSD (smartctl) (description?)
				add(211, AtaStorageAttribute::DiskType::Ssd, "SATA_UNC_Count", "SATA Uncorrectable Error Count", "",
						"Number of errors in data transfer via the SATA interface cable");
				// Shock During Write (custom) (description?)
				add(212, AtaStorageAttribute::DiskType::Hdd, "", "Shock During Write", "",
						"Shock encountered during write operations");
				// Indilinx Barefoot SSD: SATA_Error_Ct_Handshake (smartctl) (description?)
				add(212, AtaStorageAttribute::DiskType::Ssd, "SATA_Error_Ct_Handshake", "SATA Handshake Error Count", "",
						"Number of errors occurring during SATA handshake.");
				// OCZ SSD (smartctl) (description?)
				add(212, AtaStorageAttribute::DiskType::Ssd, "Pages_Requiring_Rd_Rtry", "Pages Requiring Read Retry", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(212, AtaStorageAttribute::DiskType::Ssd, "NAND_Reads_with_Retry", "Number of NAND Reads with Retry", "",
						"");
				// Sandisk SSDs: (smartctl) (description?)
				add(212, AtaStorageAttribute::DiskType::Ssd, "SATA_PHY_Error", "SATA Physical Error Count", "",
						"");
				// Indilinx Barefoot SSD: Indilinx_Internal (smartctl) (description?)
				add(213, AtaStorageAttribute::DiskType::Ssd, "Indilinx_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// OCZ SSD (smartctl) (description?)
				add(213, AtaStorageAttribute::DiskType::Ssd, "Simple_Rd_Rtry_Attempts", "Simple Read Retry Attempts", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(213, AtaStorageAttribute::DiskType::Ssd, "Snmple_Retry_Attempts", "Simple Retry Attempts", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(213, AtaStorageAttribute::DiskType::Ssd, "Simple_Retry_Attempts", "Simple Retry Attempts", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(213, AtaStorageAttribute::DiskType::Ssd, "Adaptv_Rd_Rtry_Attempts", "Adaptive Read Retry Attempts", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(214, AtaStorageAttribute::DiskType::Ssd, "Adaptive_Retry_Attempts", "Adaptive Retry Attempts", "",
						"");
				// Kingston SSD (smartctl)
				add(218, AtaStorageAttribute::DiskType::Ssd, "CRC_Error_Count", "CRC Error Count", "",
						"");
				// Disk Shift (smartctl)
				// Note: There's also smartctl shortcut option "-v 220,temp" (possibly for Temperature Celsius),
				// but it's not used anywhere, so we ignore it.
				add(220, AtaStorageAttribute::DiskType::Hdd, "Disk_Shift", "Disk Shift", "",
						"Shift of disks towards spindle. Shift of disks is possible as a result of a strong shock or a fall, high temperature, or some other reasons.");
				// G-sense error rate (smartctl)
				add(221, AtaStorageAttribute::DiskType::Hdd, "G-Sense_Error_Rate", "G-Sense Error Rate", "",
						"Number of errors resulting from externally-induced shock and vibration (Raw value). May indicate incorrect installation.");
				// OCZ SSD (smartctl) (description?)
				add(213, AtaStorageAttribute::DiskType::Ssd, "Int_Data_Path_Prot_Unc", "Internal Data Path Protection Uncorrectable", "",
						"");
				// Loaded Hours (smartctl)
				add(222, AtaStorageAttribute::DiskType::Hdd, "Loaded_Hours", "Loaded Hours", "",
						"Number of hours spent operating under load (movement of magnetic head armature) (Raw value)");
				// OCZ SSD (smartctl) (description?)
				add(222, AtaStorageAttribute::DiskType::Ssd, "RAID_Recovery_Count", "RAID Recovery Count", "",
						"");
				// Load/Unload Retry Count (smartctl) (description?)
				add(223, AtaStorageAttribute::DiskType::Hdd, "Load_Retry_Count", "Load / Unload Retry Count", "",
						"Number of times the head armature entered / left the data zone.");
				// Load Friction (smartctl)
				add(224, AtaStorageAttribute::DiskType::Hdd, "Load_Friction", "Load Friction", "",
						"Resistance caused by friction in mechanical parts while operating. An increase of Raw value may mean that there is "
						"a problem with the mechanical subsystem of the drive.");
				// OCZ SSD (smartctl) (description?)
				add(224, AtaStorageAttribute::DiskType::Ssd, "In_Warranty", "In Warranty", "",
						"");
				// Load/Unload Cycle Count (smartctl) (description?)
				add(225, AtaStorageAttribute::DiskType::Hdd, "Load_Cycle_Count", "Load / Unload Cycle Count", "",
						"Total number of load cycles.");
				// Intel SSD: Host_Writes_32MiB (smartctl) (description?)
				add(225, AtaStorageAttribute::DiskType::Ssd, "Host_Writes_32MiB", "Host Written (32 MiB)", "",
						"Total number of sectors written by the host system. The Raw value is increased by 1 for every 32 MiB written by the host.");
				// OCZ SSD (smartctl) (description?)
				add(225, AtaStorageAttribute::DiskType::Ssd, "DAS_Polarity", "DAS Polarity", "",
						"");
				// Innodisk SSDs: (smartctl) (description?)
				add(225, AtaStorageAttribute::DiskType::Ssd, "Data_Log_Write_Count", "Data Log Write Count", "",
						"");
				// Load-in Time (smartctl)
				add(226, AtaStorageAttribute::DiskType::Hdd, "Load-in_Time", "Load-in Time", "",
						"Total time of loading on the magnetic heads actuator. Indicates total time in which the drive was under load "
						"(on the assumption that the magnetic heads were in operating mode and out of the parking area).");
				// Intel SSD: Intel_Internal (smartctl)
				add(226, AtaStorageAttribute::DiskType::Ssd, "Intel_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Intel SSD: Workld_Media_Wear_Indic (smartctl)
				add(226, AtaStorageAttribute::DiskType::Ssd, "Workld_Media_Wear_Indic", "Timed Workload Media Wear", "",
						"Timed workload media wear indicator (percent*1024)");
				// OCZ SSD (smartctl) (description?)
				add(226, AtaStorageAttribute::DiskType::Ssd, "Partial_Pfail", "Partial Program Fail", "",
						"");
				// Torque Amplification Count (aka TA) (smartctl)
				add(227, AtaStorageAttribute::DiskType::Hdd, "Torq-amp_Count", "Torque Amplification Count", "",
						"Number of attempts to compensate for platter speed variations.");
				// Intel SSD: Intel_Internal (smartctl)
				add(227, AtaStorageAttribute::DiskType::Ssd, "Intel_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Intel SSD: Workld_Host_Reads_Perc (smartctl)
				add(227, AtaStorageAttribute::DiskType::Ssd, "Workld_Host_Reads_Perc", "Timed Workload Host Reads %", "",
						"");
				// Power-Off Retract Count (smartctl)
				add(228, "Power-off_Retract_Count", "Power-Off Retract Count", "",
						"Number of times the magnetic armature was retracted automatically as a result of power loss.");
				// Intel SSD: Intel_Internal (smartctl)
				add(228, AtaStorageAttribute::DiskType::Ssd, "Intel_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Intel SSD: Workload_Minutes (smartctl)
				add(228, AtaStorageAttribute::DiskType::Ssd, "Workload_Minutes", "Workload (Minutes)", "",
						"");
				// Transcend SSD: Halt_System_ID (smartctl) (description?)
				add(229, AtaStorageAttribute::DiskType::Ssd, "Halt_System_ID", "Halt System ID", "",
						"Halt system ID and flash ID");
				// InnoDisk SSD (smartctl)
				add(229, AtaStorageAttribute::DiskType::Ssd, "Flash_ID", "Flash ID", "",
						"Flash ID");
				// IBM: GMR Head Amplitude (smartctl)
				add(230, AtaStorageAttribute::DiskType::Hdd, "Head_Amplitude", "GMR Head Amplitude", "",
						"Amplitude of heads trembling (GMR-head) in running mode.");
				// Sandforce SSD: Life_Curve_Status (smartctl) (description?)
				add(230, AtaStorageAttribute::DiskType::Ssd, "Life_Curve_Status", "Life Curve Status", "",
						"Current state of drive operation based upon the Life Curve.");
				// OCZ SSD (smartctl) (description?)
				add(230, AtaStorageAttribute::DiskType::Ssd, "SuperCap_Charge_Status", "Super-Capacitor Charge Status", "",
						"0 means not charged, 1 - fully charged, 2 - unknown.");
				// OCZ SSD (smartctl) (description?)
				add(230, AtaStorageAttribute::DiskType::Ssd, "Write_Throttling", "Write Throttling", "",
						"");
				// Sandisk SSD (smartctl) (description?)
				add(230, AtaStorageAttribute::DiskType::Ssd, "Perc_Write/Erase_Count", "Write / Erase Count (%)", "",
						"");
				// Temperature (Some drives) (smartctl)
				add(231, "Temperature_Celsius", "Temperature", "attr_temperature_celsius",
						"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius). "
						"Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).");
				// Sandforce SSD: SSD_Life_Left
				add(231, AtaStorageAttribute::DiskType::Ssd, "SSD_Life_Left", "SSD Life Left", "attr_ssd_life_left",
						"A measure of drive's estimated life left. A Normalized value of 100 indicates a new drive. "
						"10 means there are reserved blocks left but Program / Erase cycles have been used. "
						"0 means insufficient reserved blocks, drive may be in read-only mode to allow recovery of the data.");
				// Intel SSD: Available_Reservd_Space (smartctl) (description?)
				add(232, AtaStorageAttribute::DiskType::Ssd, "Available_Reservd_Space", "Available reserved space", "",
						"Number of reserved blocks remaining. The Normalized value indicates percentage, with 100 meaning new and 10 meaning the drive being close to its end of life.");
				// Transcend SSD: Firmware_Version_information (smartctl) (description?)
				add(232, AtaStorageAttribute::DiskType::Ssd, "Firmware_Version_Info", "Firmware Version Information", "",
						"Firmware version information (year, month, day, channels, banks).");
				// Same as Firmware_Version_Info, but in older smartctl versions.
				add(232, AtaStorageAttribute::DiskType::Ssd, "Firmware_Version_information", "Firmware Version Information", "",
						"Firmware version information (year, month, day, channels, banks).");
				// OCZ SSD (description?) (smartctl)
				add(232, AtaStorageAttribute::DiskType::Ssd, "Lifetime_Writes", "Lifetime_Writes", "",
						"");
				// Kingston SSD (description?) (smartctl)
				add(232, AtaStorageAttribute::DiskType::Ssd, "Flash_Writes_GiB", "Flash Written (GiB)", "",
						"");
				// Innodisk SSD (description?) (smartctl)
				add(232, AtaStorageAttribute::DiskType::Ssd, "Spares_Remaining_Perc", "Spare Blocks Remaining (%)", "attr_ssd_life_left",
						"Percentage of spare blocks remaining. Spare blocks are used when bad blocks develop.");
				// Innodisk SSD (description?) (smartctl)
				add(232, AtaStorageAttribute::DiskType::Ssd, "Perc_Avail_Resrvd_Space", "Available Reserved Space (%)", "attr_ssd_life_left",
						"Percentage of spare blocks remaining. Spare blocks are used when bad blocks develop.");
				// Intel SSD: Media_Wearout_Indicator (smartctl) (description?)
				add(233, AtaStorageAttribute::DiskType::Ssd, "Media_Wearout_Indicator", "Media Wear Out Indicator", "attr_ssd_life_left",
						"Number of cycles the NAND media has experienced. The Normalized value decreases linearly from 100 to 1 as the average erase cycle "
						"count increases from 0 to the maximum rated cycles.");
				// OCZ SSD
				add(233, AtaStorageAttribute::DiskType::Ssd, "Remaining_Lifetime_Perc", "Remaining Lifetime %", "attr_ssd_life_left",
						"Remaining drive life in % (usually by erase count).");
				// Sandforce SSD: SandForce_Internal (smartctl) (description?)
				add(233, AtaStorageAttribute::DiskType::Ssd, "SandForce_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Transcend SSD: ECC_Fail_Record (smartctl) (description?)
				add(233, AtaStorageAttribute::DiskType::Ssd, "ECC_Fail_Record", "ECC Failure Record", "",
						"Indicates rate of ECC (error-correcting code) failures.");
				// Innodisk SSD (smartctl) (description?)
				add(233, AtaStorageAttribute::DiskType::Ssd, "Flash_Writes_32MiB", "Flash Written (32MiB)", "",
						"");
				// Innodisk SSD (smartctl) (description?)
				add(233, AtaStorageAttribute::DiskType::Ssd, "Total_NAND_Writes_GiB", "Total NAND Written (GiB)", "",
						"");
				// Sandforce SSD: SandForce_Internal (smartctl) (description?)
				add(234, AtaStorageAttribute::DiskType::Ssd, "SandForce_Internal", "Internal Attribute", "",
						"This attribute has been reserved by vendor as internal.");
				// Intel SSD (smartctl)
				add(234, AtaStorageAttribute::DiskType::Ssd, "Thermal_Throttle", "Thermal Throttle", "",
						"");
				// Transcend SSD: Erase_Count_Avg (smartctl) (description?)
				add(234, AtaStorageAttribute::DiskType::Ssd, "Erase_Count_Avg/Max", "Erase Count Average / Maximum", "",
						"");
				// Innodisk SSD (smartctl) (description?)
				add(234, AtaStorageAttribute::DiskType::Ssd, "Flash_Reads_32MiB", "Flash Read (32MiB)", "",
						"");
				// Sandisk SSD (smartctl) (description / name?)
				add(234, AtaStorageAttribute::DiskType::Ssd, "Perc_Write/Erase_Ct_BC", "Write / Erase Count BC (%)", "",
						"");
				// Sandforce SSD: SuperCap_Health (smartctl) (description?)
				add(235, AtaStorageAttribute::DiskType::Ssd, "SuperCap_Health", "Supercapacitor Health", "",
						"");
				// Transcend SSD: Block_Count_Good/System (smartctl) (description?)
				add(235, AtaStorageAttribute::DiskType::Ssd, "Block_Count_Good/System", "Good / System Free Block Count", "",
						"Good block count and system free block count.");
				// InnoDisk SSD (smartctl). (description / name?)
				add(235, AtaStorageAttribute::DiskType::Ssd, "Later_Bad_Block", "Later Bad Block", "",
						"");
				// InnoDisk SSD (smartctl). (description / name?)
				add(235, AtaStorageAttribute::DiskType::Ssd, "Later_Bad_Blk_Inf_R/W/E", "Later Bad Block Read / Write / Erase", "",
						"");
				// Samsung SSD (smartctl). (description / name?)
				add(235, AtaStorageAttribute::DiskType::Ssd, "POR_Recovery_Count", "POR Recovery Count", "",
						"");
				// InnoDisk SSD (smartctl). (description / name?)
				add(236, AtaStorageAttribute::DiskType::Ssd, "Unstable_Power_Count", "Unstable Power Count", "",
						"");
				// Head Flying Hours (smartctl)
				add(240, AtaStorageAttribute::DiskType::Hdd, "Head_Flying_Hours", "Head Flying Hours", "",
						"Time spent on head is positioning.");
				// Fujitsu: Transfer_Error_Rate (smartctl) (description?)
				add(240, AtaStorageAttribute::DiskType::Hdd, "Transfer_Error_Rate", "Transfer Error Rate", "",
						"");
				// InnoDisk SSD (smartctl). (description / name?)
				add(240, AtaStorageAttribute::DiskType::Ssd, "Write_Head", "Write Head", "",
						"");
				// Total_LBAs_Written (smartctl) (description?)
				add(241, "Total_LBAs_Written", "Total LBAs Written", "",
						"Logical blocks written during lifetime.");
				// Sandforce SSD: Lifetime_Writes_GiB (smartctl) (maybe in 64GiB increments?)
				add(241, AtaStorageAttribute::DiskType::Ssd, "Lifetime_Writes_GiB", "Total GiB Written", "",
						"Total GiB written during lifetime.");
				// Intel SSD: Host_Writes_32MiB (smartctl) (description?)
				add(241, AtaStorageAttribute::DiskType::Ssd, "Host_Writes_32MiB", "Host Written (32 MiB)", "",
						"Total number of sectors written by the host system. The Raw value is increased by 1 for every 32 MiB written by the host.");
				// OCZ SSD (smartctl)
				add(241, AtaStorageAttribute::DiskType::Ssd, "Host_Writes_GiB", "Host Written (GiB)", "",
						"Total number of sectors written by the host system. The Raw value is increased by 1 for every GiB written by the host.");
				// Sandisk SSD (smartctl)
				add(241, AtaStorageAttribute::DiskType::Ssd, "Total_Writes_GiB", "Total Written (GiB)", "",
						"Total GiB written.");
				// Toshiba SSD (smartctl)
				add(241, AtaStorageAttribute::DiskType::Ssd, "Host_Writes", "Host Written", "",
						"Total number of sectors written by the host system.");
				// Total_LBAs_Read (smartctl) (description?)
				add(242, "Total_LBAs_Read", "Total LBAs Read", "",
						"Logical blocks read during lifetime.");
				// Sandforce SSD: Lifetime_Writes_GiB (smartctl) (maybe in 64GiB increments?)
				add(242, AtaStorageAttribute::DiskType::Ssd, "Lifetime_Reads_GiB", "Total GiB Read", "",
						"Total GiB read during lifetime.");
				// Intel SSD: Host_Reads_32MiB (smartctl) (description?)
				add(242, AtaStorageAttribute::DiskType::Ssd, "Host_Reads_32MiB", "Host Read (32 MiB)", "",
						"Total number of sectors read by the host system. The Raw value is increased by 1 for every 32 MiB read by the host.");
				// OCZ SSD (smartctl)
				add(242, AtaStorageAttribute::DiskType::Ssd, "Host_Reads_GiB", "Host Read (GiB)", "",
						"Total number of sectors read by the host system. The Raw value is increased by 1 for every GiB read by the host.");
				// Marvell SSD (smartctl)
				add(242, AtaStorageAttribute::DiskType::Ssd, "Host_Reads", "Host Read", "",
						"");
				// Sandisk SSD (smartctl)
				add(241, AtaStorageAttribute::DiskType::Ssd, "Total_Reads_GiB", "Total Read (GiB)", "",
						"Total GiB read.");
				// Intel SSD: (smartctl) (description?)
				add(243, AtaStorageAttribute::DiskType::Ssd, "NAND_Writes_32MiB", "NAND Written (32MiB)", "",
						"");
				// Samsung SSD (smartctl). (description / name?)
				add(243, AtaStorageAttribute::DiskType::Ssd, "SATA_Downshift_Ct", "SATA Downshift Count", "",
						"");
				// Kingston SSDs (description?) (smartctl)
				add(244, AtaStorageAttribute::DiskType::Ssd, "Average_Erase_Count", "Average Erase Count", "",
						"The average of individual erase counts of all the blocks");
				// Samsung SSDs (description?) (smartctl)
				add(244, AtaStorageAttribute::DiskType::Ssd, "Thermal_Throttle_St", "Thermal Throttle Status", "",
						"");
				// Sandisk SSDs (description?) (smartctl)
				add(244, AtaStorageAttribute::DiskType::Ssd, "Thermal_Throttle", "Thermal Throttle Status", "",
						"");
				// Kingston SSDs (smartctl)
				add(245, AtaStorageAttribute::DiskType::Ssd, "Max_Erase_Count", "Maximum Erase Count", "",
						"The maximum of individual erase counts of all the blocks.");
				// Innodisk SSD (smartctl) (description?)
				add(245, AtaStorageAttribute::DiskType::Ssd, "Flash_Writes_32MiB", "Flash Written (32MiB)", "",
						"");
				// Samsung SSD (smartctl) (description?)
				add(245, AtaStorageAttribute::DiskType::Ssd, "Timed_Workld_Media_Wear", "Timed Workload Media Wear", "",
						"");
				// SiliconMotion SSD (smartctl) (description?)
				add(245, AtaStorageAttribute::DiskType::Ssd, "TLC_Writes_32MiB", "TLC Written (32MiB)", "",
						"Total number of sectors written to TLC. The Raw value is increased by 1 for every 32 MiB written by the host.");
				// Crucial / Micron SSD (smartctl)
				add(246, AtaStorageAttribute::DiskType::Ssd, "Total_Host_Sector_Write", "Total Host Sectors Written", "",
						"Total number of sectors written by the host system.");
				// Kingston SSDs (description?) (smartctl)
				add(246, AtaStorageAttribute::DiskType::Ssd, "Total_Erase_Count", "Total Erase Count", "",
						"");
				// Samsung SSD (smartctl) (description?)
				add(246, AtaStorageAttribute::DiskType::Ssd, "Timed_Workld_RdWr_Ratio", "Timed Workload Read/Write Ratio", "",
						"");
				// SiliconMotion SSD (smartctl) (description?)
				add(246, AtaStorageAttribute::DiskType::Ssd, "SLC_Writes_32MiB", "SLC Written (32MiB)", "",
						"Total number of sectors written to SLC. The Raw value is increased by 1 for every 32 MiB written by the host.");
				// Crucial / Micron SSD (smartctl)
				add(247, AtaStorageAttribute::DiskType::Ssd, "Host_Program_Page_Count", "Host Program Page Count", "",
						"");
				// Samsung SSD (smartctl)
				add(247, AtaStorageAttribute::DiskType::Ssd, "Timed_Workld_Timer", "Timed Workload Timer", "",
						"");
				// SiliconMotion SSD (smartctl) (description?)
				add(247, AtaStorageAttribute::DiskType::Ssd, "Raid_Recoverty_Ct", "RAID Recovery Count", "",
						"");
				add(248, AtaStorageAttribute::DiskType::Ssd, "Bckgnd_Program_Page_Cnt", "Background Program Page Count", "",
						"");
				// Intel SSD: NAND_Writes_1GiB (smartctl) (description?)
				add(249, AtaStorageAttribute::DiskType::Ssd, "NAND_Writes_1GiB", "NAND Written (1GiB)", "",
						"");
				// OCZ SSD: Total_NAND_Prog_Ct_GiB (smartctl) (description?)
				add(249, AtaStorageAttribute::DiskType::Ssd, "Total_NAND_Prog_Ct_GiB", "Total NAND Written (1GiB)", "",
						"");
				// Read Error Retry Rate (smartctl) (description?)
				add(250, "Read_Error_Retry_Rate", "Read Error Retry Rate", "",
						"Number of errors found while reading.");
				// Samsung SSD: (smartctl) (description?)
				add(183, AtaStorageAttribute::DiskType::Any, "SATA_Iface_Downshift", "SATA Downshift Error Count", "",
						"");
				// OCZ SSD (smartctl) (description?)
				add(251, AtaStorageAttribute::DiskType::Ssd, "Total_NAND_Read_Ct_GiB", "Total NAND Read (1GiB)", "",
						"");
				// Samsung SSD: (smartctl) (description?)
				add(251, AtaStorageAttribute::DiskType::Any, "NAND_Writes", "NAND Write Count", "",
						"");
				// Free Fall Protection (smartctl) (seagate laptop drives)
				add(254, AtaStorageAttribute::DiskType::Hdd, "Free_Fall_Sensor", "Free Fall Protection", "",
						"Number of free fall events detected by accelerometer sensor.");
			}


			/// Add an attribute description to the attribute database
			void add(AttributeDescription descr)
			{
				id_db[descr.id].emplace_back(std::move(descr));
			}


			/// Add an attribute description to the attribute database
			void add(int32_t id, std::string reported_name, std::string displayable_name,
					std::string generic_name, std::string description)
			{
				add(AttributeDescription(id, AtaStorageAttribute::DiskType::Any,
						std::move(reported_name), std::move(displayable_name), std::move(generic_name), std::move(description)));
			}


			/// Add a previously added description to the attribute database under a
			/// different smartctl name (fill the other members from the previous attribute).
// 			void add(int32_t id, const std::string& reported_name)
// 			{
// 				auto iter = id_db.find(id);
// 				DBG_ASSERT(iter != id_db.end() && !iter->second.empty());
// 				if (iter != id_db.end() || iter->second.empty()) {
// 					AttributeDescription attr = iter->second.front();
// 					add(AttributeDescription(id, AtaStorageAttribute::DiskType::Any, reported_name, attr.displayable_name, attr.generic_name, attr.description));
// 				}
// 			}

			/// Add an attribute description to the attribute database
			void add(int32_t id, AtaStorageAttribute::DiskType type, std::string reported_name, std::string displayable_name,
					std::string generic_name, std::string description)
			{
				add(AttributeDescription(id, type, std::move(reported_name), std::move(displayable_name), std::move(generic_name), std::move(description)));
			}


			/// Add a previously added description to the attribute database under a
			/// different smartctl name (fill the other members from the previous attribute).
			void add(int32_t id, AtaStorageAttribute::DiskType type, std::string reported_name)
			{
				auto iter = id_db.find(id);
				DBG_ASSERT(iter != id_db.end() && !iter->second.empty());
				if (iter != id_db.end() || iter->second.empty()) {
					AttributeDescription attr = iter->second.front();
					add(AttributeDescription(id, type,
							std::move(reported_name), std::move(attr.displayable_name), std::move(attr.generic_name), std::move(attr.description)));
				}
			}


			/// Find the description by smartctl name or id, merging them if they're partial.
			[[nodiscard]] AttributeDescription find(const std::string& reported_name, int32_t id, AtaStorageAttribute::DiskType type) const
			{
				// search by ID first
				auto id_iter = id_db.find(id);
				if (id_iter == id_db.end()) {
					return AttributeDescription();  // not found
				}
				DBG_ASSERT(!id_iter->second.empty());
				if (id_iter->second.empty()) {
					return AttributeDescription();  // invalid DB?
				}

				std::vector<AttributeDescription> type_matched;
				for (const auto& attr_iter : id_iter->second) {
					if (attr_iter.disk_type == type || attr_iter.disk_type == AtaStorageAttribute::DiskType::Any || type == AtaStorageAttribute::DiskType::Any) {
						type_matched.push_back(attr_iter);
					}
				}
				if (type_matched.empty()) {
					return AttributeDescription();  // not found
				}

				// search by smartctl name in ID-supplied vector
				for (const auto& attr_iter : type_matched) {
					// compare them case-insensitively, just in case
					if ( hz::string_to_lower_copy(attr_iter.reported_name) == hz::string_to_lower_copy(reported_name)) {
						return attr_iter;  // found it
					}
				}

				// nothing was found by name, return the first one by that ID.
				return type_matched.front();
			}


		private:

			std::map< int32_t, std::vector<AttributeDescription> > id_db;  ///< id => attribute descriptions

	};




	/// Get program-wide attribute description database
	inline const AttributeDatabase& get_attribute_db()
	{
		static const AttributeDatabase attribute_db;
		return attribute_db;
	}




	/// Attribute description for attribute database
	struct StatisticDescription {
		/// Constructor
		StatisticDescription() = default;

		/// Constructor
		StatisticDescription(std::string reported_name_,
				std::string displayable_name_, std::string generic_name_, std::string description_)
				: reported_name(std::move(reported_name_)), displayable_name(std::move(displayable_name_)),
				generic_name(std::move(generic_name_)), description(std::move(description_))
		{ }

		std::string reported_name;  ///< e.g. Highest Temperature
		std::string displayable_name;  ///< e.g. Highest Temperature (C)
		std::string generic_name;  ///< Generic name to be set on the property.
		std::string description;  ///< Attribute description, can be "".
	};



	/// Devstat entry description database
	class StatisticsDatabase {
		public:

			/// Constructor
			StatisticsDatabase()
			{
				// See http://www.t13.org/documents/UploadedDocuments/docs2016/di529r14-ATAATAPI_Command_Set_-_4.pdf

				// General Statistics

				add("Lifetime Power-On Resets", "", "",
						"The number of times the device has processed a power-on reset.");

				add("Power-on Hours", "", "",
						"The amount of time that the device has been operational since it was manufactured.");

				add("Logical Sectors Written", "", "",
						"The number of logical sectors received from the host. "
						"This statistic is incremented by one for each logical sector that was received from the host without an error.");

				add("Number of Write Commands", "", "",
						"The number of write commands that returned command completion without an error. "
						"This statistic is incremented by one for each write command that returns command completion without an error.");

				add("Logical Sectors Read", "", "",
						"The number of logical sectors sent to the host. "
						"This statistic is incremented by one for each logical sector that was sent to the host without an error.");

				add("Number of Read Commands", "", "",
						"The number of read commands that returned command completion without an error. "
						"This statistic is incremented by one for each read command that returns command completion without an error.");

				add("Date and Time TimeStamp", "", "",
						"a) the TimeStamp set by the most recent SET DATE &amp; TIME EXT command plus the number of "
						"milliseconds that have elapsed since that SET DATE &amp; TIME EXT command was processed;\n"
						"or\n"
						"b) a copy of the Power-on Hours statistic (see A.5.4.4) with the hours unit of measure changed to milliseconds as described");

				add("Pending Error Count", "", "",
						"The number of logical sectors listed in the Pending Errors log.");

				add("Workload Utilization", "", "",
						"An estimate of device utilization as a percentage of the manufacturer's designs for various wear factors "
						"(e.g., wear of the medium, head load events), if any. The reported value can be greater than 100%.");

				add("Utilization Usage Rate", "", "",
						"An estimate of the rate at which device wear factors (e.g., damage to the recording medium) "
						"are being used during a specified interval of time. This statistic is expressed as a percentage of the manufacturer's designs.");

				// Free-Fall Statistics

				add("Number of Free-Fall Events Detected", "", "",
						"The number of free-fall events detected by the device.");

				add("Overlimit Shock Events", "", "",
						"The number of shock events detected by the device "
						"with the magnitude higher than the maximum rating of the device.");

				// Rotating Media Statistics

				add("Spindle Motor Power-on Hours", "", "",
						"The amount of time that the spindle motor has been powered on since the device was manufactured. ");

				add("Head Flying Hours", "", "",
						"The number of hours that the device heads have been flying over the surface of the media since the device was manufactured. ");

				add("Head Load Events", "", "",
						"The number of head load events. A head load event is defined as:\n"
						"a) when the heads are loaded from the ramp to the media for a ramp load device;\n"
						"or\n"
						"b) when the heads take off from the landing zone for a contact start stop device.");

				add("Number of Reallocated Logical Sectors", "", "",
						"The number of logical sectors that have been reallocated after device manufacture.\n\n"
						"If the value is normalized, this is the whole number percentage of the available logical sector reallocation "
						"resources that have been used (i.e., 0-100)."
						"\n\n" + get_uncorrectable_text());

				add("Read Recovery Attempts", "", "",
						"The number of logical sectors that require three or more attempts to read the data from the media for each read command. "
						"This statistic is incremented by one for each logical sector that encounters a read recovery attempt. "
						"These events may be caused by external environmental conditions (e.g., operating in a moving vehicle).");

				add("Number of Mechanical Start Failures", "", "",
						"The number of mechanical start failures after device manufacture. "
						"A mechanical start failure is a failure that prevents the device from achieving a normal operating condition");

				add("Number of Realloc. Candidate Logical Sectors", "Number of Reallocation Candidate Logical Sectors", "",
						"The number of logical sectors that are candidates for reallocation. "
						"A reallocation candidate sector is a logical sector that the device has determined may need to be reallocated."
						"\n\n" + get_uncorrectable_text());

				add("Number of High Priority Unload Events", "", "",
						"The number of emergency head unload events.");

				// General Errors Statistics

				add("Number of Reported Uncorrectable Errors", "", "",
						"The number of errors that are reported as an Uncorrectable Error. "
						"Uncorrectable errors that occur during background activity shall not be counted. "
						"Uncorrectable errors reported by reads to flagged uncorrectable logical blocks should not be counted"
						"\n\n" + get_uncorrectable_text());

				add("Resets Between Cmd Acceptance and Completion", "", "",
						"The number of software reset or hardware reset events that occur while one or more commands have "
						"been accepted by the device but have not reached command completion.");

				// Temperature Statistics

				add("Current Temperature", "Current Temperature (C)", "",
						"Drive temperature (Celsius)");

				add("Average Short Term Temperature", "Average Short Term Temperature (C)", "",
						"A value based on the most recent 144 temperature samples in a 24 hour period.");

				add("Average Long Term Temperature", "Average Long Term Temperature (C)", "",
						"A value based on the most recent 42 Average Short Term Temperature values (1,008 recorded hours).");

				add("Highest Temperature", "Highest Temperature (C)", "",
						"The highest temperature measured after the device is manufactured.");

				add("Lowest Temperature", "Lowest Temperature (C)", "",
						"The lowest temperature measured after the device is manufactured.");

				add("Highest Average Short Term Temperature", "Highest Average Short Term Temperature (C)", "",
						"The highest device Average Short Term Temperature after the device is manufactured.");

				add("Lowest Average Short Term Temperature", "Lowest Average Short Term Temperature (C)", "",
						"The lowest device Average Short Term Temperature after the device is manufactured.");

				add("Highest Average Long Term Temperature", "Highest Average Long Term Temperature (C)", "",
						"The highest device Average Long Term Temperature after the device is manufactured.");

				add("Lowest Average Long Term Temperature", "Lowest Average Long Term Temperature (C)", "",
						"The lowest device Average Long Term Temperature after the device is manufactured.");

				add("Time in Over-Temperature", "Time in Over-Temperature (Minutes)", "",
						"The number of minutes that the device has been operational while the device temperature specification has been exceeded.");

				add("Specified Maximum Operating Temperature", "Specified Maximum Operating Temperature (C)", "",
						"The maximum operating temperature device is designed to operate.");

				add("Time in Under-Temperature", "Time in Under-Temperature (C)", "",
						"The number of minutes that the device has been operational while the temperature is lower than the device minimum temperature specification.");

				add("Specified Minimum Operating Temperature", "Specified Minimum Operating Temperature (C)", "",
						"The minimum operating temperature device is designed to operate.");

				// Transport Statistics

				add("Number of Hardware Resets", "", "",
						"The number of hardware resets received by the device.");

				add("Number of ASR Events", "", "",
						"The number of ASR (Asynchronous Signal Recovery) events.");

				add("Number of Interface CRC Errors", "", "",
						"the number of Interface CRC (checksum) errors reported in the ERROR field since the device was manufactured.");

				// Solid State Device Statistics

				add("Percentage Used Endurance Indicator", "", "",
						"A vendor specific estimate of the percentage of device life used based on the actual device usage "
						"and the manufacturer's prediction of device life. A value of 100 indicates that the estimated endurance "
						"of the device has been consumed, but may not indicate a device failure (e.g., minimum "
						"power-off data retention capability reached for devices using NAND flash technology).");

			}


			/// Add an attribute description to the attribute database
			void add(const std::string& reported_name, const std::string& displayable_name,
					const std::string& generic_name, const std::string& description)
			{
				add(StatisticDescription(reported_name, displayable_name, generic_name, description));
			}


			/// Add an devstat entry description to the devstat database
			void add(const StatisticDescription& descr)
			{
				devstat_db[descr.reported_name] = descr;
			}


			/// Find the description by smartctl name or id, merging them if they're partial.
			[[nodiscard]] StatisticDescription find(const std::string& reported_name) const
			{
				// search by ID first
				auto iter = devstat_db.find(reported_name);
				if (iter == devstat_db.end()) {
					return StatisticDescription();  // not found
				}
				return iter->second;
			}


		private:

			std::map<std::string, StatisticDescription> devstat_db;  ///< reported_name => devstat entry description

	};



	/// Get program-wide devstat description database
	inline const StatisticsDatabase& get_devstat_db()
	{
		static const StatisticsDatabase devstat_db;
		return devstat_db;
	}



	/// Check if a property matches a name (generic or reported)
	inline bool name_match(AtaStorageProperty& p, const std::string& name)
	{
		if (p.generic_name.empty()) {
			return hz::string_to_lower_copy(p.reported_name) == hz::string_to_lower_copy(name);
		}
		return hz::string_to_lower_copy(p.generic_name) == hz::string_to_lower_copy(name);
	}


	/// Check if a property matches a name (generic or reported) and if it does,
	/// set a description on it.
	inline bool auto_set(AtaStorageProperty& p, const std::string& name, const char* descr)
	{
		if (name_match(p, name)) {
			p.set_description(descr);
			return true;
		}
		return false;
	}



	/// Check if a property is an attribute and matches a generic name
	inline bool attr_match(AtaStorageProperty& p, const std::string& generic_name)
	{
		return (p.is_value_type<AtaStorageAttribute>() && p.generic_name == generic_name);
	}



	/// Find a property's attribute in the attribute database and fill the property
	/// with all the readable information we can gather.
	inline void auto_set_attr(AtaStorageProperty& p, AtaStorageAttribute::DiskType disk_type)
	{
		AttributeDescription attr = get_attribute_db().find(p.reported_name, p.get_value<AtaStorageAttribute>().id, disk_type);

		std::string humanized_reported_name;
		std::string ssd_hdd_str;
		bool known_by_smartctl = !app_pcre_match("/Unknown_(HDD|SSD)_?Attr.*/i", p.reported_name, &ssd_hdd_str);
		if (known_by_smartctl) {
			humanized_reported_name = " " + p.reported_name + " ";  // spaces are for easy replacements

			static std::unordered_map<std::string, std::string> replacement_map = {
					{"_", " "},
					{"/", " / "},
					{" Ct ", " Count "},
					{" Tot ", " Total "},
					{" Blk ", " Block "},
					{" Cel ", " Celsius "},
					{" Uncorrect ", " Uncorrectable "},
					{" Cnt ", " Count "},
					{" Offl ", " Offline "},
					{" UNC ", " Uncorrectable "},
					{" Err ", " Error "},
					{" Errs ", " Errors "},
					{" Perc ", " Percent "},
					{" Ct ", " Count "},
					{" Avg ", " Average "},
					{" Max ", " Maximum "},
					{" Min ", " Minimum "}
			};

			hz::string_replace_array(humanized_reported_name, replacement_map);
			hz::string_trim(humanized_reported_name);
			hz::string_remove_adjacent_duplicates(humanized_reported_name, ' ');  // may happen with slashes
		}

		if (attr.displayable_name.empty()) {
			// try to display something sensible (use humanized form of smartctl name)
			if (!humanized_reported_name.empty()) {
				attr.displayable_name = humanized_reported_name;

			} else {  // unknown to smartctl
				if (hz::string_to_upper_copy(ssd_hdd_str) == "SSD") {
					attr.displayable_name = "Unknown SSD Attribute";
				} else if (hz::string_to_upper_copy(ssd_hdd_str) == "HDD") {
					attr.displayable_name = "Unknown HDD Attribute";
				} else {
					attr.displayable_name = "Unknown Attribute";
				}
			}
		}



		if (attr.description.empty()) {
			attr.description = "No description is available for this attribute.";

		} else {
			bool same_names = true;
			if (known_by_smartctl) {
				// See if humanized smartctl-reported name looks like our found name.
				// If not, show it in description.
				std::string match = " " + humanized_reported_name + " ";
				std::string against = " " + attr.displayable_name + " ";

				static std::unordered_map<std::string, std::string> replacement_map = {
						{" Percent ", " % "},
						{"-", 	""},
						{"(", 	""},
						{")", 	""},
						{" ", 	""},
				};
				hz::string_replace_array(match, replacement_map);
				hz::string_replace_array(against, replacement_map);

				same_names = app_pcre_match("/^" + app_pcre_escape(match) + "$/i", against);
			}

			std::string descr =  std::string("<b>") + attr.displayable_name + "</b>";
			if (!same_names) {
				std::string reported_name_for_descr = hz::string_replace_copy(p.reported_name, '_', ' ');
				descr += "\n<small>Reported by smartctl as <b>\"" + reported_name_for_descr + "\"</b></small>\n";
			}
			descr += "\n";
			descr += attr.description;

			attr.description = descr;
		}

		p.displayable_name = attr.displayable_name;
		p.set_description(attr.description);
		p.generic_name = attr.generic_name;
	}



	/// Find a property's statistic in the statistics database and fill the property
	/// with all the readable information we can gather.
	inline bool auto_set_statistic(AtaStorageProperty& p)
	{
		StatisticDescription sd = get_devstat_db().find(p.reported_name);

		std::string displayable_name = (sd.displayable_name.empty() ? sd.reported_name : sd.displayable_name);

		bool found = !sd.description.empty();
		if (!found) {
			sd.description = "No description is available for this attribute.";

		} else {
			std::string descr =  std::string("<b>") + displayable_name + "</b>\n";
			descr += sd.description;

			if (p.get_value<AtaStorageStatistic>().is_normalized()) {
				descr += "\n\nNote: The value is normalized.";
			}

			sd.description = descr;
		}

		if (!displayable_name.empty()) {
			p.displayable_name = displayable_name;
		}
		p.set_description(sd.description);
		p.generic_name = sd.generic_name;

		return found;
	}


}



bool ata_storage_property_autoset_description(AtaStorageProperty& p, AtaStorageAttribute::DiskType disk_type)
{
	bool found = false;


	// checksum errors first
	if (p.generic_name.find("_checksum_error") != std::string::npos) {
		p.set_description("Checksum errors indicate that SMART data is invalid. This shouldn't happen in normal circumstances.");
		found = true;

	// Section Info
	} else if (p.section == AtaStorageProperty::Section::info) {
		found = auto_set(p, "model_family", "Model family (from smartctl database)")
		|| auto_set(p, "device_model", "Device model")
		|| auto_set(p, "serial_number", "Serial number, unique to each physical drive")
		|| auto_set(p, "capacity", "User-serviceable drive capacity as reported to an operating system")
		|| auto_set(p, "in_smartctl_db", "Whether the device is in smartctl database or not. "
				"If it is, additional information may be provided; otherwise, Raw values of some attributes may be incorrectly formatted.")
		|| auto_set(p, "smart_supported", "Whether the device supports SMART. If not, then only very limited information will be available.")
		|| auto_set(p, "smart_enabled", "Whether the device has SMART enabled. If not, most of the reported values will be incorrect.")
		|| auto_set(p, "aam_feature", "Automatic Acoustic Management (AAM) feature")
		|| auto_set(p, "aam_feature", "Automatic Acoustic Management (AAM) level")
		|| auto_set(p, "apm_feature", "Automatic Power Management (APM) feature")
		|| auto_set(p, "apm_level", "Advanced Power Management (APM) level")
		|| auto_set(p, "dsn_feature", "Device Statistics Notification (DSN) feature")
		|| auto_set(p, "power_mode", "Power mode at the time of query");

		// set just its name as a tooltip
		if (!found) {
			p.set_description(p.displayable_name);
			found = true;
		}

	} else if (p.section == AtaStorageProperty::Section::data) {

		switch (p.subsection) {
			case AtaStorageProperty::SubSection::health:
				found = auto_set(p, "overall_health", "Overall health self-assessment test result. Note: If the drive passes this test, it doesn't mean it's OK. "
						"However, if the drive doesn't pass it, then it's either already dead, or it's predicting its own failure within the next 24 hours. In this case do a backup immediately!");
				break;

			case AtaStorageProperty::SubSection::capabilities:
				found = auto_set(p, "offline_status_group", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
						"This should show if Automatic Offline Data Collection is enabled.")
				|| auto_set(p, "iodc_total_time_length", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
						"This value shows the estimated time required to perform this operation in idle conditions. A value of 0 means unsupported.")
				|| auto_set(p, "short_total_time_length", "This value shows the estimated time required to perform a short self-test in idle conditions. A value of 0 means unsupported.")
				|| auto_set(p, "long_total_time_length", "This value shows the estimated time required to perform a long self-test in idle conditions. A value of 0 means unsupported.")
				|| auto_set(p, "conveyance_total_time_length", "This value shows the estimated time required to perform a conveyance self-test in idle conditions. "
						"A value of 0 means unsupported.")
				|| auto_set(p, "last_selftest_cap_group", "Status of the last self-test run.")
				|| auto_set(p, "offline_cap_group", "Drive properties related to Offline Data Collection and self-tests.")
				|| auto_set(p, "smart_cap_group", "Drive properties related to SMART handling.")
				|| auto_set(p, "error_log_cap_group", "Drive properties related to error logging.")
				|| auto_set(p, "sct_cap_group", "Drive properties related to temperature information.");
				break;

			case AtaStorageProperty::SubSection::attributes:
				found = auto_set(p, "data_structure_version", p.displayable_name.c_str());
				if (!found) {
					auto_set_attr(p, disk_type);
					found = true;  // true, because auto_set_attr() may set "Unknown attribute", which is still "found".
				}
				break;

			case AtaStorageProperty::SubSection::devstat:
				found = auto_set_statistic(p);
				break;

			case AtaStorageProperty::SubSection::error_log:
				found = auto_set(p, "error_log_version", p.displayable_name.c_str())
				|| auto_set(p, "error_log_error_count", "Number of errors in error log. Note: Some manufacturers may list completely harmless errors in this log "
					"(e.g., command invalid, not implemented, etc...).");
// 				|| auto_set(p, "error_log_unsupported", "This device does not support error logging.");  // the property text already says that
				if (p.is_value_type<AtaStorageErrorBlock>()) {
					for (size_t i = 0; i < p.get_value<AtaStorageErrorBlock>().reported_types.size(); ++i) {
						p.set_description(AtaStorageErrorBlock::get_displayable_error_types(p.get_value<AtaStorageErrorBlock>().reported_types));
						found = true;
					}
				}
				break;

			case AtaStorageProperty::SubSection::selftest_log:
				found = auto_set(p, "selftest_log_version", p.displayable_name.c_str())
				|| auto_set(p, "selftest_num_entries", "Number of tests in selftest log. Note: The number of entries may be limited to the newest manual tests.");
		// 		|| auto_set(p, "selftest_log_unsupported", "This device does not support self-test logging.");  // the property text already says that
				break;

			case AtaStorageProperty::SubSection::selective_selftest_log:
				// nothing here
				break;

			case AtaStorageProperty::SubSection::temperature_log:
				found = auto_set(p, "sct_unsupported", "SCT support is needed for SCT temperature logging.");
				break;

			case AtaStorageProperty::SubSection::erc_log:
			case AtaStorageProperty::SubSection::phy_log:
			case AtaStorageProperty::SubSection::directory_log:
			case AtaStorageProperty::SubSection::unknown:
				// nothing
				break;
		}
	}

	return found;
}




WarningLevel ata_storage_property_autoset_warning(AtaStorageProperty& p)
{
	WarningLevel w = WarningLevel::none;
	std::string reason;

	// checksum errors first
	if (p.generic_name.find("_checksum_error") != std::string::npos) {
		w = WarningLevel::warning;
		reason = "The drive may have a broken implementation of SMART, or it's failing.";


	// Section Info
	} else if (p.section == AtaStorageProperty::Section::info) {
		if (name_match(p, "smart_supported") && !p.get_value<bool>()) {
			w = WarningLevel::notice;
			reason = "SMART is not supported. You won't be able to read any SMART information from this drive.";

		} else if (name_match(p, "smart_enabled") && !p.get_value<bool>()) {
			w = WarningLevel::notice;
			reason = "SMART is disabled. You should enable it to read any SMART information from this drive. "
					"Additionally, some drives do not log useful data with SMART disabled, so it's advisable to keep it always enabled.";

		} else if (name_match(p, "info_warning")) {
			w = WarningLevel::notice;
			reason = "Your drive may be affected by the warning, please see the details.";
		}

	} else if (p.section == AtaStorageProperty::Section::data) {

		switch(p.subsection) {
			case AtaStorageProperty::SubSection::health:
				if (name_match(p, "overall_health") && p.get_value<std::string>() != "PASSED") {
					w = WarningLevel::alert;
					reason = "The drive is reporting that it will FAIL very soon. Please back up as soon as possible!";
				}
				break;

			case AtaStorageProperty::SubSection::capabilities:
				// nothing
				break;

			case AtaStorageProperty::SubSection::attributes:
			{
				if (p.is_value_type<AtaStorageAttribute>()) {

					const auto& attr = p.get_value<AtaStorageAttribute>();

					// Set notices for known pre-fail attributes. These are notices only, since the warnings
					// and alerts are shown only in case of attribute failure.

					// Reallocated Sector Count
					if (attr_match(p, "attr_reallocated_sector_count") && attr.raw_value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. "
								"This could be an indication of future failures and/or potential data loss in bad sectors.";

					// Spin-up Retry Count
					} else if (attr_match(p, "attr_spin_up_retry_count") && attr.raw_value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. "
								"Your drive may have problems spinning up, which could lead to a complete mechanical failure. Please back up.";

					// Soft Read Error Rate
					} else if (attr_match(p, "attr_soft_read_error_rate") && attr.raw_value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. "
								"This could be an indication of future failures and/or potential data loss in bad sectors.";

					// Temperature (for some it may be 10xTemp, so limit the upper bound.)
					} else if (attr_match(p, "attr_temperature_celsius")
							&& attr.raw_value_int > 50 && attr.raw_value_int <= 120) {  // 50C
						w = WarningLevel::notice;
						reason = "The temperature of the drive is higher than 50 degrees Celsius. "
								"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

					// Temperature (for some it may be 10xTemp, so limit the upper bound.)
					} else if (attr_match(p, "attr_temperature_celsius_x10") && attr.raw_value_int > 500) {  // 50C
						w = WarningLevel::notice;
						reason = "The temperature of the drive is higher than 50 degrees Celsius. "
								"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

					// Reallocation Event Count
					} else if (attr_match(p, "attr_reallocation_event_count") && attr.raw_value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. "
								"This could be an indication of future failures and/or potential data loss in bad sectors.";

					// Current Pending Sector Count
					} else if ((attr_match(p, "attr_current_pending_sector_count") || attr_match(p, "attr_total_pending_sectors"))
								&& attr.raw_value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. "
								"This could be an indication of future failures and/or potential data loss in bad sectors.";

					// Uncorrectable Sector Count
					} else if ((attr_match(p, "attr_offline_uncorrectable") || attr_match(p, "attr_total_attr_offline_uncorrectable"))
								&& attr.raw_value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. "
								"This could be an indication of future failures and/or potential data loss in bad sectors.";

					// SSD Life Left (%)
					} else if ((attr_match(p, "attr_ssd_life_left"))
								&& attr.value.value() < 50) {
						w = WarningLevel::notice;
						reason = "The drive has less than half of its estimated life left.";

					// SSD Life Used (%)
					} else if ((attr_match(p, "attr_ssd_life_used"))
								&& attr.raw_value_int >= 50) {
						w = WarningLevel::notice;
						reason = "The drive has less than half of its estimated life left.";
					}

					// Now override this with reported SMART attribute failure warnings / errors

					if (attr.when_failed == AtaStorageAttribute::FailTime::now) {  // NOW

						if (attr.attr_type == AtaStorageAttribute::AttributeType::old_age) {  // old-age
							w = WarningLevel::warning;
							reason = "The drive has a failing old-age attribute. Usually this indicates a wear-out. You should consider replacing the drive.";
						} else {  // pre-fail
							w = WarningLevel::alert;
							reason = "The drive has a failing pre-fail attribute. Usually this indicates a that the drive will FAIL soon. Please back up immediately!";
						}

					} else if (attr.when_failed == AtaStorageAttribute::FailTime::past) {  // PAST

						if (attr.attr_type == AtaStorageAttribute::AttributeType::old_age) {  // old-age
							// nothing. we don't warn about e.g. temperature increase in the past
						} else {  // pre-fail
							w = WarningLevel::warning;  // there was a problem, it got corrected (hopefully)
							reason = "The drive had a failing pre-fail attribute, but it has been restored to a normal value. "
									"This may be a serious problem, you should consider replacing the drive.";
						}
					}
				}
				break;
			}

			case AtaStorageProperty::SubSection::devstat:
			{
				if (p.is_value_type<AtaStorageStatistic>()) {
					const auto& statistic = p.get_value<AtaStorageStatistic>();

					if (name_match(p, "Pending Error Count") && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

					// "Workload Utilization" is either normalized, or encodes several values, so we can't use it.
	/*
					} else if (name_match(p, "Workload Utilization") && statistic.value_int >= 50) {
						w = WarningLevel::notice;
						reason = "The drive has less than half of its estimated life left.";

					} else if (name_match(p, "Workload Utilization") && statistic.value_int >= 100) {
						w = WarningLevel::warning;
						reason = "The drive is past its estimated lifespan.";
	*/

					} else if (name_match(p, "Utilization Usage Rate") && statistic.value_int >= 50) {
						w = WarningLevel::notice;
						reason = "The drive has less than half of its estimated life left.";

					} else if (name_match(p, "Utilization Usage Rate") && statistic.value_int >= 100) {
						w = WarningLevel::warning;
						reason = "The drive is past its estimated lifespan.";

					} else if (name_match(p, "Number of Reallocated Logical Sectors") && !statistic.is_normalized() && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

					} else if (name_match(p, "Number of Reallocated Logical Sectors") && statistic.is_normalized() && statistic.value_int <= 0) {
						w = WarningLevel::warning;
						reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

					} else if (name_match(p, "Number of Mechanical Start Failures") && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive is reporting mechanical errors.";

					} else if (name_match(p, "Number of Realloc. Candidate Logical Sectors") && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

					} else if (name_match(p, "Number of Reported Uncorrectable Errors") && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

					} else if (name_match(p, "Current Temperature") && statistic.value_int > 50) {
						w = WarningLevel::notice;
						reason = "The temperature of the drive is higher than 50 degrees Celsius. "
								"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

					} else if (name_match(p, "Time in Over-Temperature") && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The temperature of the drive is or was over the manufacturer-specified maximum. "
								"This may have shortened its lifespan and caused damage. Please install a cooling solution.";

					} else if (name_match(p, "Time in Under-Temperature") && statistic.value_int > 0) {
						w = WarningLevel::notice;
						reason = "The temperature of the drive is or was under the manufacturer-specified minimum. "
								"This may have shortened its lifespan and caused damage. Please operate the drive within manufacturer-specified temperature range.";

					} else if (name_match(p, "Percentage Used Endurance Indicator") && statistic.value_int >= 50) {
						w = WarningLevel::notice;
						reason = "The drive has less than half of its estimated life left.";

					} else if (name_match(p, "Percentage Used Endurance Indicator") && statistic.value_int >= 100) {
						w = WarningLevel::warning;
						reason = "The drive is past its estimated lifespan.";
					}
				}

				break;
			}

			case AtaStorageProperty::SubSection::error_log:
			{
				// Note: The error list table doesn't display any descriptions, so if any
				// error-entry related descriptions are added here, don't forget to enable
				// the tooltips.

				if (name_match(p, "error_log_error_count") && p.get_value<int64_t>() > 0) {
					w = WarningLevel::notice;
					reason = "The drive is reporting internal errors. Usually this means uncorrectable data loss and similar severe errors. "
							"Check the actual errors for details.";

				} else if (name_match(p, "error_log_unsupported")) {
					w = WarningLevel::notice;
					reason = "The drive does not support error logging. This means that SMART error history is unavailable.";
				}

				// Rate individual error log entries.
				if (p.is_value_type<AtaStorageErrorBlock>()) {
					const auto& eb = p.get_value<AtaStorageErrorBlock>();
					if (!eb.reported_types.empty()) {
						WarningLevel error_block_warning = WarningLevel::none;
						for (const auto& reported_type : eb.reported_types) {
							WarningLevel individual_warning = AtaStorageErrorBlock::get_warning_level_for_error_type(reported_type);
							if (individual_warning > error_block_warning) {
								error_block_warning = WarningLevel(individual_warning);
							}
						}
						if (error_block_warning > WarningLevel::none) {
							w = error_block_warning;
							reason = "The drive is reporting internal errors. Your data may be at risk depending on error severity.";
						}
					}
				}

				break;
			}

			case AtaStorageProperty::SubSection::selftest_log:
			{
				// Note: The error list table doesn't display any descriptions, so if any
				// error-entry related descriptions are added here, don't forget to enable
				// the tooltips.

				// Don't include selftest warnings - they may be old or something.
				// Self-tests are carried manually anyway, so the user is expected to check their status anyway.

				if (name_match(p, "selftest_log_unsupported")) {
					w = WarningLevel::notice;
					reason = "The drive does not support self-test logging. This means that SMART test results won't be logged.";
				}
				break;
			}

			case AtaStorageProperty::SubSection::selective_selftest_log:
				// nothing here
				break;

			case AtaStorageProperty::SubSection::temperature_log:
				// Don't highlight SCT Unsupported as warning, it's harmless.
// 				if (name_match(p, "sct_unsupported") && p.value_bool) {
// 					w = WarningLevel::notice;
// 					reason = "The drive does not support SCT Temperature logging.";
// 				}
				// Current temperature
				if (name_match(p, "sct_temperature_celsius") && p.get_value<int64_t>() > 50) {  // 50C
					w = WarningLevel::notice;
					reason = "The temperature of the drive is higher than 50 degrees Celsius. "
							"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";
				}
				break;

			case AtaStorageProperty::SubSection::erc_log:
			case AtaStorageProperty::SubSection::phy_log:
			case AtaStorageProperty::SubSection::directory_log:
			case AtaStorageProperty::SubSection::unknown:
				// nothing here
				break;
		}
	}


	p.warning = w;
	p.warning_reason = reason;

	return w;
}






/// @}
