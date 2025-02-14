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

//#include <glibmm.h>
#include <utility>
//#include <vector>
//#include <map>
//#include <unordered_map>

#include "hz/string_algo.h"  // string_replace_copy
#include "applib/app_regex.h"

#include "storage_property_descr.h"
#include "warning_colors.h"
#include "storage_property_descr_ata_attribute.h"
#include "storage_property_descr_ata_statistic.h"
#include "storage_property_descr_nvme_attribute.h"


namespace {


	/// Check if a property matches a name (generic or reported)
	inline bool name_match(StorageProperty& p, const std::string& name)
	{
		if (p.generic_name.empty()) {
			return hz::string_to_lower_copy(p.reported_name) == hz::string_to_lower_copy(name);
		}
		return hz::string_to_lower_copy(p.generic_name) == hz::string_to_lower_copy(name);
	}


	/// Check if a property matches a name (generic or reported) and if it does,
	/// set a description on it.
	inline bool auto_set(StorageProperty& p, const std::string& name, const char* descr)
	{
		if (name_match(p, name)) {
			p.set_description(descr);
			return true;
		}
		return false;
	}

}



bool storage_property_autoset_description(StorageProperty& p, StorageDeviceDetectedType device_type)
{
	bool found = false;


	// checksum errors first
	if (p.generic_name.find("_text_only/_checksum_error") != std::string::npos) {
		p.set_description("Checksum errors indicate that SMART data is invalid. This shouldn't happen in normal circumstances.");
		found = true;

	// Section Info
	} else {
		switch (p.section) {
			case StoragePropertySection::Info:
				found = auto_set(p, "model_family", "Model family (from smartctl database)")
				|| auto_set(p, "model_name", "Device model")
				|| auto_set(p, "serial_number", "Serial number, unique to each physical drive")
				|| auto_set(p, "user_capacity/bytes/_short", "User-serviceable drive capacity as reported to an operating system")
				|| auto_set(p, "user_capacity/bytes", "User-serviceable drive capacity as reported to an operating system")
				|| auto_set(p, "in_smartctl_database", "Whether the device is in smartctl database or not. "
						"If it is, additional information may be provided; otherwise, Raw values of some attributes may be incorrectly formatted.")
				|| auto_set(p, "smart_support/available", "Whether the device supports SMART. If not, then only very limited information will be available.")
				|| auto_set(p, "smart_support/enabled", "Whether the device has SMART enabled. If not, most of the reported values will be incorrect.")
				|| auto_set(p, "ata_aam/enabled", "Automatic Acoustic Management (AAM) feature")
				|| auto_set(p, "ata_aam/level", "Automatic Acoustic Management (AAM) level")
				|| auto_set(p, "ata_apm/enabled", "Automatic Power Management (APM) feature")
				|| auto_set(p, "ata_apm/level", "Advanced Power Management (APM) level")
				|| auto_set(p, "ata_dsn/enabled", "Device Statistics Notification (DSN) feature")
				|| auto_set(p, "power_mode", "Power mode at the time of query");

				// set just its name as a tooltip
				if (!found) {
					p.set_description(p.displayable_name);
					found = true;
				}
				break;

			case StoragePropertySection::OverallHealth:
				found = auto_set(p, "smart_status/passed", "Overall health self-assessment test result. Note: If the drive passes this test, it doesn't mean it's OK. "
						"However, if the drive doesn't pass it, then it's either already dead, or it's predicting its own failure within the next 24 hours. In this case do a backup immediately!");
				break;

			case StoragePropertySection::Capabilities:
				found = auto_set(p, "ata_smart_data/offline_data_collection/status/_group", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
						"This should show if Automatic Offline Data Collection is enabled.")
				|| auto_set(p, "ata_smart_data/offline_data_collection/completion_seconds", "Offline Data Collection (a.k.a. Offline test) is usually automatically performed when the device is idle or every fixed amount of time. "
						"This value shows the estimated time required to perform this operation in idle conditions. A value of 0 means unsupported.")
				|| auto_set(p, "ata_smart_data/self_test/polling_minutes/short", "This value shows the estimated time required to perform a short self-test in idle conditions. A value of 0 means unsupported.")
				|| auto_set(p, "ata_smart_data/self_test/polling_minutes/extended", "This value shows the estimated time required to perform a long self-test in idle conditions. A value of 0 means unsupported.")
				|| auto_set(p, "ata_smart_data/self_test/polling_minutes/conveyance", "This value shows the estimated time required to perform a conveyance self-test in idle conditions. "
						"A value of 0 means unsupported.")
				|| auto_set(p, "ata_smart_data/self_test/status/_group", "Status of the last self-test run.")
				|| auto_set(p, "ata_smart_data/offline_data_collection/_group", "Drive properties related to Offline Data Collection and self-tests.")
				|| auto_set(p, "ata_smart_data/capabilities/_group", "Drive properties related to SMART handling.")
				|| auto_set(p, "ata_smart_data/capabilities/error_logging_supported/_group", "Drive properties related to error logging.")
				|| auto_set(p, "ata_sct_capabilities/_group", "Drive properties related to temperature information.");
				break;

			case StoragePropertySection::AtaAttributes:
				found = auto_set(p, "ata_smart_attributes/revision", p.displayable_name.c_str());
				if (!found) {
					auto_set_ata_attribute_description(p, device_type);
					found = true;  // true, because auto_set_attr() may set "Unknown attribute", which is still "found".
				}
				break;

			case StoragePropertySection::Statistics:
				found = auto_set_ata_statistic_description(p);
				break;

			case StoragePropertySection::AtaErrorLog:
				found = auto_set(p, "ata_smart_error_log/extended/revision", p.displayable_name.c_str())
				|| auto_set(p, "ata_smart_error_log/extended/count", "Number of errors in error log. Note: Some manufacturers may list completely harmless errors in this log "
					"(e.g., command invalid, not implemented, etc.).");
// 				|| auto_set(p, "error_log_unsupported", "This device does not support error logging.");  // the property text already says that
				if (p.is_value_type<AtaStorageErrorBlock>()) {
					if (!p.get_value<AtaStorageErrorBlock>().reported_types.empty()) {  // Text parser only
						p.set_description(AtaStorageErrorBlock::format_readable_error_types(
								p.get_value<AtaStorageErrorBlock>().reported_types));
					}
					/// TODO JSON parser
					found = true;
				}
				break;

			case StoragePropertySection::SelftestLog:
				found = auto_set(p, "ata_smart_self_test_log/extended/revision", p.displayable_name.c_str())
				|| auto_set(p, "ata_smart_self_test_log/standard/revision", p.displayable_name.c_str())
				|| auto_set(p, "ata_smart_self_test_log/extended/count", "Number of tests in selftest log. Note: The number of entries may be limited to the newest manual tests.")
				|| auto_set(p, "ata_smart_self_test_log/standard/count", "Number of tests in selftest log. Note: The number of entries may be limited to the newest manual tests.");
		// 		|| auto_set(p, "ata_smart_self_test_log/_present", "This device does not support self-test logging.");  // the property text already says that
				break;

			case StoragePropertySection::SelectiveSelftestLog:
				// nothing here
				break;

			case StoragePropertySection::TemperatureLog:
				found = auto_set(p, "_text_only/ata_sct_status/_not_present", "SCT support is needed for SCT temperature logging.");
				break;

			case StoragePropertySection::NvmeHealth:
			case StoragePropertySection::NvmeErrorLog:
				break;

			case StoragePropertySection::NvmeAttributes:
				found = auto_set_nvme_attribute_description(p);
				break;

			case StoragePropertySection::ErcLog:
			case StoragePropertySection::PhyLog:
			case StoragePropertySection::DirectoryLog:
			case StoragePropertySection::Unknown:
				// nothing
				break;
		}
	}

	return found;
}




