/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2022 - 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "smartctl_json_ata_parser.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <chrono>

#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include "storage_property.h"
#include "hz/debug.h"
#include "hz/string_algo.h"
// #include "smartctl_version_parser.h"
#include "hz/format_unit.h"
#include "hz/error_container.h"
#include "hz/string_num.h"
#include "smartctl_json_parser_helpers.h"
#include "smartctl_parser_types.h"


/*
Information not printed in JSON yet:

- Checksum warnings (smartctl.cpp: checksumwarning()).
	Smartctl output: Warning! SMART <section name> Structure error: invalid SMART checksum
 	Keys:
 		_text_only/attribute_data_checksum_error
 		_text_only/attribute_thresholds_checksum_error
 		_text_only/ata_error_log_checksum_error
 		_text_only/selftest_log_checksum_error

- Samsung warning
 	Smartctl output: May need -F samsung or -F samsung2 enabled; see manual for details
 	We ignore this in text parser.

- Warnings from drivedb.h in the middle of Info section
	Smartctl output (example):
		WARNING: A firmware update for this drive may be available,
		see the following Seagate web pages:
		...
	Keys: _text_only/info_warning

- Errors about consistency:
	"Invalid Error Log index ..."
	"Warning: ATA error count %d inconsistent with error log pointer"
	We ignore this in text parser.

- "mandatory SMART command failed" and similar errors.
	We ignore this in text parser.

- SMART support and some other Info keys
	_text_only/write_cache_reorder
	_text_only/power_mode

 - Directory log supported
 	We don't use this.
 	_text_only/directory_log_supported

*/




hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse(std::string_view smartctl_output)
{
	if (hz::string_trim_copy(smartctl_output).empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Empty string passed as an argument. Returning.\n");
		return hz::Unexpected(SmartctlParserError::EmptyInput, "Smartctl data is empty.");
	}

	nlohmann::json json_root_node;
	try {
		json_root_node = nlohmann::json::parse(smartctl_output);
	} catch (const nlohmann::json::parse_error& e) {
		debug_out_warn("app", DBG_FUNC_MSG << "Error parsing smartctl output as JSON: " << e.what() << "\n");
		return hz::Unexpected(SmartctlParserError::SyntaxError, std::string("Invalid JSON data: ") + e.what());
	}

	StorageProperty merged_property, full_property;
	auto version_parse_status = SmartctlJsonParserHelpers::parse_version(json_root_node, merged_property, full_property);
	if (!version_parse_status) {
		return version_parse_status;
	}
	add_property(merged_property);
	add_property(full_property);

	// Info must be supported.
	auto info_parse_status = parse_section_info(json_root_node);
	if (!info_parse_status) {
		return info_parse_status;
	}

	// Add properties for each parsed section so that the UI knows which tabs to show or hide
	{
		auto section_parse_status = parse_section_health(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::Health;
//		p.set_name("_parser/health_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_capabilities(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::Capabilities;
//		p.set_name("_parser/capabilities_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_attributes(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::Attributes;
//		p.set_name("_parser/attributes_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_directory_log(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::DirectoryLog;
//		p.set_name("_parser/directory_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_error_log(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::ErrorLog;
//		p.set_name("_parser/error_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_selftest_log(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::SelftestLog;
//		p.set_name("_parser/selftest_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_selective_selftest_log(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::SelectiveSelftestLog;
//		p.set_name("_parser/selective_selftest_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_scttemp_log(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::TemperatureLog;
//		p.set_name("_parser/temperature_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_scterc_log(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::ErcLog;
//		p.set_name("_parser/erc_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_devstat(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::Devstat;
//		p.set_name("_parser/devstat_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_sataphy(json_root_node);
//		StorageProperty p;
//		p.section = StoragePropertySection::PhyLog;
//		p.set_name("_parser/phy_log_section_available");
//		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_info(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	// This is very similar to Basic Parser, but the Basic Parser supports different drive types, while this
	// one is only for ATA.

	static const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> json_keys = {

			{"device/type", _("Smartctl Device Type"),  // nvme, sat, etc.
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					if (auto jval = get_node_data<std::string>(root_node, "device/type"); jval.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = jval.value();
						p.show_in_ui = false;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"device/protocol", _("Smartctl Device Protocol"),  // NVMe, ...
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					if (auto jval = get_node_data<std::string>(root_node, "device/protocol"); jval.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = jval.value();
						p.show_in_ui = false;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"model_family", _("Model Family"), string_formatter()},
			{"model_name", _("Device Model"), string_formatter()},
			{"serial_number", _("Serial Number"), string_formatter()},

			{"wwn/_merged", _("World Wide Name"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto jval1 = get_node_data<int64_t>(root_node, "wwn/naa");
					auto jval2 = get_node_data<int64_t>(root_node, "wwn/oui");
					auto jval3 = get_node_data<int64_t>(root_node, "wwn/id");

					if (jval1 && jval2 && jval3) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						// p.readable_value = fmt::format("{:X} {:X} {:X}", jval1.value(), jval2.value(), jval3.value());
						p.readable_value = fmt::format("{:X}-{:06X}-{:08X}", jval1.value(), jval2.value(), jval3.value());
						p.value = p.readable_value;  // string-type value
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"firmware_version", _("Firmware Version"), string_formatter()},

			{"user_capacity/bytes", _("Capacity"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return fmt::format("{} [{}; {} bytes]",
						hz::format_size(static_cast<uint64_t>(value), true),
						hz::format_size(static_cast<uint64_t>(value), false),
						hz::number_to_string_locale(value));
				})
			},

			{"user_capacity/bytes/_short", _("Capacity"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					if (auto jval = get_node_data<int64_t>(root_node, "user_capacity/bytes"); jval) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.readable_value = hz::format_size(static_cast<uint64_t>(jval.value()), true);
						p.value = jval.value();
						p.show_in_ui = false;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", "user_capacity/bytes"));
				}
			},

			{"physical_block_size/_and/logical_block_size", _("Sector Size"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					std::vector<std::string> values;
					if (auto jval1 = get_node_data<int64_t>(root_node, "logical_block_size"); jval1) {
						values.emplace_back(fmt::format("{} bytes logical", jval1.value()));
					}
					if (auto jval2 = get_node_data<int64_t>(root_node, "physical_block_size"); jval2) {
						values.emplace_back(fmt::format("{} bytes physical", jval2.value()));
					}
					if (!values.empty()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.readable_value = hz::string_join(values, ", ");
						p.value = p.readable_value;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			// (S)ATA, used to detect HDD vs SSD
			{"rotation_rate", _("Rotation Rate"), integer_formatter<int64_t>("{} RPM")},

			{"form_factor/name", _("Form Factor"), string_formatter()},
			{"trim/supported", _("TRIM Supported"), bool_formatter(_("Yes"), _("No"))},
			{"in_smartctl_database", _("In Smartctl Database"), bool_formatter(_("Yes"), _("No"))},
			{"smartctl/drive_database_version/string", _("Smartctl Database Version"), string_formatter()},
			{"ata_version/string", _("ATA Version"), string_formatter()},
			{"sata_version/string", _("SATA Version"), string_formatter()},

			{"interface_speed/_merged", _("Interface Speed"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					std::vector<std::string> values;
					if (auto jval1 = get_node_data<std::string>(root_node, "interface_speed/max/string"); jval1) {
						values.emplace_back(fmt::format("Max: {}", jval1.value()));
					}
					if (auto jval2 = get_node_data<std::string>(root_node, "interface_speed/current/string"); jval2) {
						values.emplace_back(fmt::format("Current: {}", jval2.value()));
					}
					if (!values.empty()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.readable_value = hz::string_join(values, ", ");
						p.value = p.readable_value;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"local_time/asctime", _("Scanned on"), string_formatter()},

			{"smart_support/available", _("SMART Supported"), bool_formatter(_("Yes"), _("No"))},
			{"smart_support/enabled", _("SMART Enabled"), bool_formatter(_("Yes"), _("No"))},

			{"ata_aam/enabled", _("AAM Feature"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"ata_aam/level", _("AAM Level"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					if (auto level_result = get_node_data<int64_t>(root_node, "ata_aam/level"); level_result.has_value()) {
						std::string level_string = get_node_data<std::string>(root_node, "ata_aam/string").value_or("");
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.readable_value = fmt::format("{} ({})", level_string, level_result.value());
						p.value = level_result.value();
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},
			{"ata_aam/recommended_level", _("AAM Recommended Level"), integer_formatter<int64_t>()},

			{"ata_apm/enabled", _("APM Feature"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"ata_apm/level", _("APM Level"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					if (auto level_result = get_node_data<int64_t>(root_node, "ata_apm/level"); level_result.has_value()) {
						std::string level_string = get_node_data<std::string>(root_node, "ata_apm/string").value_or("");
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.readable_value = fmt::format("{} ({})", level_string, level_result.value());
						p.value = level_result.value();
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"read_lookahead/enabled", _("Read Look-Ahead"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"write_cache/enabled", _("Write Cache"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"ata_dsn/enabled", _("DSN Feature"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"ata_security/string", _("ATA Security"), string_formatter()},

			// Protocol-independent JSON-only values
			{"power_cycle_count", _("Number of Power Cycles"), integer_formatter<int64_t>()},
			{"power_on_time/hours", _("Powered for"), integer_formatter<int64_t>("{} hours")},
			{"temperature/current", _("Current Temperature"), integer_formatter<int64_t>("{}° Celsius")},
	};

	bool any_found = false;
	for (const auto& [key, displayable_name, retrieval_func] : json_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StoragePropertySection::Info;
			add_property(p.value());
			any_found = true;
		}
	}

	if (!any_found) {
		return hz::Unexpected(SmartctlParserError::KeyNotFound, "No keys info found in JSON data.");
	}
	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_health(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> health_keys = {
			{"smart_status/passed", _("Overall Health Self-Assessment Test"), bool_formatter(_("PASSED"), _("FAILED"))},
	};

	for (const auto& [key, displayable_name, retrieval_func] : health_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StoragePropertySection::OverallHealth;
			add_property(p.value());
		}
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_capabilities(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	static const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> json_keys = {
//			// FIXME Remove?
//			{"ata_smart_data/capabilities/_group", _("SMART Capabilities"),
//			 		conditional_formatter("ata_smart_data/capabilities/values",
//					AtaStorageProperty{} )},
//
//			// FIXME Remove?
//			{"ata_smart_data/capabilities/error_logging_supported/_group", _("Error Logging Capabilities"),
//			 		conditional_formatter("ata_smart_data/capabilities/error_logging_supported",
//					AtaStorageProperty{} )},
//

			{"ata_smart_data/offline_data_collection/status/_auto_enabled", _("Automatic offline data collection status"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, "ata_smart_data/offline_data_collection/status/value");
					if (value_val.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = static_cast<bool>(value_val.value() & 0x80);  // taken from ataprint.cpp
						p.readable_value = p.get_value<bool>() ? _("Enabled") : _("Disabled");
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},


			// Last self-test status
			{"ata_smart_data/offline_data_collection/status/value/_decoded", "Last offline data collection status",
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<uint8_t>(root_node, "ata_smart_data/offline_data_collection/status/value");
					if (value_val.has_value()) {
						std::string status_str;
						switch (value_val.value() & 0x7f) {
							// Data from smartmontools/ataprint.cpp
							case 0x00: status_str = _("Never started"); break;
							case 0x02: status_str = _("Completed without error"); break;
							case 0x03: status_str = (value_val.value() == 0x03 ? _("In progress") : _("In reserved state")); break;
							case 0x04: status_str = _("Suspended by an interrupting command from host"); break;
							case 0x05: status_str = _("Aborted by an interrupting command from host"); break;
							case 0x06: status_str = _("Aborted by the device with a fatal error"); break;
							default: status_str = ((value_val.value() & 0x7f) > 0x40 ? _("In vendor-specific state") : _("In reserved state")); break;
						}
						StorageProperty p;
						p.section = StoragePropertySection::Capabilities;
						p.set_name(key, displayable_name);
						p.value = status_str;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/offline_data_collection/completion_seconds", _("Time to complete offline data collection"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = std::chrono::seconds(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/status/_merged", _("Self-test execution status"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					// Testing:
					// "status": {
					//   "value": 249,
					//   "string": "in progress, 90% remaining",
					//   "remaining_percent": 90
					// },

					// Not testing:
					// "status": {
					//   "value": 0,
					//   "string": "completed without error",
					//   "passed": true
					// },

					AtaStorageSelftestEntry::Status status = AtaStorageSelftestEntry::Status::Unknown;

					auto value_val = get_node_data<uint8_t>(root_node, "ata_smart_data/self_test/status/value");
					if (value_val.has_value()) {
						switch (value_val.value() >> 4) {
							// Data from smartmontools/ataprint.cpp
							case 0x0: status = AtaStorageSelftestEntry::Status::CompletedNoError; break;
							case 0x1: status = AtaStorageSelftestEntry::Status::AbortedByHost; break;
							case 0x2: status = AtaStorageSelftestEntry::Status::Interrupted; break;
							case 0x3: status = AtaStorageSelftestEntry::Status::FatalOrUnknown; break;
							case 0x4: status = AtaStorageSelftestEntry::Status::ComplUnknownFailure; break;
							case 0x5: status = AtaStorageSelftestEntry::Status::ComplElectricalFailure; break;
							case 0x6: status = AtaStorageSelftestEntry::Status::ComplServoFailure; break;
							case 0x7: status = AtaStorageSelftestEntry::Status::ComplReadFailure; break;
							case 0x8: status = AtaStorageSelftestEntry::Status::ComplHandlingDamage; break;
							// Special case
							case 0xf: status = AtaStorageSelftestEntry::Status::InProgress; break;
							default: status = AtaStorageSelftestEntry::Status::Reserved; break;
						}

						AtaStorageSelftestEntry sse;
						sse.test_num = 0;  // capability uses 0
						sse.status_str = AtaStorageSelftestEntry::get_readable_status_name(status);
						sse.status = status;

						sse.remaining_percent = -1;  // unknown or n/a
						// Present only when extended self-test log is supported
						if (auto remaining_percent_val = get_node_data<int8_t>(root_node, "ata_smart_data/self_test/status/remaining_percent"); remaining_percent_val.has_value()) {
							sse.remaining_percent = remaining_percent_val.value();
						}

						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = sse;
						return p;
					}

					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			// Present only when extended self-test log is supported
			{"ata_smart_data/self_test/status/remaining_percent", _("Self-test remaining percentage"), integer_formatter<int64_t>("{} %")},

			{"ata_smart_data/capabilities/self_tests_supported", _("Self-tests supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_smart_data/capabilities/exec_offline_immediate_supported", _("Offline immediate test supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/offline_is_aborted_upon_new_cmd", _("Abort offline collection on new command"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/offline_surface_scan_supported", _("Offline surface scan supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_smart_data/capabilities/conveyance_self_test_supported", _("Conveyance self-test supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/selective_self_test_supported", _("Selective self-test supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_smart_data/self_test/polling_minutes/short", _("Short self-test status recommended polling time"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = std::chrono::minutes(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/polling_minutes/extended", _("Extended self-test status recommended polling time"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = std::chrono::minutes(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/polling_minutes/conveyance", _("Conveyance self-test status recommended polling time"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = std::chrono::minutes(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},


			{"ata_smart_data/capabilities/attribute_autosave_enabled", _("Saves SMART data before entering power-saving mode"), bool_formatter(_("Enabled"), _("Disabled"))},

			{"ata_smart_data/capabilities/error_logging_supported", _("Error logging supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/gp_logging_supported", _("General purpose logging supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_sct_capabilities/_supported", _("SCT capabilities supported"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					if (auto value_val = get_node_exists(root_node, "ata_sct_capabilities"); value_val.has_value()) {
						StorageProperty p;
						p.set_name(key, displayable_name);
						p.value = value_val.value();
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, fmt::format("Error getting key {} from JSON data.", key));
				}
			},
			{"ata_sct_capabilities/error_recovery_control_supported", _("SCT error recovery control supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_sct_capabilities/feature_control_supported", _("SCT feature control supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_sct_capabilities/data_table_supported", _("SCT data table supported"), bool_formatter(_("Yes"), _("No"))},

	};

	bool section_properties_found = false;

	for (const auto& [key, displayable_name, retrieval_func] : json_keys) {
		DBG_ASSERT(retrieval_func != nullptr);
		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StoragePropertySection::Capabilities;
			add_property(p.value());
			section_properties_found = true;
		}
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::Capabilities)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_attributes(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	// Revision
	if (get_node_exists(json_root_node, "ata_smart_attributes/revision").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_smart_attributes/revision", _("Data structure revision number"));
		p.section = StoragePropertySection::AtaAttributes;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_attributes/revision").value_or(0);
		add_property(p);
		section_properties_found = true;
	}

	const std::string table_key = "ata_smart_attributes/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		for (const auto& table_entry : table_node.value()) {
			AtaStorageAttribute a;

			a.id = get_node_data<int32_t>(table_entry, "id").value_or(0);
			a.flag = get_node_data<std::string>(table_entry, "flags/string").value_or("");
			a.value = (get_node_exists(table_entry, "value").value_or(false) ? std::optional<uint8_t>(get_node_data<uint8_t>(table_entry, "value").value_or(0)) : std::nullopt);
			a.worst = (get_node_exists(table_entry, "worst").value_or(false) ? std::optional<uint8_t>(get_node_data<uint8_t>(table_entry, "worst").value_or(0)) : std::nullopt);
			a.threshold = (get_node_exists(table_entry, "thresh").value_or(false) ? std::optional<uint8_t>(get_node_data<uint8_t>(table_entry, "thresh").value_or(0)) : std::nullopt);
			a.attr_type = get_node_data<bool>(table_entry, "flags/prefailure").value_or(false) ? AtaStorageAttribute::AttributeType::Prefail : AtaStorageAttribute::AttributeType::OldAge;
			a.update_type = get_node_data<bool>(table_entry, "flags/updated_online").value_or(false) ? AtaStorageAttribute::UpdateType::Always : AtaStorageAttribute::UpdateType::Offline;

			const std::string when_failed = get_node_data<std::string>(table_entry, "when_failed").value_or(std::string());
			if (when_failed == "now") {
				a.when_failed = AtaStorageAttribute::FailTime::Now;
			} else if (when_failed == "past") {
				a.when_failed = AtaStorageAttribute::FailTime::Past;
			} else {  // ""
				a.when_failed = AtaStorageAttribute::FailTime::None;
			}

			a.raw_value = get_node_data<std::string>(table_entry, "raw/string").value_or(std::string());
			a.raw_value_int = get_node_data<int64_t>(table_entry, "raw/value").value_or(0);

			std::string reported_name = get_node_data<std::string>(table_entry, "name").value_or(std::string());

			StorageProperty p;
			p.set_name(reported_name, reported_name, reported_name);  // The description database will correct this.
			p.section = StoragePropertySection::AtaAttributes;
			p.value = a;
			add_property(p);

			section_properties_found = true;
		}
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::AtaAttributes)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_directory_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;
	using namespace std::string_literals;

	bool section_properties_found = false;

	std::vector<std::string> lines;

	if (get_node_exists(json_root_node, "ata_log_directory/gp_dir_version").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_log_directory/gp_dir_version", _("General purpose log directory version"));
		p.section = StoragePropertySection::DirectoryLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_log_directory/gp_dir_version").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("General Purpose Log Directory Version: {}", p.get_value<int64_t>()));
		section_properties_found = true;
	}
	if (get_node_exists(json_root_node, "ata_log_directory/smart_dir_version").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_log_directory/smart_dir_version", _("SMART log directory version"));
		p.section = StoragePropertySection::DirectoryLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_log_directory/smart_dir_version").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("SMART Log Directory Version: {}", p.get_value<int64_t>()));
		section_properties_found = true;
	}
	if (get_node_exists(json_root_node, "ata_log_directory/smart_dir_multi_sector").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_log_directory/smart_dir_multi_sector", _("Multi-sector log support"));
		p.section = StoragePropertySection::DirectoryLog;
		p.value = get_node_data<bool>(json_root_node, "ata_log_directory/smart_dir_multi_sector").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Multi-sector log support: {}", p.get_value<bool>() ? "Yes" : "No"));
		section_properties_found = true;
	}

	// Table
	const std::string table_key = "ata_log_directory/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		lines.emplace_back();

		for (const auto& table_entry : table_node.value()) {
			const uint64_t address = get_node_data<uint64_t>(table_entry, "address").value_or(0);
			const std::string name = get_node_data<std::string>(table_entry, "name").value_or(std::string());
			const bool read = get_node_data<bool>(table_entry, "read").value_or(false);
			const bool write = get_node_data<bool>(table_entry, "write").value_or(false);
			const uint64_t gp_sectors = get_node_data<uint64_t>(table_entry, "gp_sectors").value_or(0);
			const uint64_t smart_sectors = get_node_data<uint64_t>(table_entry, "smart_sectors").value_or(0);

			// Address, GPL/SL, RO/RW, Num Sectors (GPL, Smart) , Name
			// 0x00       GPL,SL  R/O      1  Log Directory
			lines.emplace_back(fmt::format(
					"0x{:02X}    GPL Sectors: {:8}    SL Sectors: {:8}    {}{}    {}",
					address,
					gp_sectors == 0 ? "-" : std::to_string(gp_sectors),
					smart_sectors == 0 ? "-" : std::to_string(smart_sectors),
					(read ? "R" : "-"),
					(write ? "W" : "-"),
					name));
		}

		// The whole section
		{
			StorageProperty p;
			p.set_name("ata_log_directory/_merged", "General Purpose Log Directory");
			p.section = StoragePropertySection::DirectoryLog;
			p.reported_value = hz::string_join(lines, "\n");
			p.value = p.reported_value;  // string-type value

			add_property(p);
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::DirectoryLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_error_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	// Revision
	if (get_node_exists(json_root_node, "ata_smart_error_log/extended/revision").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_smart_error_log/extended/revision", _("SMART extended comprehensive error log version"));
		p.section = StoragePropertySection::AtaErrorLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_error_log/extended/revision").value_or(0);
		add_property(p);
		section_properties_found = true;
	}
	// Count
	if (get_node_exists(json_root_node, "ata_smart_error_log/extended/count").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_smart_error_log/extended/count", _("ATA error count"));
		p.section = StoragePropertySection::AtaErrorLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_error_log/extended/count").value_or(0);
		add_property(p);
		section_properties_found = true;
	}

	const std::string table_key = "ata_smart_error_log/extended/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		for (const auto& table_entry : table_node.value()) {
			AtaStorageErrorBlock block;
			block.error_num = get_node_data<uint32_t>(table_entry, "error_number").value_or(0);
			block.log_index = get_node_data<uint64_t>(table_entry, "log_index").value_or(0);
			block.lifetime_hours = get_node_data<uint32_t>(table_entry, "lifetime_hours").value_or(0);
			block.device_state = get_node_data<std::string>(table_entry, "device_state/string").value_or(std::string());
			block.lba = get_node_data<uint64_t>(table_entry, "completion_registers/lba").value_or(0);
			block.type_more_info = get_node_data<std::string>(table_entry, "error_description").value_or(std::string());

			StorageProperty p;
			std::string gen_name = fmt::format("{}/{}", table_key, block.error_num);
			std::string disp_name = fmt::format("Error {}", block.error_num);
			p.set_name(gen_name, disp_name, gen_name);
			p.section = StoragePropertySection::AtaErrorLog;
			p.value = block;
			add_property(p);
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::AtaErrorLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_selftest_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	const bool extended = get_node_exists(json_root_node, "ata_smart_self_test_log/extended/revision").value_or(false);
	const std::string log_key = extended ? "ata_smart_self_test_log/extended" : "ata_smart_self_test_log/standard";

	// Revision
	if (get_node_exists(json_root_node, log_key + "/revision").value_or(false)) {
		StorageProperty p;
		p.set_name(log_key + "/revision", extended ? _("SMART extended self-test log version") : _("SMART standard self-test log version"));
		p.section = StoragePropertySection::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, log_key + "/revision").value_or(0);
		add_property(p);
		section_properties_found = true;
	}

	std::vector<std::string> counts;

	// Count
	{
		StorageProperty p;
		p.set_name(log_key + "/count", _("Self-test count"));
		p.section = StoragePropertySection::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, log_key + "/count").value_or(0);
		p.show_in_ui = false;
		add_property(p);
		counts.emplace_back(fmt::format("Self-test entries: {}", p.get_value<int64_t>()));
	}
	// Error Count
	{
		StorageProperty p;
		p.set_name(log_key + "/error_count_total", _("Total error count"));
		p.section = StoragePropertySection::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, log_key + "/error_count_total").value_or(0);
		p.show_in_ui = false;
		add_property(p);
		counts.emplace_back(fmt::format("Total error count: {}", p.get_value<int64_t>()));
	}
	// Outdated Error Count
	{
		StorageProperty p;
		p.set_name(log_key + "/error_count_outdated", _("Outdated error count"));
		p.section = StoragePropertySection::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, log_key + "/error_count_outdated").value_or(0);
		p.show_in_ui = false;
		add_property(p);
		counts.emplace_back(fmt::format("Outdated error count: {}", p.get_value<int64_t>()));
	}

	// Displayed Counts
	if (!counts.empty()) {
		StorageProperty p;
		p.set_name(log_key + "/_counts", _("Entries"));
		p.section = StoragePropertySection::SelftestLog;
		p.value = hz::string_join(counts, "; ");
		add_property(p);

		section_properties_found = true;
	}

	const std::string table_key = log_key + "/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		uint32_t entry_num = 1;
		for (const auto& table_entry : table_node.value()) {
			AtaStorageSelftestEntry entry;
			entry.test_num = entry_num;
			entry.type = get_node_data<std::string>(table_entry, "type/string").value_or(std::string());  // FIXME use type/value for i18n
			entry.status_str = get_node_data<std::string>(table_entry, "status/string").value_or(std::string());
			entry.remaining_percent = get_node_data<int8_t>(table_entry, "status/remaining_percent").value_or(-1);  // extended only
			entry.lifetime_hours = get_node_data<uint32_t>(table_entry, "lifetime_hours").value_or(0);
			entry.passed = get_node_data<bool>(table_entry, "status/passed").value_or(false);

			if (get_node_exists(table_entry, "lba").value_or(false)) {
				entry.lba_of_first_error = hz::number_to_string_locale(get_node_data<uint64_t>(table_entry, "lba").value_or(0));
			} else {
				entry.lba_of_first_error = "-";
			}

			if (get_node_exists(table_entry, "status/value").value_or(false)) {
				const uint8_t status_value = get_node_data<uint8_t>(table_entry, "status/value").value_or(0);
				entry.status = static_cast<AtaStorageSelftestEntry::Status>(status_value >> 4);
			} else {
				entry.status = AtaStorageSelftestEntry::Status::Unknown;
			}

			StorageProperty p;
			std::string gen_name = fmt::format("{}/{}", table_key, entry_num);
			std::string disp_name = fmt::format("Self-test entry {}", entry.test_num);
			p.set_name(gen_name, disp_name);
			p.section = StoragePropertySection::SelftestLog;
			p.value = entry;
			add_property(p);

			++entry_num;
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::SelftestLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_selective_selftest_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;
	using namespace std::string_literals;

	bool section_properties_found = false;

	std::vector<std::string> lines;

	if (get_node_exists(json_root_node, "ata_smart_selective_self_test_log/revision").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_smart_selective_self_test_log/revision", _("SMART Selective self-test log data structure revision number"));
		p.section = StoragePropertySection::SelectiveSelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_selective_self_test_log/revision").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("SMART Selective self-test log data structure revision number: {}", p.get_value<int64_t>()));
		section_properties_found = true;
	}
	if (get_node_exists(json_root_node, "ata_smart_selective_self_test_log/power_up_scan_resume_minutes").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_smart_selective_self_test_log/power_up_scan_resume_minutes",
				_("If Selective self-test is pending on power-up, resume delay (minutes)"));
		p.section = StoragePropertySection::SelectiveSelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_selective_self_test_log/power_up_scan_resume_minutes").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("If Selective self-test is pending on power-up, resume delay: {} minutes", p.get_value<int64_t>()));
		section_properties_found = true;
	}
	if (get_node_exists(json_root_node, "ata_smart_selective_self_test_log/flags/remainder_scan_enabled").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_smart_selective_self_test_log/flags/remainder_scan_enabled",
				_("After scanning selected spans, scan remainder of the drive"));
		p.section = StoragePropertySection::SelectiveSelftestLog;
		p.value = get_node_data<bool>(json_root_node, "ata_smart_selective_self_test_log/flags/remainder_scan_enabled").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("After scanning selected spans, scan remainder of the drive: {}", p.get_value<bool>() ? "Yes" : "No"));
		section_properties_found = true;
	}

	// Table
	const std::string table_key = "ata_smart_selective_self_test_log/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		lines.emplace_back();

		int entry_num = 1;
		for (const auto& table_entry : table_node.value()) {
			const uint64_t lba_min = get_node_data<uint64_t>(table_entry, "lba_min").value_or(0);
			const uint64_t lba_max = get_node_data<uint64_t>(table_entry, "lba_max").value_or(0);
			const std::string status_str = get_node_data<std::string>(table_entry, "status/string").value_or(std::string());

			lines.emplace_back(fmt::format(
					"Span: {:2}    Min LBA: {:020}    Max LBA: {:020}    Status: {}",
					entry_num,
					lba_min,
					lba_max,
					status_str));
			++entry_num;
		}

		// The whole section
		{
			StorageProperty p;
			p.set_name("ata_smart_selective_self_test_log/_merged", _("SMART selective self-test log"));
			p.section = StoragePropertySection::SelectiveSelftestLog;
			p.reported_value = hz::string_join(lines, "\n");
			p.value = p.reported_value;  // string-type value

			add_property(p);
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::SelectiveSelftestLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_scttemp_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;
	using namespace std::string_literals;

	bool section_properties_found = false;

	std::vector<std::string> lines;

	if (get_node_exists(json_root_node, "ata_sct_status/format_version").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/format_version", _("SCT status version"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/format_version").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("SCT status version: {}", get_node_data<int64_t>(json_root_node, "ata_sct_status/format_version").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/sct_version").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/sct_version", _("SCT format version"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/sct_version").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("SCT format version: {}", get_node_data<int64_t>(json_root_node, "ata_sct_status/sct_version").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/device_state/string").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/device_state/string", _("Device state"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<std::string>(json_root_node, "ata_sct_status/device_state/string").value_or(std::string());
		add_property(p);

		lines.emplace_back(fmt::format("Device state: {}", get_node_data<std::string>(json_root_node, "ata_sct_status/device_state/string").value_or(std::string())));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/current").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/current", _("Current temperature (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/current").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Current temperature: {}° Celsius", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/current").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/power_cycle_min").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/power_cycle_min", _("Power cycle min. temperature (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/power_cycle_min").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Power cycle min. temperature: {}° Celsius", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/power_cycle_min").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/power_cycle_max").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/power_cycle_max", _("Power cycle max. temperature (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/power_cycle_max").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Power cycle max. temperature: {}° Celsius", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/power_cycle_max").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/lifetime_min").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/lifetime_min", _("Lifetime min. temperature (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/lifetime_min").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Lifetime min. temperature: {}° Celsius", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/lifetime_min").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/lifetime_max").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/lifetime_max", _("Lifetime max. temperature (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/lifetime_max").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Lifetime max. temperature: {}° Celsius", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/lifetime_max").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/under_limit_count").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/under_limit_count", _("Under limit count"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/under_limit_count").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Under limit count: {}", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/under_limit_count").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_status/temperature/over_limit_count").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_status/temperature/over_limit_count", _("Over limit count"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/over_limit_count").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Over limit count: {}", get_node_data<int64_t>(json_root_node, "ata_sct_status/temperature/over_limit_count").value_or(0)));
	}

	lines.emplace_back();

	if (get_node_exists(json_root_node, "ata_sct_temperature_history/version").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/version", _("SCT temperature history version"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/version").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("SCT temperature history version: {}", get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/version").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_temperature_history/sampling_period_minutes").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/sampling_period_minutes", _("Temperature sampling period (min)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/sampling_period_minutes").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Temperature sampling period: {} min.", get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/sampling_period_minutes").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_temperature_history/logging_interval_minutes").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/logging_interval_minutes", _("Temperature logging interval (min)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/logging_interval_minutes").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Temperature logging interval: {} min.", get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/logging_interval_minutes").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_temperature_history/temperature/op_limit_min").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/temperature/op_limit_min", _("Recommended operating temperature (minimum) (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/op_limit_min").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Recommended operating temperature (minimum): {}° Celsius",
				get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/op_limit_min").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_temperature_history/temperature/op_limit_max").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/temperature/op_limit_max", _("Recommended operating temperature (maximum) (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/op_limit_max").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Recommended operating temperature (maximum): {}° Celsius",
				get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/op_limit_max").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_temperature_history/temperature/limit_min").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/temperature/limit_min", _("Allowed operating temperature (minimum) (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/limit_min").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Allowed operating temperature (minimum): {}° Celsius",
				get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/limit_min").value_or(0)));
	}
	if (get_node_exists(json_root_node, "ata_sct_temperature_history/temperature/limit_max").value_or(false)) {
		StorageProperty p;
		p.set_name("ata_sct_temperature_history/temperature/limit_max", _("Allowed operating temperature (maximum) (C)"));
		p.section = StoragePropertySection::TemperatureLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/limit_max").value_or(0);
		add_property(p);

		lines.emplace_back(fmt::format("Allowed operating temperature (maximum): {}° Celsius",
				get_node_data<int64_t>(json_root_node, "ata_sct_temperature_history/temperature/limit_max").value_or(0)));
	}

	// The whole section
	if (!lines.empty()) {
		StorageProperty p;
		p.set_name("ata_sct_status/_and/ata_sct_temperature_history/_merged", _("Temperature log"));
		p.section = StoragePropertySection::TemperatureLog;
		p.reported_value = hz::string_join(lines, "\n");
		p.value = p.reported_value;  // string-type value
		add_property(p);

		section_properties_found = true;
	}

	// TODO Temperature log graph

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::TemperatureLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_scterc_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;
	using namespace std::string_literals;

	bool section_properties_found = false;

	std::vector<std::string> lines;

	if (get_node_exists(json_root_node, "ata_sct_erc/read/enabled").value_or(false)) {
		lines.emplace_back(fmt::format("SCT error recovery control (read): {}, {:.2f} seconds",
				(get_node_data<bool>(json_root_node, "ata_sct_erc/read/enabled").value_or(false) ? "enabled" : "disabled"),
				get_node_data<double>(json_root_node, "ata_sct_erc/read/deciseconds").value_or(0.) / 10.));
	}
	if (get_node_exists(json_root_node, "ata_sct_erc/write/enabled").value_or(false)) {
		lines.emplace_back(fmt::format("SCT error recovery control (write): {}, {:.2f} seconds",
				(get_node_data<bool>(json_root_node, "ata_sct_erc/write/enabled").value_or(false) ? "enabled" : "disabled"),
				get_node_data<double>(json_root_node, "ata_sct_erc/write/deciseconds").value_or(0.) / 10.));
	}

	// The whole section
	if (!lines.empty()) {
		StorageProperty p;
		p.set_name("ata_sct_erc/_merged", _("SCT error recovery log"));
		p.section = StoragePropertySection::ErcLog;
		p.reported_value = hz::string_join(lines, "\n");
		p.value = p.reported_value;  // string-type value
		add_property(p);

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::ErcLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_devstat(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	const std::string pages_key = "ata_device_statistics/pages";
	auto page_node = get_node(json_root_node, pages_key);

	// Entries
	if (page_node.has_value() && page_node->is_array()) {
		for (const auto& page_entry : page_node.value()) {
			AtaStorageStatistic page_stat;
			page_stat.is_header = true;
			page_stat.page = get_node_data<int64_t>(page_entry, "number").value_or(0);

			StorageProperty page_prop;
			{
				const std::string gen_name = get_node_data<std::string>(page_entry, "name").value_or(std::string());
				const std::string disp_name = gen_name;  // TODO: Translate
				page_prop.set_name(gen_name, disp_name);
				page_prop.section = StoragePropertySection::Statistics;
				page_prop.value = page_stat;
			}
			add_property(page_prop);

			const std::string table_key = "table";
			auto table_node = get_node(page_entry, table_key);

			if (table_node.has_value() && table_node->is_array()) {
				for (const auto& table_entry : table_node.value()) {
					AtaStorageStatistic s;
					s.page = page_stat.page;
					s.flags = get_node_data<std::string>(table_entry, "flags/string").value_or(std::string());
					s.value_int = get_node_data<int64_t>(table_entry, "value").value_or(0);
					s.value = std::to_string(get_node_data<int64_t>(table_entry, "value").value_or(0));
					s.offset = get_node_data<int64_t>(table_entry, "offset").value_or(0);

					StorageProperty p;
					const std::string gen_name = get_node_data<std::string>(table_entry, "name").value_or(std::string());
					p.set_name(gen_name, gen_name, gen_name);  // The description database will correct this.
					p.section = StoragePropertySection::Statistics;
					p.value = s;
					add_property(p);
				}
			}

			section_properties_found = true;
		}
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::Statistics)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_sataphy(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;
	using namespace std::string_literals;

	bool section_properties_found = false;

	std::vector<std::string> lines;

	// Table
	const std::string table_key = "sata_phy_event_counters/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		for (const auto& table_entry : table_node.value()) {
			const uint64_t id = get_node_data<uint64_t>(table_entry, "id").value_or(0);
			const std::string name = get_node_data<std::string>(table_entry, "name").value_or(std::string());
			const uint64_t size = get_node_data<uint64_t>(table_entry, "size").value_or(0);
			const int64_t value = get_node_data<int64_t>(table_entry, "value").value_or(0);
//			const bool overflow = get_node_data<bool>(table_entry, "overflow").value_or(false);

			lines.emplace_back(fmt::format(
					"ID: 0x{:04X}    Size: {:8}    Value: {:20}    Description: {}",
					id,
					size,
					value,
					name));
		}

		// The whole section
		{
			StorageProperty p;
			p.set_name("sata_phy_event_counters/_merged", _("SATA Phy Log"));
			p.section = StoragePropertySection::PhyLog;
			p.reported_value = hz::string_join(lines, "\n");
			p.value = p.reported_value;  // string-type value

			add_property(p);
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				fmt::format("No section {} parsed.", StoragePropertySectionExt::get_displayable_name(StoragePropertySection::PhyLog)));
	}

	return {};
}



/// @}
