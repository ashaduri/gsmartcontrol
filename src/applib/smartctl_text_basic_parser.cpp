/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "smartctl_text_basic_parser.h"

// #include <glibmm.h>
//#include <cstdint>
//#include <utility>
#include <cstdint>
#include <string>
#include <string_view>

// #include "hz/locale_tools.h"  // ScopedCLocale, locale_c_get().
#include "storage_device_detected_type.h"
#include "storage_property.h"
#include "hz/string_algo.h"  // string_*
#include "hz/string_num.h"  // string_is_numeric, number_to_string
//#include "hz/debug.h"  // debug_*

#include "app_regex.h"
//#include "ata_storage_property_descr.h"
// #include "warning_colors.h"
#include "smartctl_parser_types.h"
#include "smartctl_version_parser.h"
#include "smartctl_text_parser_helper.h"



hz::ExpectedVoid<SmartctlParserError> SmartctlTextBasicParser::parse(std::string_view smartctl_output)
{
	// perform any2unix
	const std::string output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_output));

	if (output.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Empty string passed as an argument. Returning.\n");
		return hz::Unexpected(SmartctlParserError::EmptyInput, "Smartctl data is empty.");
	}

	// Version
	std::string version, version_full;
	if (!SmartctlVersionParser::parse_version_text(output, version, version_full)) {  // is this smartctl data at all?
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot extract version information. Returning.\n");
		return hz::Unexpected(SmartctlParserError::NoVersion, "Cannot extract smartctl version information.");
	}
	{
		StorageProperty p;
		p.set_name("smartctl/version/_merged", "Smartctl Version");
		p.reported_value = version;
		p.value = p.reported_value;  // string-type value
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}
	{
		StorageProperty p;
		p.set_name("smartctl/version/_merged_full", "Smartctl Version");
		p.reported_value = version_full;
		p.value = p.reported_value;  // string-type value
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}

	// Full text output
	{
		StorageProperty p;
		p.set_name("smartctl/output", "Smartctl Text Output");
		p.reported_value = output;
		p.value = p.reported_value;  // string-type value
		p.show_in_ui = false;
		add_property(p);
	}

	bool is_raid = false;

	// Detect type. note: we can't distinguish between sata and scsi (on linux, for -d ata switch).
	// Sample output line 1 (encountered on a CDRW drive):
	// SMART support is: Unavailable - Packet Interface Devices [this device: CD/DVD] don't support ATA SMART
	// Sample output line 2 (encountered on a BDRW drive):
	// Device type:          CD/DVD
	// NOTE: CD/DVD detection does not work in "-d scsi" mode.
	if (app_regex_partial_match("/this device: CD\\/DVD/mi", output)
			|| app_regex_partial_match("/^Device type:\\s+CD\\/DVD/mi", output)) {
		StorageProperty p;
		p.set_name("_text_only/custom/parser_detected_drive_type", "Parser-Detected Drive Type");
		p.reported_value = "CD/DVD";
		p.value = StorageDeviceDetectedTypeExt::get_storable_name(StorageDeviceDetectedType::CdDvd);
		p.readable_value = StorageDeviceDetectedTypeExt::get_displayable_name(StorageDeviceDetectedType::CdDvd);
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);

	// This was encountered on a csmi soft-raid under windows with pd0.
	// The device reported that it had smart supported and enabled.
	// Product:              Raid 5 Volume
	} else if (app_regex_partial_match("/Product:[ \\t]*Raid/mi", output)) {
		StorageProperty p;
		p.set_name("_text_only/custom/parser_detected_drive_type", "Parser-Detected Drive Type");
		p.reported_value = "RAID";
		p.value = StorageDeviceDetectedTypeExt::get_storable_name(StorageDeviceDetectedType::UnsupportedRaid);
		p.readable_value = StorageDeviceDetectedTypeExt::get_displayable_name(StorageDeviceDetectedType::UnsupportedRaid);
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);

		is_raid = true;

	} else if (app_regex_partial_match("/ATA Version is:/mi", output)) {
		StorageProperty p;
		p.set_name("_text_only/custom/parser_detected_drive_type", "Parser-Detected Drive Type");
		p.reported_value = "(S)ATA";
		p.value = StorageDeviceDetectedTypeExt::get_storable_name(StorageDeviceDetectedType::AtaAny);
		p.readable_value = StorageDeviceDetectedTypeExt::get_displayable_name(StorageDeviceDetectedType::AtaAny);
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}

	bool smart_supported = true;
	bool smart_enabled = true;

	// RAID volume may report that it has SMART, but it obviously doesn't.
	if (is_raid) {
		smart_supported = false;
		smart_enabled = false;

	} else {
		// Note: We don't use SmartctlTextAtaParser here, because this information
		// may be in some other format. If this information is valid, only then it's
		// passed to SmartctlTextAtaParser.
		// Compared to SmartctlTextAtaParser, this one is much looser.

		// Don't put complete messages here - they change across smartctl versions.
		if (app_regex_partial_match("/^SMART support is:[ \\t]*Unavailable/mi", output)  // cdroms output this
				|| app_regex_partial_match("/Device does not support SMART/mi", output)  // usb flash drives, non-smart hds
				|| app_regex_partial_match("/Device Read Identity Failed/mi", output)) {  // solaris scsi, unsupported by smartctl (maybe others?)
			smart_supported = false;
			smart_enabled = false;

		} else if (app_regex_partial_match("/^SMART support is:[ \\t]*Available/mi", output)
				|| app_regex_partial_match("/^SMART support is:[ \\t]*Ambiguous/mi", output)) {
			smart_supported = true;

			if (app_regex_partial_match("/^SMART support is:[ \\t]*Enabled/mi", output)) {
				smart_enabled = true;
			} else if (app_regex_partial_match("/^SMART support is:[ \\t]*Disabled/mi", output)) {
				smart_enabled = false;
			}
		}
	}

	{
		StorageProperty p;
		p.set_name("smart_support/available", "SMART Supported");
		p.value = smart_supported;
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}
	{
		StorageProperty p;
		p.set_name("smart_support/enabled", "SMART Enabled");
		p.value = smart_enabled;
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}


	std::string model;
	if (app_regex_partial_match("/^Device Model:[ \\t]*(.*)$/mi", output, &model)) {  // HDDs and CDROMs
		model = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(model), ' ');
		StorageProperty p;
		p.set_name("model_name", "Device Model");
		p.value = model;  // string-type value
		add_property(p);

	} else if (app_regex_partial_match("/^(?:Device|Product):[ \\t]*(.*)$/mi", output, &model)) {  // usb flash drives
		model = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(model), ' ');
		StorageProperty p;
		p.set_name("model_name", "Device Model");
		p.value = model;
		add_property(p);
	}


	std::string family;  // this is from smartctl's database
	if (app_regex_partial_match("/^Model Family:[ \\t]*(.*)$/mi", output, &family)) {
		family = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(family), ' ');
		StorageProperty p;
		p.set_name("model_family", "Model Family");
		p.value = family;
		add_property(p);
	}

	std::string serial;
	if (app_regex_partial_match("/^Serial Number:[ \\t]*(.*)$/mi", output, &serial)) {
		serial = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(serial), ' ');
		StorageProperty p;
		p.set_name("serial_number", "Serial Number");
		p.value = serial;
		add_property(p);
	}

	std::string rpm_str;
	if (app_regex_partial_match("/^Rotation Rate:[ \\t]*(.*)$/mi", output, &rpm_str)) {
		StorageProperty p;
		p.set_name("rotation_rate", "Rotation Rate");
		p.reported_value = rpm_str;
		p.value = hz::string_to_number_nolocale<int>(rpm_str, false);
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}


	// Note: this property is present since 5.33.
	std::string size;
	if (app_regex_partial_match("/^User Capacity:[ \\t]*(.*)$/mi", output, &size)) {
		int64_t bytes = 0;
		const std::string readable_size = SmartctlTextParserHelper::parse_byte_size(size, bytes, false);
		StorageProperty p;
		p.set_name("user_capacity/bytes/_short", "Capacity");
		p.reported_value = size;
		p.value = bytes;
		p.readable_value = readable_size;
		p.section = StoragePropertySection::Info;  // add to info section
		add_property(p);
	}


	return {};
}






/// @}