void storage_property_autoset_warning(StorageProperty& p)
{
	std::optional<WarningLevel> w;
	std::string reason;

	// checksum errors first
	if (p.generic_name.find("_text_only/_checksum_error") != std::string::npos) {
		w = WarningLevel::Warning;
		reason = "The drive may have a broken implementation of SMART, or it's failing.";


	// Section Info
	} else {
		switch (p.section) {
			case StoragePropertySection::Info:
				if (name_match(p, "smart_support/available") && !p.get_value<bool>()) {
					w = WarningLevel::Notice;
					reason = "SMART is not supported. You won't be able to read any SMART information from this drive.";

				} else if (name_match(p, "smart_support/enabled") && !p.get_value<bool>()) {
					w = WarningLevel::Notice;
					reason = "SMART is disabled. You should enable it to read any SMART information from this drive. "
							 "Additionally, some drives do not log useful data with SMART disabled, so it's advisable to keep it always enabled.";

				} else if (name_match(p, "_text_only/info_warning")) {
					w = WarningLevel::Notice;
					reason = "Your drive may be affected by the warning, please see the details.";
				}
				break;

			case StoragePropertySection::OverallHealth:
				if (name_match(p, "smart_status/passed") && !p.get_value<bool>()) {
					w = WarningLevel::Alert;
					reason = "The drive is reporting that it will FAIL very soon. Please back up as soon as possible!";
				}
				break;

			case StoragePropertySection::Capabilities:
				// nothing
				break;

			case StoragePropertySection::AtaAttributes:
			{
				storage_property_ata_attribute_autoset_warning(p);
				break;
			}

			case StoragePropertySection::Statistics:
			{
				storage_property_ata_statistic_autoset_warning(p);
				break;
			}

			case StoragePropertySection::AtaErrorLog:
			{
				// Note: The error list table doesn't display any descriptions, so if any
				// error-entry related descriptions are added here, don't forget to enable
				// the tooltips.

				if (name_match(p, "ata_smart_error_log/extended/count") && p.get_value<int64_t>() > 0) {
					w = WarningLevel::Notice;
					reason = "The drive is reporting internal errors. Usually this means uncorrectable data loss and similar severe errors. "
							"Check the actual errors for details.";

				} else if (name_match(p, "_text_only/ata_smart_error_log/_not_present")) {
					w = WarningLevel::Notice;
					reason = "The drive does not support error logging. This means that SMART error history is unavailable.";
				}

				// Rate individual error log entries.
				if (p.is_value_type<AtaStorageErrorBlock>()) {
					const auto& eb = p.get_value<AtaStorageErrorBlock>();
					if (!eb.reported_types.empty()) {
						WarningLevel error_block_warning = WarningLevel::None;
						for (const auto& reported_type : eb.reported_types) {
							const WarningLevel individual_warning = AtaStorageErrorBlock::get_warning_level_for_error_type(reported_type);
							if (individual_warning > error_block_warning) {
								error_block_warning = WarningLevel(individual_warning);
							}
						}
						if (error_block_warning > WarningLevel::None) {
							w = error_block_warning;
							reason = "The drive is reporting internal errors. Your data may be at risk depending on error severity.";
						}
					}
				}

				break;
			}

			case StoragePropertySection::SelftestLog:
			{
				// Note: The error list table doesn't display any descriptions, so if any
				// error-entry related descriptions are added here, don't forget to enable
				// the tooltips.

				// Don't include selftest warnings - they may be old or something.
				// Self-tests are carried manually anyway, so the user is expected to check their status anyway.

				if (name_match(p, "ata_smart_self_test_log/_present")) {
					w = WarningLevel::Notice;
					reason = "The drive does not support self-test logging. This means that SMART test results won't be logged.";
				}
				break;
			}

			case StoragePropertySection::SelectiveSelftestLog:
				// nothing here
				break;

			case StoragePropertySection::TemperatureLog:
				// Don't highlight SCT Unsupported as warning, it's harmless.
// 				if (name_match(p, "_text_only/ata_sct_status/_not_present") && p.value_bool) {
// 					w = WarningLevel::notice;
// 					reason = "The drive does not support SCT Temperature logging.";
// 				}
				// Current temperature
				if (name_match(p, "ata_sct_status/temperature/current") && p.get_value<int64_t>() > 50) {  // 50C
					w = WarningLevel::Notice;
					reason = "The temperature of the drive is higher than 50 degrees Celsius. "
							"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";
				}
				break;

			case StoragePropertySection::NvmeHealth:
			case StoragePropertySection::NvmeErrorLog:
				break;

			case StoragePropertySection::NvmeAttributes:
			{
				storage_property_nvme_attribute_autoset_warning(p);
				break;
			}

			case StoragePropertySection::ErcLog:
			case StoragePropertySection::PhyLog:
			case StoragePropertySection::DirectoryLog:
			case StoragePropertySection::Unknown:
//			case AtaStoragePropertySection::Internal:
				// nothing here
				break;
		}
	}

	if (w.has_value()) {
		p.warning_level = w.value();
		p.warning_reason = reason;
	}
}



/// Append warning text to description and set it on the property
inline void storage_property_autoset_warning_descr(StorageProperty& p)
{
	std::string reason = storage_property_get_warning_reason(p);
	if (!reason.empty()) {
		p.set_description(p.get_description() + "\n\n" + reason);
	}
}



StoragePropertyRepository StoragePropertyProcessor::process_properties(
		StoragePropertyRepository properties, StorageDeviceDetectedType device_type)
{
	for (auto& p : properties.get_properties_ref()) {
		storage_property_autoset_description(p, device_type);
		storage_property_autoset_warning(p);
		storage_property_autoset_warning_descr(p);  // append warning to description
	}
	return properties;
}



/// @}
