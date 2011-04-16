/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <vector>
#include <map>

#include "hz/string_algo.h"  // string_replace_copy
#include "applib/app_pcrecpp.h"

#include "storage_property_descr.h"



namespace {


	struct AttributeDescription {
		AttributeDescription() : id(-1)
		{ }

		AttributeDescription(int32_t id_, const std::string& smartctl_name_,
				const std::string& readable_name_, const std::string& generic_name_, const std::string& description_)
				: id(id_), smartctl_name(smartctl_name_), readable_name(readable_name_),
				generic_name(generic_name_), description(description_)
		{ }

		int32_t id;  ///< e.g. 190
		std::string smartctl_name;  ///< e.g. Airflow_Temperature_Cel
		std::string readable_name;  ///< e.g. Airflow Temperature (C)
		std::string generic_name;  ///< Generic name to be set on the property.
		std::string description;  ///< Attribute description, can be "".
	};



	class AttributeDatabase {
	public:
		AttributeDatabase()
		{
			// Note: The first one with the same ID is the one displayed in case smartctl
			// doesn't return a name. See atacmds.cpp in smartmontools. The rest are
			// from drivedb.h, which contain overrides.
			// Based on: smartmontools r3312 2011-04-05.

			// "default" means it's in the default smartctl DB.
			// "non-default" means it's in drivedb.h.
			// "custom" means it's somewhere else.

			// Descriptions are based on:
			// http://en.wikipedia.org/wiki/S.M.A.R.T.
			// http://kb.acronis.com/taxonomy/term/1644
			// http://www.ariolic.com/activesmart/smart-attributes/
			// http://sourceforge.net/apps/trac/smartmontools/wiki/TocDoc
			// http://www.ocztechnologyforum.com/staff/ryderocz/misc/Sandforce.jpg
			// Intel Solid-State Drive Toolbox User Guide
			// as well as various other sources.

			std::string unc_text = "When a drive encounters a surface error, it marks that sector as &quot;unstable&quot; (also known as &quot;pending reallocation&quot;). "
					"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
					"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
					"transferring the data so that no changes are reported to the operating system. This is why you generally don't see &quot;bad blocks&quot; "
					"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area."
					"\n\nNote: SSDs reallocate blocks as part of their normal operation, so low reallocation counts are not critical for them.";

			// Raw read error rate (default)
			add(1, "Raw_Read_Error_Rate", "Raw Read Error Rate", "",
					"Indicates the rate of read errors that occurred while reading data from a disk surface. A non-zero Raw value may indicate a problem with either the disk surface or read/write heads. "
					"<i>Note:</i> Some drives (e.g. Seagate) are known to report very high Raw values for this attribute, and it's not an indication of a problem.");
			// Throughput Performance (default)
			add(2, "Throughput_Performance", "Throughput Performance", "",
					"Average efficiency of a drive. Reduction of this attribute value can signal various internal problems.");
			// Spin Up Time (default) (some say it can also happen due to bad PSU or power connection (?))
			add(3, "Spin_Up_Time", "Spin-Up Time", "",
					"Average time of spindle spin-up time (from stopped to fully operational). Raw value may show this in milliseconds or seconds. Changes in spin-up time can reflect problems with the spindle motor or power.");
			// Start/Stop Count (default)
			add(4, "Start_Stop_Count", "Start / Stop Count", "",
					"Number of start/stop cycles of a spindle (Raw value). That is, number of drive spin-ups.");
			// Reallocated Sector Count (default)
			add(5, "Reallocated_Sector_Ct", "Reallocated Sector Count", "reallocated_sector_count",
					"Number of reallocated sectors (Raw value). Non-zero Raw value indicates a disk surface failure."
					"\n\n" + unc_text);
			// SandForce SSD: Retired_Block_Count (non-default)
			add(5, "Retired_Block_Count", "Retired Block Rate", "",
					"Indicates estimated remaining life of the drive. Normalized value is (100-100*RBC/MRB) where RBC is the number of retired blocks and MRB is the minimum required blocks.");
			// Read Channel Margin (default)
			add(6, "Read_Channel_Margin", "Read Channel Margin", "",
					"Margin of a channel while reading data. The function of this attribute is not specified.");
			// Seek Error Rate (default)
			add(7, "Seek_Error_Rate", "Seek Error Rate", "",
					"Frequency of errors appearance while positioning. When a drive reads data, it positions heads in the needed place. If there is a failure in the mechanical positioning system, a seek error arises. More seek errors indicate worse condition of a disk surface and disk mechanical subsystem. The exact meaning of the Raw value is manufacturer-dependent.");
			// Seek Time Performance (default)
			add(8, "Seek_Time_Performance", "Seek Time Performance", "",
					"Average efficiency of seek operations of the magnetic heads. If this value is decreasing, it is a sign of problems in the hard disk drive mechanical subsystem.");
			// Power-On Hours (default) (Maxtor may use minutes, Fujitsu may use seconds, some even temperature?)
			add(9, "Power_On_Hours", "Power-On Time", "",
					"Number of hours in power-on state. Raw value shows total count of hours (or minutes, or half-minutes, or seconds, depending on manufacturer) in power-on state.");
			// SandForce SSD: Power_On_Hours_and_Msec (non-default) (description?)
			add(9, "Power_On_Hours_and_Msec");
			// Spin-up Retry Count (default)
			add(10, "Spin_Retry_Count", "Spin-Up Retry Count", "spin_up_retry_count",
					"Number of retries of spin start attempts (Raw value). An increase of this attribute value is a sign of problems in the hard disk mechanical subsystem.");
			// Calibration Retry Count (default)
			add(11, "Calibration_Retry_Count", "Calibration Retry Count", "",
					"Number of times recalibration was requested, under the condition that the first attempt was unsuccessful (Raw value). A decrease is a sign of problems in the hard disk mechanical subsystem.");
			// Power Cycle Count (default)
			add(12, "Power_Cycle_Count", "Power Cycle Count", "",
					"Number of complete power start / stop cycles of a drive.");
			// Soft Read Error Rate (default) (same as 201 ?) (description sounds lame, fix?)
			add(13, "Read_Soft_Error_Rate", "Soft Read Error Rate", "soft_read_error_rate",
					"Uncorrected read errors reported to the operating system (Raw value). If the value is non-zero, you should back up your data.");
			// Sandforce SSD: Soft_Read_Error_Rate (non-default)
			add(13, "Soft_Read_Error_Rate");
			// Maxtor: Average FHC (custom) (description?)
			add(99, "", "Average FHC (Flying Height Control)", "",
					"");
			// Sandforce SSD: Gigabytes_Erased (non-default) (description?)
			add(100, "Gigabytes_Erased", "GiB Erased", "",
					"Number of GiB erased.");
			// STEC CF: (custom)
			add(100, "", "Erase / Program Cycles", "",  // unused
					"Number of Erase / Program cycles of the entire drive.");
			// Maxtor: Maximum FHC (custom) (description?)
			add(101, "", "Maximum FHC (Flying Height Control)", "",
					"");
			// Unknown (source says Maxtor, but it's an SSD thing and Maxtor doesn't have them at this point).
// 			add(101, "", "Translation Table Rebuild", "",
// 					"Indicates power backup fault or internal error resulting in loss of system unit tables.");
			// STEC CF: Translation Table Rebuild (custom)
			add(103, "", "Translation Table Rebuild", "",
					"Indicates power backup fault or internal error resulting in loss of system unit tables.");
			// STEC CF: Reserved Block Count (non-default)
			add(170, "Reserve_Block_Count", "Reserved Block Count", "",
					"Number of reserved (spare) blocks for bad block handling.");
			// Sandforce SSD, STEC CF: Program Fail Count (non-default)
			add(171, "Program_Fail_Count", "Program Fail Count", "",
					"Number of flash program (write) failures. High values may indicate old drive age or other problems.");
			// Sandforce SSD, STEC CF: Erase Fail Count (non-default)
			add(172, "Erase_Fail_Count", "Erase Fail Count", "",
					"Number of flash erase command failures. High values may indicate old drive age or other problems.");
			// STEC CF: Wear Leveling Count (custom) (description?)
			add(173, "", "Wear Leveling Count", "",
					"Indicates the difference between the most worn block and the least worn block.");
			// Sandforce SSD: Unexpected Power Loss (non-default)
			add(174, "Unexpect_Power_Loss_Ct", "Unexpected Power Loss", "",
					"Number of unexpected power loss events.");
			// Program_Fail_Count_Chip (default)
			add(175, "Program_Fail_Count_Chip", "Program Fail Count (Chip)", "",
					"Number of flash program (write) failures. High values may indicate old drive age or other problems.");
			// Erase_Fail_Count_Chip (default)
			add(176, "Erase_Fail_Count_Chip", "Erase Fail Count (Chip)", "",
					"Number of flash erase command failures. High values may indicate old drive age or other problems.");
			// Wear_Leveling_Count (default) (same as Wear_Range_Delta?)
			add(177, "Wear_Leveling_Count", "Wear Leveling Count (Chip)", "",
					"Indicates the difference (in percent) between the most worn block and the least worn block.");
			// Sandforce SSD: Wear_Range_Delta (non-default)
			add(177, "Wear_Range_Delta", "Wear Range Delta", "",
					"Indicates the difference (in percent) between the most worn block and the least worn block.");
			// Used_Rsvd_Blk_Cnt_Chip (default)
			add(178, "Used_Rsvd_Blk_Cnt_Chip", "Used Reserved Block Count (Chip)", "",
					"Number of a chip's used reserved blocks. High values may indicate old drive age or other problems.");
			// Used_Rsvd_Blk_Cnt_Tot (default) (description?)
			add(179, "Used_Rsvd_Blk_Cnt_Tot", "Used Reserved Block Count (Total)", "",
					"Number of used reserved blocks. High values may indicate old drive age or other problems.");
			// Unused_Rsvd_Blk_Cnt_Tot (default)
			add(180, "Unused_Rsvd_Blk_Cnt_Tot", "Unused Reserved Block Count (Total)", "",
					"Number of unused reserved blocks. High values may indicate old drive age or other problems.");
			// Program_Fail_Cnt_Total (default)
			add(181, "Program_Fail_Cnt_Total", "Program Fail Count", "",
					"Number of flash program (write) failures. High values may indicate old drive age or other problems.");
			// Sandforce SSD: Program_Fail_Count (non-default) (Sandforce says it's identical to 171)
			add(181, "Program_Fail_Count");
			// Erase_Fail_Count_Total (default) (description?)
			add(182, "Erase_Fail_Count_Total", "Erase Fail Count", "",
					"Number of flash erase command failures. High values may indicate old drive age or other problems.");
			// Sandforce SSD: Erase_Fail_Count (non-default) (Sandforce says it's identical to 172)
			add(182, "Erase_Fail_Count");
			// Runtime_Bad_Block (default) (description?)
			add(183, "Runtime_Bad_Block", "Runtime Bad Blocks", "",
					"");
			// Samsung, WD: SATA Downshift Error Count (custom) (description?)
			add(183, "", "SATA Downshift Error Count", "",  // unused
					"");
			// End to End Error (default) (description?)
			add(184, "End-to-End_Error", "End to End Error", "",
					"Indicates discrepancy of data between the host and the drive cache.");
			// Sandforce SSD: IO_Error_Detect_Code_Ct (non-default)
			add(184, "IO_Error_Detect_Code_Ct", "Input/Output ECC Error Count", "",
					"");
			// Indilinx Barefoot SSD: IO_Error_Detect_Code_Ct (non-default)
			add(184, "Initial_Bad_Block_Count", "Initial Bad Block Count", "",
					"Factory-determined number of initial bad blocks.");
			// WD: Head Stability (custom)
			add(185, "", "Head Stability", "",
					"");
			// WD: Induced Op-Vibration Detection (custom)
			add(185, "", "Induced Op-Vibration Detection", "",  // unused
					"");
			// Reported Uncorrectable (default)
			add(187, "Reported_Uncorrect", "Reported Uncorrectable", "",
					"Number of errors that could not be recovered using hardware ECC (Error-Correcting Code).");
			// Command Timeout (default)
			add(188, "Command_Timeout", "Command Timeout", "",
					"Number of aborted operations due to drive timeout. High values may indicate problems with cabling or power supply.");
			// High Fly Writes (default)
			add(189, "High_Fly_Writes", "High Fly Writes", "",
					"Some drives can detect when a recording head is flying outside its normal operating range. "
					"If an unsafe fly height condition is encountered, the write process is stopped, and the information "
					"is rewritten or reallocated to a safe region of the drive. This attribute indicates the count of "
					"these errors detected over the lifetime of the drive.");
			// Airflow Temperature (default) (WD Caviar (may be 50 less), Samsung). Temperature or (100 - temp.) on Seagate/Maxtor.
			add(190, "Airflow_Temperature_Cel", "Airflow Temperature", "",
					"Indicates temperature (in Celsius), 100 - temperature, or something completely different (highly depends on manufacturer and model).");
			// G-sense error rate (default) (same as 221?)
			add(191, "G-Sense_Error_Rate", "G-Sense Error Rate", "",
					"Number of errors caused by externally-induced shock and vibration (Raw value). May indicate incorrect installation.");
			// Power-Off Retract Cycle (default)
			add(192, "Power-Off_Retract_Count", "Head Retract Cycle Count", "",
					"Number of times the heads were loaded off the media (during power-offs or emergency conditions).");
			// Intel SSD: Unsafe_Shutdown_Count (non-default)
			add(192, "Unsafe_Shutdown_Count", "Unsafe Shutdown Count", "",
					"Raw value indicates the number of unsafe (unclean) shutdown events over the drive lifetime. "
					"An unsafe shutdown occurs whenever the device is powered off without "
					"STANDBY IMMEDIATE being the last command.");
			// Fujitsu: Emergency Retract Cycle Count (non-default)
			add(192, "Emerg_Retract_Cycle_Ct", "Emergency Retract Cycle Count", "",
					"Number of times the heads were loaded off the media during emergency conditions.");
			// Load/Unload Cycle (default)
			add(193, "Load_Cycle_Count", "Load / Unload Cycle", "",
					"Number of load / unload cycles into Landing Zone position.");
			// Temperature Celsius (default) (same as 231). This is the most common one. Some Samsungs: 10xTemp.
			add(194, "Temperature_Celsius", "Temperature (Celsius)", "temperature_celsius",
					"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius). Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).");
			// Temperature Celsius x 10 (non-default) 
			add(194, "Temperature_Celsius_x10", "Temperature (Celsius) x 10", "temperature_celsius_x10",
					"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius * 10). Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).");
			// Hardware ECC Recovered (default)
			add(195, "Hardware_ECC_Recovered", "Hardware ECC Recovered", "",
					"Number of ECC on the fly errors (Raw value). Users are advised to ignore this attribute.");
			// Fujitsu: ECC_On_The_Fly_Count (non-default)
			add(195, "ECC_On_The_Fly_Count");
			// Sandforce SSD: ECC_Uncorr_Error_Count (non-default) (description?)
			add(195, "ECC_Uncorr_Error_Count", "Uncorrected ECC Error Count", "",
					"Number of uncorrectable errors (UECC).");
			// Indilinx Barefoot SSD: Program_Failure_Blk_Ct (non-default) (description?)
			add(195, "Program_Failure_Blk_Ct", "Program Failure Block Count", "",
					"Number of flash program (write) failures.");
			// Reallocation Event Count (default)
			add(196, "Reallocated_Event_Count", "Reallocation Event Count", "reallocation_event_count",
					"Number of reallocation (remap) operations. Raw value <i>should</i> show the total number of attempts (both successful and unsuccessful) to reallocate sectors. An increase in Raw value indicates a disk surface failure."
					"\n\n" + unc_text);
			// Indilinx Barefoot SSD: Erase_Failure_Blk_Ct (non-default) (description?)
			add(196, "Erase_Failure_Blk_Ct", "Erase Failure Block Count", "",
					"Number of flash erase failures.");
			// Current Pending Sector Count (default)
			add(197, "Current_Pending_Sector", "Current Pending Sector Count", "current_pending_sector_count",
					"Number of &quot;unstable&quot; (waiting to be remapped) sectors (Raw value). If the unstable sector is subsequently read from or written to successfully, this value is decreased and the sector is not remapped. An increase in Raw value indicates a disk surface failure."
					"\n\n" + unc_text);
			// Indilinx Barefoot SSD: Read_Failure_Blk_Ct (non-default) (description?)
			add(197, "Read_Failure_Blk_Ct", "Read Failure Block Count", "",
					"Number of blocks that failed to be read.");
			// Samsung: Total_Pending_Sectors (non-default). From smartctl man page:
			// unlike Current_Pending_Sector, this won't decrease on reallocation.
			add(197, "Total_Pending_Sectors", "Total Pending Sectors", "total_pending_sectors",
					"Number of &quot;unstable&quot; (waiting to be remapped) sectors and already remapped sectors (Raw value). An increase in Raw value indicates a disk surface failure."
					"\n\n" + unc_text);
			// Offline Uncorrectable (default)
			add(198, "Offline_Uncorrectable", "Offline Uncorrectable", "offline_uncorrectable",
					"Number of sectors which couldn't be corrected during Offline Data Collection (Raw value). An increase in Raw value indicates a disk surface failure. "
					"The value may be decreased automatically when the errors are corrected (e.g., when an unreadable sector is reallocated and the next Offline test is run to see the change)."
					"\n\n" + unc_text);
			// Samsung: Offline Uncorrectable (non-default). From smartctl man page:
			// unlike Current_Pending_Sector, this won't decrease on reallocation.
			add(198, "Total_Offl_Uncorrectabl", "Total Offline Uncorrectable", "total_offline_uncorrectable",
					"Number of sectors which couldn't be corrected during Offline Data Collection (Raw value), currently and in the past. An increase in Raw value indicates a disk surface failure."
					"\n\n" + unc_text);
			// Sandforce SSD: Uncorrectable_Sector_Ct (non-default) (same description?)
			add(198, "Uncorrectable_Sector_Ct");
			// Indilinx Barefoot SSD: Read_Sectors_Tot_Ct (non-default) (description?)
			add(198, "Read_Sectors_Tot_Ct", "Total Read Sectors", "",
					"Total count of read sectors.");
			// Fujitsu: Offline_Scan_UNC_SectCt (non-default)
			add(198, "Offline_Scan_UNC_SectCt");
			// Fujitsu version of Offline Uncorrectable (non-default) (old, not in current smartctl)
			add(198, "Off-line_Scan_UNC_Sector_Ct");
			// UDMA CRC Error Count (default)
			add(199, "UDMA_CRC_Error_Count", "UDMA CRC Error Count", "",
					"Number of errors in data transfer via the interface cable in UDMA mode, as determined by ICRC (Interface Cyclic Redundancy Check) (Raw value).");
			// Sandforce SSD: SATA_CRC_Error_Count (non-default) (description?)
			add(199, "SATA_CRC_Error_Count", "SATA CRC Error Count", "",
					"Number of errors in data transfer via the SATA interface cable (Raw value).");
			// Indilinx Barefoot SSD: Write_Sectors_Tot_Ct (non-default) (description?)
			add(199, "Write_Sectors_Tot_Ct", "Total Written Sectors", "",
					"Total count of written sectors.");
			// WD: Multi-Zone Error Rate (default). (maybe head flying height too (?)) (description?)
			add(200, "Multi_Zone_Error_Rate", "Multi Zone Error Rate", "",
					"");
			// Fujitsu: Write Error Rate (non-default)
			add(200, "Write_Error_Count", "Write Error Count", "",
					"Number of errors found when writing to sectors (Raw value). The higher the value, the worse the disk surface condition and/or mechanical subsystem is.");
			// Indilinx Barefoot SSD: Read_Commands_Tot_Ct (non-default) (description?)
			add(200, "Read_Commands_Tot_Ct", "Total Read Commands Issued", "",
					"Total count of read commands issued.");
			// Soft Read Error Rate (default) (description?)
			add(201, "Soft_Read_Error_Rate", "Soft Read Error Rate", "soft_read_error_rate",
					"Uncorrected read errors reported to the operating system (Raw value). If the value is non-zero, you should back up your data.");
			// Sandforce SSD: Unc_Soft_Read_Err_Rate (non-default)
			add(201, "Unc_Soft_Read_Err_Rate");
			// Maxtor: Off Track Errors (custom)
			add(201, "", "Off Track Errors", "",  // unused
					"");
			// Fujitsu: Detected TA Count (non-default) (description?)
			add(201, "Detected_TA_Count", "Torque Amplification Count", "",
					"Number of attempts to compensate for platter speed variations.");
			// Indilinx Barefoot SSD: Write_Commands_Tot_Ct (non-default) (description?)
			add(201, "Write_Commands_Tot_Ct", "Total Write Commands Issued", "",
					"Total count of write commands issued.");
			// WD: Data Address Mark Errors (default)
			add(202, "Data_Address_Mark_Errs", "Data Address Mark Errors", "",
					"Frequency of the Data Address Mark errors.");
			// Fujitsu: TA Increase Count (same as 227?)
			add(202, "TA_Increase_Count", "TA Increase Count", "",
					"Number of attempts to compensate for platter speed variations.");
			// Indilinx Barefoot SSD: Error_Bits_Flash_Tot_Ct (non-default) (description?)
			add(202, "Error_Bits_Flash_Tot_Ct", "Total Count of Error Bits", "",
					"");
			// Maxtor: ECC Errors (default) (description?)
			add(203, "Corr_Read_Errors_Tot_Ct", "ECC Errors", "",
					"Number of ECC errors.");
			// Run Out Cancel (non-default). (description?)
			add(203, "Run_Out_Cancel", "Run Out Cancel", "",
					"Number of ECC errors.");
			// Indilinx Barefoot SSD: Corr_Read_Errors_Tot_Ct (non-default) (description?)
			add(203, "Corr_Read_Errors_Tot_Ct", "Total Corrected Read Errors", "",
					"Total cound of read sectors with correctable errors.");
			// Maxtor: Soft ECC Correction (default)
			add(204, "Soft_ECC_Correction", "Soft ECC Correction", "",
					"Number of errors corrected by software ECC (Error-Correcting Code).");
			// Fujitsu: Shock_Count_Write_Opern (non-default) (description?)
			add(204, "Shock_Count_Write_Opern", "Shock Count During Write Operation", "",
					"");
			// Sandforce SSD: Soft_ECC_Correct_Rate (non-default) (description?)
			add(204, "Soft_ECC_Correct_Rate", "Soft ECC Correction Rate", "",
					"");
			// Indilinx Barefoot SSD: Bad_Block_Full_Flag (non-default) (description?)
			add(204, "Bad_Block_Full_Flag", "Bad Block Area Is Full", "",
					"Indicates whether the bad block (reserved) area is full or not.");
			// Thermal Asperity Rate (TAR) (default)
			add(205, "Thermal_Asperity_Rate", "Thermal Asperity Rate", "",
					"Number of problems caused by high temperature.");
			// Fujitsu: Shock_Rate_Write_Opern (non-default) (description?)
			add(205, "Shock_Rate_Write_Opern", "Shock Rate During Write Operation", "",
					"");
			// Indilinx Barefoot SSD: Max_PE_Count_Spec (non-default) (description?)
			add(205, "Max_PE_Count_Spec", "Maximum PE Count Specification", "",
					"Maximum Program / Erase cycle count as per specification.");
			// Flying Height (default)
			add(206, "Flying_Height", "Head Flying Height", "",
					"The height of the disk heads above the disk surface. A downward trend will often predict a head crash, "
					"while high values may cause read / write errors.");
			// Indilinx Barefoot SSD: Min_Erase_Count (non-default) (description?)
			add(206, "Min_Erase_Count", "Minimum Erase Count", "",
					"The minimum of individual erase counts of all the blocks.");
			// Spin High Current (default)
			add(207, "Spin_High_Current", "Spin High Current", "",
					"Amount of high current needed or used to spin up the drive.");
			// Indilinx Barefoot SSD: Max_Erase_Count (non-default) (description?)
			add(207, "Max_Erase_Count", "Maximum Erase Count", "",
					"The maximum of individual erase counts of all the blocks.");
			// Spin Buzz (default)
			add(208, "Spin_Buzz", "Spin Buzz", "",
					"Number of buzz routines (retries because of low current) to spin up the drive.");
			// Indilinx Barefoot SSD: Average_Erase_Count (non-default) (description?)
			add(208, "Average_Erase_Count", "Average Erase Count", "",
					"The average of individual erase counts of all the blocks.");
			// Offline Seek Performance (default) (description?)
			add(209, "Offline_Seek_Performnce", "Offline Seek Performance", "",
					"Seek performance during Offline Data Collection operations.");
			// Indilinx Barefoot SSD, OCZ SSD: Remaining_Lifetime_Perc (non-default) (description?)
			add(209, "Remaining_Lifetime_Perc", "Remaining Lifetime %", "",
					"Remaining drive life in % (usually by erase count).");
			// Vibration During Write (custom). wikipedia says 211, but it's wrong. (description?)
			add(210, "", "Vibration During Write", "",
					"Vibration encountered during write operations.");
			// Indilinx Barefoot SSD: Indilinx_Internal (non-default) (description?)
			add(210, "Indilinx_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Vibration During Read (description?)
			add(211, "", "Vibration During Read", "",
					"Vibration encountered during read operations.");
			// Indilinx Barefoot SSD: Erase_Failure_Blk_Ct (non-default) (description?)
			add(211, "SATA_Error_Ct_CRC", "SATA CRC Error Count", "",
					"Number of errors in data transfer via the SATA interface cable");
			// Shock During Write (custom) (description?)
			add(212, "", "Shock During Write", "",
					"Shock encountered during write operations");
			// Indilinx Barefoot SSD: SATA_Error_Ct_Handshake (non-default) (description?)
			add(212, "SATA_Error_Ct_Handshake", "SATA Handshake Error Count", "",
					"Number of errors occurring during SATA handshake.");
			// Indilinx Barefoot SSD: Indilinx_Internal (non-default) (description?)
			add(213, "Indilinx_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Disk Shift (default)
			// Note: There's also smartctl shortcut option "-v 220,temp" (possibly for Temperature Celsius),
			// but it's not used anywhere, so we ignore it.
			add(220, "Disk_Shift", "Disk Shift", "",
					"Shift of disks towards spindle. Shift of disks is possible as a result of a strong shock or a fall, high temperature, or some other reasons.");
			// G-sense error rate (default)
			add(221, "G-Sense_Error_Rate", "G-Sense Error Rate", "",
					"Number of errors resulting from externally-induced shock and vibration (Raw value). May indicate incorrect installation.");
			// Loaded Hours (default)
			add(222, "Loaded_Hours", "Loaded Hours", "",
					"Number of hours spent operating under load (movement of magnetic head armature) (Raw value)");
			// Load/Unload Retry Count (default) (description?)
			add(223, "Load_Retry_Count", "Load / Unload Retry Count", "",
					"Number of times the head armature entered / left the data zone.");
			// Load Friction (default)
			add(224, "Load_Friction", "Load Friction", "",
					"Resistance caused by friction in mechanical parts while operating. An increase of Raw value may mean that there is a problem with the mechanical subsystem of the drive.");
			// Load/Unload Cycle Count (default) (description?)
			add(225, "Load_Cycle_Count", "Load / Unload Cycle Count", "",
					"Total number of load cycles.");
			// Intel SSD: Host_Writes_32MiB (non-default) (description?)
			add(225, "Host_Writes_32MiB", "Host Writes (32 MiB)", "",
					"Total number of sectors written by the host system. The Raw value is increased by 1 for every 65536 sectors written by the host.");
			// Load-in Time (default)
			add(226, "Load-in_Time", "Load-in Time", "",
					"Total time of loading on the magnetic heads actuator. Indicates total time in which the drive was under load (on the assumption that the magnetic heads were in operating mode and out of the parking area).");
			// Intel SSD: Intel_Internal (non-default)
			add(226, "Intel_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Intel SSD: Workld_Media_Wear_Indic (non-default)
			add(226, "Workld_Media_Wear_Indic", "Timed Workload Media Wear", "",
					"Timed workload media wear indicator (percent*1024)");
			// Torque Amplification Count (aka TA) (default)
			add(227, "Torq-amp_Count", "Torque Amplification Count", "",
					"Number of attempts to compensate for platter speed variations.");
			// Intel SSD: Intel_Internal (non-default)
			add(227, "Intel_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Intel SSD: Workld_Host_Reads_Perc (non-default)
			add(227, "Workld_Host_Reads_Perc", "Timed Workload Host Reads %", "",
					"");
			// Power-Off Retract Count (default)
			add(228, "Power-off_Retract_Count", "Power-Off Retract Count", "",
					"Number of times the magnetic armature was retracted automatically as a result of power loss.");
			// Intel SSD: Intel_Internal (non-default)
			add(228, "Intel_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Intel SSD: Workload_Minutes (non-default)
			add(228, "Workload_Minutes", "Workload (Minutes)", "",
					"");
			// Transcend SSD: Halt_System_ID (non-default) (description?)
			add(229, "Halt_System_ID", "Halt System ID", "",
					"Halt system ID and flash ID");
			// IBM: GMR Head Amplitude (default)
			add(230, "Head_Amplitude", "GMR Head Amplitude", "",
					"Amplitude of heads trembling (GMR-head) in running mode.");
			// Sandforce SSD: Life_Curve_Status (non-default) (description?)
			add(230, "Life_Curve_Status", "Life Curve Status", "",
					"");
			// Temperature (Some drives) (default)
			add(231, "Temperature_Celsius", "Temperature", "temperature_celsius",
					"Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius). Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).");
			// Sandforce SSD: SSD_Life_Left
			add(231, "SSD_Life_Left", "SSD Life Left", "",
					"A measure of drive's estimated life left. A Normalized value of 100 indicates a new drive. "
					"10 means there are reserved blocks left but Program / Erase cycles have been used. "
					"0 means insufficient reserved blocks, drive may be in read-only mode to allow recovery of the data.");
			// Intel SSD: Available_Reservd_Space (default) (description?)
			add(232, "Available_Reservd_Space", "Available reserved space", "",
					"Number of reserved blocks remaining. The Normalized value indicates percentage, with 100 meaning new and 10 meaning the drive being close to its end of life.");
			// Transcend SSD: Firmware_Version_information (non-default) (description?)
			add(232, "Firmware_Version_Info", "Firmware Version Information", "",
					"Firmware version information (year, month, day, channels, banks).");
			// Same as Firmware_Version_Info, but in older smartctl versions.
			add(232, "Firmware_Version_information", "Firmware Version Information", "",
					"Firmware version information (year, month, day, channels, banks).");
			// Intel SSD: Media_Wearout_Indicator (default) (description?)
			add(233, "Media_Wearout_Indicator", "Media Wear Out Indicator", "",
					"Number of cycles the NAND media has experienced. The Normalized value decreases linearly from 100 to 1 as the average erase cycle "
					"count increases from 0 to the maximum rated cycles.");
			// Sandforce SSD: SandForce_Internal (non-default) (description?)
			add(233, "SandForce_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Transcend SSD: ECC_Fail_Record (non-default) (description?)
			add(233, "ECC_Fail_Record", "ECC Failure Record", "",
					"Indicates rate of ECC (error-correcting code) failures.");
			// Sandforce SSD: SandForce_Internal (non-default) (description?)
			add(234, "SandForce_Internal", "Internal Attribute", "",
					"This attribute has been reserved by vendor as internal.");
			// Transcend SSD: Erase_Count_Avg (non-default) (description?)
			add(234, "Erase_Count_Avg/Max", "Erase Count Average / Maximum", "",
					"");
			// Sandforce SSD: SuperCap_Health (non-default) (description?)
			add(235, "SuperCap_Health", "Supercapacitor Health", "",
					"");
			// Transcend SSD: Block_Count_Good/System (non-default) (description?)
			add(235, "Block_Count_Good/System", "Good / System Free Block Count", "",
					"Good block count and system free block count.");
			// Head Flying Hours (default)
			add(240, "Head_Flying_Hours", "Head Flying Hours", "",
					"Time spent on head is positioning.");
			// Fujitsu: Transfer_Error_Rate (non-default) (description?)
			add(240, "Transfer_Error_Rate", "Transfer Error Rate", "",
					"");
			// Total_LBAs_Written (default) (description?)
			add(241, "Total_LBAs_Written", "Total LBAs Written", "",
					"Logical blocks written during lifetime.");
			// Sandforce SSD: Lifetime_Writes_GiB (non-default) (maybe in 64GiB increments?)
			add(241, "Lifetime_Writes_GiB", "Total GiB Written", "",
					"Total GiB written during lifetime.");
			// Total_LBAs_Read (default) (description?)
			add(242, "Total_LBAs_Read", "Total LBAs Read", "",
					"Logical blocks read during lifetime.");
			// Sandforce SSD: Lifetime_Writes_GiB (non-default) (maybe in 64GiB increments?)
			add(242, "Lifetime_Reads_GiB", "Total GiB Read", "",
					"Total GiB read during lifetime.");
			// Read Error Retry Rate (default) (description?)
			add(250, "Read_Error_Retry_Rate", "Read Error Retry Rate", "",
					"Number of errors found while reading.");
			// Free Fall Protection (default) (seagate laptop drives)
			add(254, "Free_Fall_Sensor", "Free Fall Protection", "",
					"Number of free fall events detected by accelerometer sensor.");
		}


		void add(int32_t id, const std::string& smartctl_name, const std::string& readable_name, 
				const std::string& generic_name, const std::string& description)
		{
			add(AttributeDescription(id, smartctl_name, readable_name, generic_name, description));
		}


		/// Add a previously added description under a different smartctl name
		/// (fill the other members from the previous attribute).
		void add(int32_t id, const std::string& smartctl_name)
		{
			std::map<int32_t, std::vector< AttributeDescription> >::iterator iter = id_db.find(id);
			DBG_ASSERT(iter != id_db.end() && !iter->second.empty());
			if (iter != id_db.end() || iter->second.empty()) {
				AttributeDescription attr = iter->second.front();
				add(AttributeDescription(id, smartctl_name, attr.readable_name, attr.generic_name, attr.description));
			}
		}


		void add(const AttributeDescription& descr)
		{
			id_db[descr.id].push_back(descr);
		}


		/// Find the description by smartctl name or id, merging them if they're partial.
		AttributeDescription find(const std::string& smartctl_name, int32_t id) const
		{
			// search by ID first
			std::map< int32_t, std::vector<AttributeDescription> >::const_iterator id_iter = id_db.find(id);
			if (id_iter == id_db.end()) {
				return AttributeDescription();  // not found
			}
			DBG_ASSERT(!id_iter->second.empty());
			if (id_iter->second.empty()) {
				return AttributeDescription();  // invalid DB?
			}

			// search by smartctl name in ID-supplied vector
			for (std::vector<AttributeDescription>::const_iterator attr_iter = id_iter->second.begin(); attr_iter != id_iter->second.end(); ++attr_iter) {
				// compare them case-insensitively, just in case
				if (hz::string_to_lower_copy(attr_iter->smartctl_name) == hz::string_to_lower_copy(smartctl_name)) {
					return *attr_iter;  // found it
				}
			}

			// nothing was found by name, return the first one by that ID.
			return id_iter->second.front();
		}


		private:

			std::map< int32_t, std::vector<AttributeDescription> > id_db;  ///< id => attribute descriptions
	};


	static const AttributeDatabase s_attribute_db;




	inline bool name_match(StorageProperty& p, const std::string& name)
	{
		if (p.generic_name.empty()) {
			return hz::string_to_lower_copy(p.reported_name) == hz::string_to_lower_copy(name);
		}
		return hz::string_to_lower_copy(p.generic_name) == hz::string_to_lower_copy(name);
	}


	inline bool auto_set(StorageProperty& p, const std::string& name, const char* descr)
	{
		if (name_match(p, name)) {
			p.set_description(descr);
			return true;
		}
		return false;
	}



	inline bool attr_match(StorageProperty& p, const std::string& generic_name)
	{
		return (p.value_type == StorageProperty::value_type_attribute &&
				(p.generic_name == generic_name));
	}



	inline void auto_set_attr(StorageProperty& p)
	{
		AttributeDescription attr = s_attribute_db.find(p.reported_name, p.value_attribute.id);

		std::string humanized_smartctl_name;
		bool known_by_smartctl = !app_pcre_match("/Unknown_Attribute/i", p.reported_name);
		if (known_by_smartctl) {
			humanized_smartctl_name = " " + p.reported_name + " ";  // spaces are for easy replacements

			std::vector<std::string> searches, replacements;
			searches.push_back("_");
			replacements.push_back(" ");
			searches.push_back("/");
			replacements.push_back(" / ");
			searches.push_back(" Ct ");
			replacements.push_back(" Count ");
			searches.push_back(" Tot ");
			replacements.push_back(" Total ");
			searches.push_back(" Blk ");
			replacements.push_back(" Block ");
			searches.push_back(" Cel ");
			replacements.push_back(" Celsius ");
			searches.push_back(" Uncorrect ");
			replacements.push_back(" Uncorrectable ");
			searches.push_back(" Cnt ");
			replacements.push_back(" Count ");
			searches.push_back(" Offl ");
			replacements.push_back(" Offline ");
			searches.push_back(" UNC ");
			replacements.push_back(" Uncorrectable ");
			searches.push_back(" Err ");
			replacements.push_back(" Error ");
			searches.push_back(" Errs ");
			replacements.push_back(" Errors ");
			searches.push_back(" Perc ");
			replacements.push_back(" Percent ");
			searches.push_back(" Ct ");
			replacements.push_back(" Count ");
			searches.push_back(" Avg ");
			replacements.push_back(" Average ");
			searches.push_back(" Max ");
			replacements.push_back(" Maximum ");
			searches.push_back(" Min ");
			replacements.push_back(" Minimum ");

			hz::string_replace_array(humanized_smartctl_name, searches, replacements);
			hz::string_trim(humanized_smartctl_name);
			hz::string_remove_adjacent_duplicates(humanized_smartctl_name, ' ');  // may happen with slashes
		}

		if (attr.readable_name.empty()) {
			// try to display something sensible (use humanized form of smartctl name)
			if (!humanized_smartctl_name.empty()) {
				attr.readable_name = humanized_smartctl_name;

			} else {  // unknown by smartctl
				attr.readable_name = "Unknown Attribute";
			}
		}



		if (attr.description.empty()) {
			attr.description = "No description is available for this attribute.";

		} else {
			bool same_names = true;
			if (known_by_smartctl) {
				// See if humanized smartctl-reported name looks like our found name.
				// If not, show it in description.
				std::string match = " " + humanized_smartctl_name + " ";
				std::string against = " " + attr.readable_name + " ";

				std::vector<std::string> searches, replacements;
				searches.push_back(" Percent ");
				replacements.push_back(" % ");
				searches.push_back("-");
				replacements.push_back("");
				searches.push_back("(");
				replacements.push_back("");
				searches.push_back(")");
				replacements.push_back("");
				searches.push_back(" ");
				replacements.push_back("");
				hz::string_replace_array(match, searches, replacements);
				hz::string_replace_array(against, searches, replacements);

				same_names = app_pcre_match("/^" + app_pcre_escape(match) + "$/i", against);
			}

			std::string descr =  std::string("<b>") + attr.readable_name + "</b>";
			if (!same_names) {
				std::string smartctl_name_for_descr = hz::string_replace_copy(p.reported_name, '_', ' ');
				descr += "\n<small>Reported by smartctl as <b>\"" + smartctl_name_for_descr + "\"</b></small>\n";
			}
			descr += "\n";
			descr += attr.description;

			attr.description = descr;
		}

		p.readable_name = attr.readable_name;
		p.set_description(attr.description);
		p.generic_name = attr.generic_name;
	}

}



bool storage_property_autoset_description(StorageProperty& p)
{
	bool found = false;


	// checksum errors first
	if (p.generic_name.find("_checksum_error") != std::string::npos) {
		p.set_description("Checksum errors indicate that SMART data is invalid. This shouldn't happen in normal circumstances.");
		found = true;

	// Section Info
	} else if (p.section == StorageProperty::section_info) {
		found = auto_set(p, "Serial Number", "Serial number, unique to each physical drive.")
		|| auto_set(p, "User Capacity", "User-serviceable drive capacity as reported to an operating system.")
		|| auto_set(p, "in_smartctl_db", "Whether the device is in smartctl database or not. If it is, additional information may be provided; otherwise, Raw values of some attributes may be incorrectly formatted.")
		|| auto_set(p, "smart_supported", "Whether the device supports SMART. If not, then only very limited information will be available.")
		|| auto_set(p, "smart_enabled", "Whether the device has SMART enabled. If not, most of the reported values will be incorrect.");

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_health) {
		found = auto_set(p, "overall_health", "Overall health self-assessment test result. Note: If the drive passes this test, it doesn't mean it's OK. "
				"However, if the drive doesn't pass it, then it's either already dead, or it's predicting its own failure within the next 24 hours. In this case do a backup immediately!");

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_capabilities) {
		found = auto_set(p, "offline_status_group", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
				"This should show if Automatic Offline Data Collection is enabled.")
		|| auto_set(p, "iodc_total_time_length", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
				"This value shows the estimated time required to perform this operation in idle conditions. A value of 0 means unsupported.")
		|| auto_set(p, "short_total_time_length", "This value shows the estimated time required to perform a short self-test in idle conditions. A value of 0 means unsupported.")
		|| auto_set(p, "long_total_time_length", "This value shows the estimated time required to perform a long self-test in idle conditions. A value of 0 means unsupported.")
		|| auto_set(p, "conveyance_total_time_length", "This value shows the estimated time required to perform a conveyance self-test in idle conditions. A value of 0 means unsupported.")
		|| auto_set(p, "last_selftest_cap_group", "Status of the last self-test run.")
		|| auto_set(p, "offline_cap_group", "Drive properties related to Offline Data Collection and self-tests.")
		|| auto_set(p, "smart_cap_group", "Drive properties related to SMART handling.")
		|| auto_set(p, "error_log_cap_group", "Drive properties related to error logging.")
		|| auto_set(p, "sct_cap_group", "Drive properties related to temperature information.");

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_attributes) {
		auto_set_attr(p);
		found = true;  // true, because auto_set_attr() may set "Unknown attribute", which is still "found".


	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_error_log) {
		found = auto_set(p, "error_count", "Number of errors in error log. Note: Some manufacturers may list completely harmless errors in this log "
			"(e.g., command invalid, not implemented, etc...).");
// 		|| auto_set(p, "error_log_unsupported", "This device does not support error logging.");  // the property text already says that


	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selftest_log) {
		found = auto_set(p, "selftest_num_entries", "Number of tests in selftest log. Note: This log usually contains only the last 20 or so manual tests. ");
// 		|| auto_set(p, "selftest_log_unsupported", "This device does not support self-test logging.");  // the property text already says that

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selective_selftest_log) {
		// nothing here

	}


	return found;
}




StorageProperty::warning_t storage_property_autoset_warning(StorageProperty& p)
{
	StorageProperty::warning_t w = StorageProperty::warning_none;
	std::string reason;

	// checksum errors first
	if (p.generic_name.find("_checksum_error") != std::string::npos) {
		w = StorageProperty::warning_warn;
		reason = "The drive may have a broken implementation of SMART, or it's failing.";


	// Section Info
	} else if (p.section == StorageProperty::section_info) {
		if (name_match(p, "smart_supported") && !p.value_bool) {
			w = StorageProperty::warning_notice;
			reason = "SMART is not supported. You won't be able to read any SMART information from this drive.";

		} else if (name_match(p, "smart_enabled") && !p.value_bool) {
			w = StorageProperty::warning_notice;
			reason = "SMART is disabled. You shoud enable it to read any SMART information from this drive. "
					"Additionally, some drives do not log useful data with SMART disabled, so it's advisable to keep it always enabled.";
		}

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_health) {
		if (name_match(p, "overall_health") && p.value_string != "PASSED") {
			w = StorageProperty::warning_alert;
			reason = "The drive is reporting that it will FAIL very soon. Please back up as soon as possible!";
		}

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_capabilities) {
		// nothing

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_attributes) {

		// Set notices for known pre-fail attributes

		// Reallocated Sector Count
		if (attr_match(p, "reallocated_sector_count") && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Spin-up Retry Count
		} else if (attr_match(p, "spin_up_retry_count") && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. Your drive may have problems spinning up, which could lead to a complete mechanical failure. Please back up.";

		// Soft Read Error Rate
		} else if (attr_match(p, "soft_read_error_rate") && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Temperature (for some it may be 10xTemp, so limit the upper bound.)
		} else if (attr_match(p, "temperature_celsius")
				&& p.value_attribute.raw_value_int > 50 && p.value_attribute.raw_value_int <= 120) {  // 50C
			w = StorageProperty::warning_notice;
			reason = "The temperature of the drive is higher than 50 degrees Celsius. This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

		// Temperature (for some it may be 10xTemp, so limit the upper bound.)
		} else if (attr_match(p, "temperature_celsius_x10") && p.value_attribute.raw_value_int > 500) {  // 50C
			w = StorageProperty::warning_notice;
			reason = "The temperature of the drive is higher than 50 degrees Celsius. This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

		// Reallocation Event Count
		} else if (attr_match(p, "reallocation_event_count") && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Current Pending Sector Count
		} else if ((attr_match(p, "current_pending_sector_count") || attr_match(p, "total_pending_sectors"))
					&& p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Current Pending Sector Count
		} else if ((attr_match(p, "offline_uncorrectable") || attr_match(p, "total_offline_uncorrectable"))
					&& p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		}


		// Now override this with SMART warnings / errors

		if (p.value_type == StorageProperty::value_type_attribute) {
			if (p.value_attribute.when_failed == StorageAttribute::fail_time_now) {  // NOW

				if (p.value_attribute.attr_type == StorageAttribute::attr_type_oldage) {  // old-age
					w = StorageProperty::warning_warn;
					reason = "The drive has a failing old-age attribute. Usually this indicates a wear-out. You should consider replacing the drive.";
				} else {  // pre-fail
					w = StorageProperty::warning_alert;
					reason = "The drive has a failing pre-fail attribute. Usually this indicates a that the drive will FAIL soon. Please back up immediately!";
				}

			} else if (p.value_attribute.when_failed == StorageAttribute::fail_time_past) {  // PAST

				if (p.value_attribute.attr_type == StorageAttribute::attr_type_oldage) {  // old-age
					// nothing. we don't warn about e.g. temperature increase in the past
				} else {  // pre-fail
					w = StorageProperty::warning_warn;  // there was a problem, it got corrected (hopefully)
					reason = "The drive had a failing pre-fail attribute, but it has been restored to a normal value. This may be a serious problem, you should consider replacing the drive.";
				}
			}
		}



	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_error_log) {

		// Note: The error list table doesn't display any descriptions, so if any
		// error-entry related descriptions are added here, don't forget to enable
		// the tooltips.

		if (name_match(p, "error_count") && p.value_integer > 0) {
			w = StorageProperty::warning_warn;
			reason = "The drive is reporting internal errors. Usually this means uncorrectable data loss and similar severe errors. "
					"Check the actual errors for details.";

		} else if (name_match(p, "error_log_unsupported")) {
			w = StorageProperty::warning_notice;
			reason = "The drive does not support error logging. This means that SMART error history is unavailable.";
		}


	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selftest_log) {

		// Note: The error list table doesn't display any descriptions, so if any
		// error-entry related descriptions are added here, don't forget to enable
		// the tooltips.

		// Don't include selftest warnings - they may be old or something.
		// Self-tests are carried manually anyway, so the user is expected to check their status anyway.

		if (name_match(p, "selftest_log_unsupported")) {
			w = StorageProperty::warning_notice;
			reason = "The drive does not support self-test logging. This means that SMART test results won't be logged.";
		}

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selective_selftest_log) {
		// nothing here

	}

	p.warning = w;
	p.warning_reason = reason;

	return w;
}











