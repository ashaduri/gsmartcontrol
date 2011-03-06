/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <vector>

#include "hz/string_algo.h"  // string_replace_copy
#include "applib/app_pcrecpp.h"

#include "storage_property_descr.h"



namespace {

	inline bool name_match(StorageProperty& p, const std::string& name)
	{
		return (p.generic_name.empty() ? (p.reported_name == name) : (p.generic_name == name));
	}


	inline bool auto_set(StorageProperty& p, const std::string& name, const char* descr)
	{
		if (name_match(p, name)) {
			p.set_description(descr);
			return true;
		}
		return false;
	}



	// pass -1 as attr_id to match any attribute property
	inline bool attr_match(StorageProperty& p, int32_t attr_id)
	{
		return (p.value_type == StorageProperty::value_type_attribute &&
				(p.value_attribute.id == attr_id || attr_id == -1));
	}


	inline bool auto_set_attr(StorageProperty& p, int32_t attr_id, const char* attr_name, const char* descr)
	{
		if (!attr_match(p, attr_id))
			return false;

		bool has_descr = bool(descr);
		if (!has_descr) {
			p.set_description("No description is available for this attribute");
		}

		std::string smartctl_name = hz::string_replace_copy(p.reported_name, '_', ' ');

		if (attr_id == -1) {  // not in our database
			// use smartctl-reported name. replace "_"'s with spaces.
			p.readable_name = smartctl_name;

			if (has_descr)
				p.set_description(descr);  // "no description" tooltip

		} else {
			bool known_by_smartctl = !app_pcre_match("/Unknown_Attribute/i", p.reported_name);

			// check if smartctl_name is approximately the same as our description.
			std::string regex = app_pcre_escape(smartctl_name);
			std::string match = attr_name;

			std::vector<std::string> searches, replacements;
			searches.push_back("-");
			replacements.push_back(" ");
			searches.push_back("/");
			replacements.push_back(" ");
			searches.push_back("Ct");
			replacements.push_back("Count");

			hz::string_replace_array(regex, searches, replacements);
			hz::string_replace_array(match, searches, replacements);

			bool smartctl_is_same = app_pcre_match("/^" + regex + "$/i", match);

			p.readable_name = attr_name;

			if (has_descr) {
				p.set_description(std::string("<b>") + attr_name + "</b>"
					+ ((known_by_smartctl && !smartctl_is_same) ?
							("\n<small>Reported by smartctl as <b>\"" + smartctl_name + "\"</b></small>\n") : "")
					+ "\n" + descr);
			}
		}

		return true;
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
		found = auto_set(p, "Offline data collection status", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
				"This should show if Automatic Offline Data Collection is enabled.")
		|| auto_set(p, "Total time to complete Offline Data Collection", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
				"This should show if Automatic Offline Data Collection is enabled.");

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_attributes) {
		// Raw Read Error Rate
		found = auto_set_attr(p, 1, "Raw Read Error Rate", "Indicates the rate of read errors that occurred while reading data from a disk surface. A non-zero Raw value may indicate a problem with either the disk surface or read/write heads. "
				"<i>Note:</i> Seagate drives are known to report very high Raw values for this attribute, and it's not an indication of a problem.")
		// Throughput Performance
		|| auto_set_attr(p, 2, "Throughput Performance", "Average efficiency of a drive. Reduction of this attribute value can signal various internal problems.")
		// Spin Up Time (some say it can also happen due to bad PSU or power connection (?))
		|| auto_set_attr(p, 3, "Spin-up Time", "Average time of spindle spin-up time (from stopped to fully operational). Raw value may show this in milliseconds. Changes in spin-up time can reflect problems with the spindle motor.")
		// Start/Stop Count
		|| auto_set_attr(p, 4, "Start/Stop Count", "Number of start/stop cycles of a spindle (Raw value). That is, number of drive spin-ups.")
		// Reallocated Sector Count
		|| auto_set_attr(p, 5, "Reallocated Sector Count", "Number of reallocated sectors (Raw value). Non-zero Raw value indicates a disk surface failure. "
				"\n\nWhen a drive encounters a surface error, it marks that sector as &quot;unstable&quot; (also known as &quot;pending reallocation&quot;). "
				"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
				"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
				"transferring the data so that no changes are reported to the operating system. This is why you generally don't see &quot;bad blocks&quot; "
				"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area.")
		// Read Channel Margin
		|| auto_set_attr(p, 6, "Read Channel Margin", "Margin of a channel while reading data. The function of this attribute is not specified.")
		// Seek Error Rate
		|| auto_set_attr(p, 7, "Seek Error Rate", "Frequency of errors appearance while positioning. When a drive reads data, it positions heads in the needed place. If there is a failure in the mechanical positioning system, a seek error arises. More seek errors indicate worse condition of a disk surface and disk mechanical subsystem. The exact meaning of the Raw value is manufacturer-dependent.")
		// Seek Time Performance
		|| auto_set_attr(p, 8, "Seek Time Performance", "Average efficiency of seek operations of the magnetic heads. If this value is decreasing, it is a sign of problems in the hard disk drive mechanical subsystem.")
		// Power-On Hours (Maxtor may use minutes, Fujitsu may use seconds, some even temperature?)
		|| auto_set_attr(p, 9, "Power-on Time", "Number of hours in power-on state. Raw value shows total count of hours (or minutes, or half-minutes, or seconds, depending on manufacturer) in power-on state.")
		// Spin-up Retry Count
		|| auto_set_attr(p, 10, "Spin-up Retry Count", "Number of retries of spin start attempts (Raw value). An increase of this attribute value is a sign of problems in the hard disk mechanical subsystem.")
		// Calibration Retry Count
		|| auto_set_attr(p, 11, "Calibration Retry Count", "Number of times recalibration was requested, under the condition that the first attempt was unsuccessful (Raw value). A decrease is a sign of problems in the hard disk mechanical subsystem.")
		// Power Cycle Count
		|| auto_set_attr(p, 12, "Power Cycle Count", "Number of complete power start/stop cycles of a drive.")
		// Soft Read Error Rate (same as 201 ?) (description sounds lame, fix?)
		|| auto_set_attr(p, 13, "Soft Read Error Rate", "Uncorrected read errors reported to the operating system (Raw value). If the value is non-zero, you should back up your data.")
		// End to End Error (description?) ("End-to-end seems to be HP only", in seagate it means something else?)
		|| auto_set_attr(p, 184, "End to End Error", NULL)
		// Reported Uncorrectable (seagate). anyone knows what it means? (no guesses please)
		|| auto_set_attr(p, 187, "Reported Uncorrectable", NULL)
		// Command Timeout (description?)
		|| auto_set_attr(p, 188, "Command Timeout", NULL)
		// High Fly Writes (description?)
		|| auto_set_attr(p, 189, "High Fly Writes", "Number of times the recording head is flying outside its normal operating range.")
		// Airflow Temperature (WD Caviar (may be 50 less), Samsung). Temperature or (100 - temp.) on Seagate/Maxtor.
		|| auto_set_attr(p, 190, "Airflow Temperature", "Indicates temperature, 100 - temperature, or something completely different (highly depends on manufacturer and model).")
		// G-sense error rate (same as 221?)
		|| auto_set_attr(p, 191, "G-Sense Error Rate", "Number of errors resulting from externally-induced shock and vibration (Raw value). May indicate incorrect installation.")
		// Power-Off Retract Cycle, Fujitsu: Emergency Retract Cycle Count
		|| auto_set_attr(p, 192, "Emergency Retract Cycle Count", "Number of times the heads were loaded off the media (during power-offs or emergency conditions).")
		// Load/Unload Cycle
		|| auto_set_attr(p, 193, "Load/Unload Cycle", "Number of load/unload cycles into Landing Zone position.")
		// Temperature Celsius (same as 231). This is the most common one. Some Samsungs: 10xTemp.
		|| auto_set_attr(p, 194, "Temperature Celsius", "Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius). Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).")
		// Hardware ECC Recovered, Fujitsu: ECC_On_The_Fly_Count
		|| auto_set_attr(p, 195, "Hardware ECC Recovered", "Number of ECC on the fly errors (Raw value). Users are advised to ignore this attribute.")
		// Reallocation Event Count
		|| auto_set_attr(p, 196, "Reallocation Event Count", "Number of reallocation (remap) operations. Raw value <i>should</i> show the total number of attempts (both successful and unsuccessful) to reallocate sectors. An increase in Raw value indicates a disk surface failure. "
				"\n\nWhen a drive encounters a surface error, it marks that sector as &quot;unstable&quot; (also known as &quot;pending reallocation&quot;). "
				"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
				"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
				"transferring the data so that no changes are reported to the operating system. This is why you generally don't see &quot;bad blocks&quot; "
				"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area.")
		// Current Pending Sector Count
		|| auto_set_attr(p, 197, "Current Pending Sector Count", "Number of &quot;unstable&quot; (waiting to be remapped) sectors (Raw value). If the unstable sector is subsequently read from or written to successfully, this value is decreased and the sector is not remapped. An increase in Raw value indicates a disk surface failure. "
				"\n\nWhen a drive encounters a surface error, it marks that sector as &quot;unstable&quot; (also known as &quot;pending reallocation&quot;). "
				"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
				"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
				"transferring the data so that no changes are reported to the operating system. This is why you generally don't see &quot;bad blocks&quot; "
				"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area.")
		// Offline Uncorrectable, Fujitsu: Off-line_Scan_UNC_Sector_Ct
		|| auto_set_attr(p, 198, "Offline Uncorrectable", "Number of sectors which couldn't be corrected during Offline Data Collection (Raw value). An increase in Raw value indicates a disk surface failure. "
				"The value may be decreased automatically when the errors are corrected (e.g., when an unreadable sector is reallocated and the next Offline test is run to see the change). "
				"\n\nWhen a drive encounters a surface error, it marks that sector as &quot;unstable&quot; (also known as &quot;pending reallocation&quot;). "
				"If the sector is successfully read from or written to at some later point, it is unmarked. If the sector continues to be inaccessible, "
				"the drive reallocates (remaps) it to a specially reserved area as soon as it has a chance (usually during write request or successful read), "
				"transferring the data so that no changes are reported to the operating system. This is why you generally don't see &quot;bad blocks&quot; "
				"on modern drives - if you do, it means that either they have not been remapped yet, or the drive is out of reserved area.")
		// UDMA CRC Error Count
		|| auto_set_attr(p, 199, "UDMA CRC Error Count", "Number of errors in data transfer via the interface cable in UDMA mode, as determined by ICRC (Interface Cyclic Redundancy Check) (Raw value).")
		// Fujitsu: Write Error Rate, WD: Multi-Zone Error Rate. (maybe head flying height too (?))
		|| auto_set_attr(p, 200, "Write Error Count", "Number of errors found when writing to sectors (Raw value). The higher the value, the worse the disk surface condition and/or mechanical subsystem is.")
		// Soft Read Error Rate / Maxtor: Off Track Errors / Fujitsu: Detected TA Count (description?)
		|| auto_set_attr(p, 201, "Soft Read Error Rate", "Uncorrected read errors reported to the operating system (Raw value). If the value is non-zero, you should back up your data.")
		// Data Address Mark Errors / TA Increase Count (same as 227?)
		|| auto_set_attr(p, 202, "TA Increase Count / Data Address Mark Errors", "This attribute may mean several different things. TA Increase Count: Number of attempts to compensate for platter speed variations. Data Address Mark Errors: Frequency of the Data Address Mark errors.")
		// Run Out Cancel. (description ?), Maxtor: ECC Errors
		|| auto_set_attr(p, 203, "ECC Errors", "Number of ECC errors.")
		// Shock_Count_Write_Opern (description?), Maxtor: Soft ECC Correction
		|| auto_set_attr(p, 204, "Soft ECC Correction", "Number of errors corrected by software ECC.")
		// Thermal Asperity Rate (TAR)
		|| auto_set_attr(p, 205, "Thermal Asperity Rate", "Number of thermal asperity errors.")
		// Flying Height
		|| auto_set_attr(p, 206, "Head Flying Height", "The height of the disk heads above the disk surface. A downward trend will often predict a head crash.")
		// Spin High Current
		|| auto_set_attr(p, 207, "Spin High Current", "Amount of high current used to spin up the drive.")
		// Spin Buzz
		|| auto_set_attr(p, 208, "Spin Buzz", "Number of buzz routines to spin up the drive.")
		// Offline Seek Performance (description?)
		|| auto_set_attr(p, 209, "Offline Seek Performance", "Seek performance during Offline Data Collection operations")
		// Vibration During Write. wikipedia says 211, but it's wrong. (description?)
		|| auto_set_attr(p, 210, "Vibration During Write", NULL)
		// Vibration During Read. (description?)
		|| auto_set_attr(p, 211, "Vibration During Read", NULL)
		// Shock During Write (description?)
		|| auto_set_attr(p, 212, "Shock During Write", NULL)
		// Disk Shift / Temperature Celsius (temperature again? which drives?)
		|| auto_set_attr(p, 220, "Disk Shift", "Shift of disks towards spindle. Shift of disks is possible as a result of a strong shock or a fall, or for other reasons.")
		// G-sense error rate
		|| auto_set_attr(p, 221, "G-Sense Error Rate", "Number of errors resulting from externally-induced shock and vibration (Raw value). May indicate incorrect installation.")
		// Loaded Hours
		|| auto_set_attr(p, 222, "Loaded Hours", "Number of hours spent operating under load (movement of magnetic head armature) (Raw value)")
		// Load/Unload Retry Count (description?)
		|| auto_set_attr(p, 223, "Load/Unload Retry Count", "Number of times head armature changed position.")
		// Load Friction
		|| auto_set_attr(p, 224, "Load Friction", "Resistance caused by friction in mechanical parts while operating.")
		// Load/Unload Cycle Count (description?)
		|| auto_set_attr(p, 225, "Load/Unload Cycle Count", "Total number of load cycles.")
		// Load-in Time
		|| auto_set_attr(p, 226, "Load-in Time", "Total time of loading on the magnetic heads actuator. Indicates total time in which the drive was under load (on the assumption that the magnetic heads were in operating mode and out of the parking area).")
		// Torque Amplification Count (aka TA)
		|| auto_set_attr(p, 227, "Torque Amplification Count", "Number of attempts to compensate for platter speed variations.")
		// Power-Off Retract Count
		|| auto_set_attr(p, 228, "Power-off Retract Count", "Number of times the magnetic armature was retracted automatically as a result of cutting power.")
		// GMR Head Amplitude (IBM)
		|| auto_set_attr(p, 230, "GMR Head Amplitude", "Amplitude of heads trembling (GMR-head) in running mode.")
		// Temperature (Some drives)
		|| auto_set_attr(p, 231, "Temperature", "Drive temperature. The Raw value shows built-in heat sensor registrations (in Celsius). Increases in average drive temperature often signal spindle motor problems (unless the increases are caused by environmental factors).")
		// Head Flying Hours
		|| auto_set_attr(p, 240, "Head Flying Hours", "Time while head is positioning.")
		// Read Error Retry Rate (description?)
		|| auto_set_attr(p, 250, "Read Error Retry Rate", "Number of read retries.")
		// Free Fall Protection (seagate laptop drives)
		|| auto_set_attr(p, 254, "Free Fall Protection", "Number of \"Free Fall Events\" detected.")

		// all unknown attributes (not in our DB)
		|| auto_set_attr(p, -1, "", NULL)
		;


	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_error_log) {
		found = auto_set(p, "error_count", "Number of errors in error log. Note: Some manufacturers may list completely harmless errors in this log "
			"(e.g., command invalid, not implemented, etc...).");


	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selftest_log) {
		found = auto_set(p, "selftest_num_entries", "Number of tests in selftest log. Note: This log usually contains only the last 20 or so manual tests. ");

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
		if (attr_match(p, 5) && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Spin-up Retry Count
		} else if (attr_match(p, 10) && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. Your drive may have problems spinning up, which could lead to a complete mechanical failure. Please back up.";

		// Soft Read Error Rate
		} else if ((attr_match(p, 13) || attr_match(p, 201)) && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Temperature (for some it may be 10xTemp, so limit the upper bound.)
		} else if ((attr_match(p, 194) || attr_match(p, 231))
				&& p.value_attribute.raw_value_int > 50 && p.value_attribute.raw_value_int <= 120) {  // 50C
			w = StorageProperty::warning_notice;
			reason = "The temperature of the drive is higher than 50 degrees Celsius. This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

		// Reallocation Event Count
		} else if (attr_match(p, 196) && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Current Pending Sector Count
		} else if (attr_match(p, 197) && p.value_attribute.raw_value_int > 0) {
			w = StorageProperty::warning_notice;
			reason = "The drive has a non-zero Raw value, but there is no SMART warning yet. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// Current Pending Sector Count
		} else if (attr_match(p, 198) && p.value_attribute.raw_value_int > 0) {
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
		if (name_match(p, "error_count") && p.value_integer > 0) {
			w = StorageProperty::warning_warn;
			reason = "The drive is reporting internal errors. Usually this means uncorrectable data loss and similar severe errors. "
					"Check the actual errors for details.";
		}


	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selftest_log) {
		// don't include selftest warnings - they may be old or something.
		// self-tests are carried manually anyway, so the user is expected to check their status anyway.

	} else if (p.section == StorageProperty::section_data && p.subsection == StorageProperty::subsection_selective_selftest_log) {
		// nothing here

	}

	p.warning = w;
	p.warning_reason = reason;

	return w;
}











