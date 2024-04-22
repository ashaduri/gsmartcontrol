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
#include <format>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <chrono>

#include "json/json.hpp"

#include "ata_storage_property.h"
#include "hz/debug.h"
#include "hz/string_algo.h"
// #include "smartctl_version_parser.h"
#include "hz/format_unit.h"
#include "hz/error_container.h"
#include "hz/string_num.h"
#include "smartctl_json_parser_helpers.h"


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
	_text_only/smart_supported
	_text_only/smart_enabled
	_text_only/write_cache_reorder
	_text_only/power_mode

- Automatic Offline Data Collection toggle support
	_text_only/aodc_support

 - Directory log supported
 	We don't use this.
 	_text_only/directory_log_supported

ata_smart_error_log/_not_present


Keys:
smartctl/version/_merged
 	Looks like "7.2"
smartctl/version/_merged_full
	Looks like "smartctl 7.2 2020-12-30 r5155", formed from "/smartctl" subkeys.

_custom/smart_enabled
 	Not present in json?
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

	AtaStorageProperty merged_property, full_property;
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

	// Ignore parse errors here, they are not critical.
	[[maybe_unused]] auto health_parse_status = parse_section_health(json_root_node);
	[[maybe_unused]] auto capabilities_parse_status = parse_section_capabilities(json_root_node);
	[[maybe_unused]] auto attributes_parse_status = parse_section_attributes(json_root_node);
	[[maybe_unused]] auto directory_log_parse_status = parse_section_directory_log(json_root_node);
	[[maybe_unused]] auto error_log_parse_status = parse_section_error_log(json_root_node);
	[[maybe_unused]] auto selftest_log_parse_status = parse_section_selftest_log(json_root_node);
	[[maybe_unused]] auto selective_selftest_log_parse_status = parse_section_selective_selftest_log(json_root_node);
	[[maybe_unused]] auto scttemp_log_parse_status = parse_section_scttemp_log(json_root_node);
	[[maybe_unused]] auto scterc_log_parse_status = parse_section_scterc_log(json_root_node);
	[[maybe_unused]] auto devstat_parse_status = parse_section_devstat(json_root_node);
	[[maybe_unused]] auto sataphy_parse_status = parse_section_sataphy(json_root_node);

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_info(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	// This is very similar to Basic Parser, but the Basic Parser supports different drive types, while this
	// one is only for ATA.

	static const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> json_keys = {
			{"model_family", _("Model Family"), string_formatter()},
			{"model_name", _("Device Model"), string_formatter()},
			{"serial_number", _("Serial Number"), string_formatter()},

			{"wwn/_merged", _("World Wide Name"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto jval1 = get_node_data<int64_t>(root_node, "wwn/naa");
					auto jval2 = get_node_data<int64_t>(root_node, "wwn/oui");
					auto jval3 = get_node_data<int64_t>(root_node, "wwn/id");

					if (jval1 && jval2 && jval3) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						// p.readable_value = std::format("{:X} {:X} {:X}", jval1.value(), jval2.value(), jval3.value());
						p.readable_value = std::format("{:X}-{:06X}-{:08X}", jval1.value(), jval2.value(), jval3.value());
						p.value = p.readable_value;  // string-type value
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"firmware_version", _("Firmware Version"), string_formatter()},

			{"user_capacity/bytes", _("Capacity"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} [{}; {} bytes]",
						hz::format_size(static_cast<uint64_t>(value), true),
						hz::format_size(static_cast<uint64_t>(value), false),
						hz::number_to_string_locale(value));
				})
			},

			{"physical_block_size/_and/logical_block_size", _("Sector Size"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					std::vector<std::string> values;
					if (auto jval1 = get_node_data<int64_t>(root_node, "logical_block_size"); jval1) {
						values.emplace_back(std::format("{} bytes logical", jval1.value()));
					}
					if (auto jval2 = get_node_data<int64_t>(root_node, "physical_block_size"); jval2) {
						values.emplace_back(std::format("{} bytes physical", jval2.value()));
					}
					if (!values.empty()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.readable_value = hz::string_join(values, ", ");
						p.value = p.readable_value;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"rotation_rate", _("Rotation Rate"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} RPM", value);
				})
			},

			{"form_factor/name", _("Form Factor"), string_formatter()},
			{"trim/supported", _("TRIM Supported"), bool_formatter(_("Yes"), _("No"))},
			{"in_smartctl_database", _("In Smartctl Database"), bool_formatter(_("Yes"), _("No"))},
			{"ata_version/string", _("ATA Version"), string_formatter()},
			{"sata_version/string", _("SATA Version"), string_formatter()},

			{"interface_speed/_merged", _("Interface Speed"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					std::vector<std::string> values;
					if (auto jval1 = get_node_data<std::string>(root_node, "interface_speed/max/string"); jval1) {
						values.emplace_back(std::format("Max: {}", jval1.value()));
					}
					if (auto jval2 = get_node_data<std::string>(root_node, "interface_speed/current/string"); jval2) {
						values.emplace_back(std::format("Current: {}", jval2.value()));
					}
					if (!values.empty()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.readable_value = hz::string_join(values, ", ");
						p.value = p.readable_value;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"local_time/asctime", _("Scanned on"), string_formatter()},
			{"read_lookahead/enabled", _("Read Look-Ahead"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"write_cache/enabled", _("Write Cache"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"ata_dsn/enabled", _("DSN Feature"), bool_formatter(_("Enabled"), _("Disabled"))},
			{"ata_security/string", _("ATA Security"), string_formatter()},
	};

	bool any_found = false;
	for (const auto& [key, displayable_name, retrieval_func] : json_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = AtaStorageProperty::Section::Info;
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
			p->section = AtaStorageProperty::Section::Health;
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
							-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, "ata_smart_data/offline_data_collection/status/value");
					if (value_val.has_value()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = static_cast<bool>(value_val.value() & 0x80);  // taken from ataprint.cpp
						p.readable_value = p.get_value<bool>() ? _("Enabled") : _("Disabled");
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},


			// Last self-test status
			{"ata_smart_data/offline_data_collection/status/value/_decoded", "Last offline data collection status",
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
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
						AtaStorageProperty p;
						p.section = AtaStorageProperty::Section::Capabilities;
						p.set_name(key, key, displayable_name);
						p.value = status_str;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/offline_data_collection/completion_seconds", _("Time to complete offline data collection"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = std::chrono::seconds(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/status/value/_decoded", _("Self-test execution status"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
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

						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = AtaStorageSelftestEntry::get_readable_status_name(status);
						return p;
					}

					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/status/remaining_percent", _("Self-test remaining percentage"),
			 		custom_string_formatter<int64_t>([](int64_t value)
					{
						return std::format("{} %", value);
					})
			 },


			{"ata_smart_data/capabilities/self_tests_supported", _("Self-tests supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_smart_data/capabilities/exec_offline_immediate_supported", _("Offline immediate test supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/offline_is_aborted_upon_new_cmd", _("Abort offline collection on new command"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/offline_surface_scan_supported", _("Offline surface scan supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_smart_data/capabilities/conveyance_self_test_supported", _("Conveyance self-test supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/selective_self_test_supported", _("Selective self-test supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_smart_data/self_test/polling_minutes/short", _("Short self-test status recommended polling time"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = std::chrono::minutes(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/polling_minutes/extended", _("Extended self-test status recommended polling time"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = std::chrono::minutes(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"ata_smart_data/self_test/polling_minutes/conveyance", _("Conveyance self-test status recommended polling time"),
					[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_data<int64_t>(root_node, key);
					if (value_val.has_value()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = std::chrono::minutes(value_val.value());
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},


			{"ata_smart_data/capabilities/attribute_autosave_enabled", _("Saves SMART data before entering power-saving mode"), bool_formatter(_("Enabled"), _("Disabled"))},

			{"ata_smart_data/capabilities/error_logging_supported", _("Error logging supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_smart_data/capabilities/gp_logging_supported", _("General purpose logging supported"), bool_formatter(_("Yes"), _("No"))},

			{"ata_sct_capabilities/_supported", _("SCT capabilities supported"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
							-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					auto value_val = get_node_exists(root_node, "ata_sct_capabilities");
					if (value_val.has_value()) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.value = value_val.value();
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},
			{"ata_sct_capabilities/error_recovery_control_supported", _("SCT error recovery control supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_sct_capabilities/feature_control_supported", _("SCT feature control supported"), bool_formatter(_("Yes"), _("No"))},
			{"ata_sct_capabilities/data_table_supported", _("SCT data table supported"), bool_formatter(_("Yes"), _("No"))},

	};

	for (const auto& [key, displayable_name, retrieval_func] : json_keys) {
		DBG_ASSERT(retrieval_func != nullptr);
		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = AtaStorageProperty::Section::Capabilities;
			add_property(p.value());
		}
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_attributes(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	// Revision
	{
		AtaStorageProperty p;
		p.set_name("ata_smart_attributes/revision", "ata_smart_attributes/revision", _("Data structure revision number"));
		p.section = AtaStorageProperty::Section::Attributes;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_attributes/revision").value_or(0);
		add_property(p);
	}

	std::string table_key = "ata_smart_attributes/table";
	auto table_node = get_node(json_root_node, table_key);
	if (!table_node) {
		return hz::Unexpected(SmartctlParserError::KeyNotFound, table_node.error().message());
	}
	if (!table_node->is_array()) {
		return hz::Unexpected(SmartctlParserError::DataError, std::format("Node {} is not an array.", table_key));
	}

	// Attributes
	for (const auto& attr : table_node.value()) {
		AtaStorageAttribute a;

		a.id = get_node_data<int32_t>(attr, "id").value_or(0);
		a.flag = get_node_data<std::string>(attr, "flags/string").value_or("");
		a.value = (get_node_exists(attr, "value").value_or(false) ? std::optional<uint8_t>(get_node_data<uint8_t>(attr, "value").value_or(0)) : std::nullopt);
		a.worst = (get_node_exists(attr, "worst").value_or(false) ? std::optional<uint8_t>(get_node_data<uint8_t>(attr, "worst").value_or(0)) : std::nullopt);
		a.threshold = (get_node_exists(attr, "thresh").value_or(false) ? std::optional<uint8_t>(get_node_data<uint8_t>(attr, "thresh").value_or(0)) : std::nullopt);
		a.attr_type = get_node_data<bool>(attr, "flags/prefailure").value_or(false) ? AtaStorageAttribute::AttributeType::Prefail : AtaStorageAttribute::AttributeType::OldAge;
		a.update_type = get_node_data<bool>(attr, "flags/updated_online").value_or(false) ? AtaStorageAttribute::UpdateType::Always : AtaStorageAttribute::UpdateType::Offline;

		const std::string when_failed = get_node_data<std::string>(attr, "when_failed").value_or(std::string());
		if (when_failed == "now") {
			a.when_failed = AtaStorageAttribute::FailTime::Now;
		} else if (when_failed == "past") {
			a.when_failed = AtaStorageAttribute::FailTime::Past;
		} else {  // ""
			a.when_failed = AtaStorageAttribute::FailTime::None;
		}

		a.raw_value = get_node_data<std::string>(attr, "raw/string").value_or(std::string());
		a.raw_value_int = get_node_data<int64_t>(attr, "raw/value").value_or(0);

		AtaStorageProperty p;
		p.set_name(get_node_data<std::string>(attr, "name").value_or(std::string()));
		p.section = AtaStorageProperty::Section::Attributes;
		p.value = a;
		add_property(p);
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_directory_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;
	using namespace std::string_literals;

	std::vector<std::string> lines;

	{
		AtaStorageProperty p;
		p.set_name("ata_log_directory/gp_dir_version", "ata_log_directory/gp_dir_version", _("General purpose log directory version"));
		p.section = AtaStorageProperty::Section::DirectoryLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_log_directory/gp_dir_version").value_or(0);
		add_property(p);

		lines.emplace_back(std::format("General Purpose Log Directory Version: {}", p.get_value<int64_t>()));
	}
	{
		AtaStorageProperty p;
		p.set_name("ata_log_directory/smart_dir_version", "ata_log_directory/smart_dir_version", _("SMART log directory version"));
		p.section = AtaStorageProperty::Section::DirectoryLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_log_directory/smart_dir_version").value_or(0);
		add_property(p);

		lines.emplace_back(std::format("SMART Log Directory Version: {}", p.get_value<int64_t>()));
	}
	{
		AtaStorageProperty p;
		p.set_name("ata_log_directory/smart_dir_multi_sector", "ata_log_directory/smart_dir_multi_sector", _("Multi-sector log support"));
		p.section = AtaStorageProperty::Section::DirectoryLog;
		p.value = get_node_data<bool>(json_root_node, "ata_log_directory/smart_dir_multi_sector").value_or(0);
		add_property(p);

		lines.emplace_back(std::format("Multi-sector log support: {}", p.get_value<bool>() ? "Yes" : "No"));
	}

	// Table
	std::string table_key = "ata_log_directory/table";
	auto table_node = get_node(json_root_node, table_key);
	if (!table_node) {
		return hz::Unexpected(SmartctlParserError::KeyNotFound, table_node.error().message());
	}
	if (!table_node->is_array()) {
		return hz::Unexpected(SmartctlParserError::DataError, std::format("Node {} is not an array.", table_key));
	}

	// Attributes
	for (const auto& table_entry : table_node.value()) {
		const uint64_t address = get_node_data<uint64_t>(table_entry, "address").value_or(0);
		const std::string name = get_node_data<std::string>(table_entry, "name").value_or(std::string());
		const bool read = get_node_data<bool>(table_entry, "read").value_or(false);
		const bool write = get_node_data<bool>(table_entry, "write").value_or(false);
		const uint64_t gp_sectors = get_node_data<uint64_t>(table_entry, "gp_sectors").value_or(0);
		const uint64_t smart_sectors = get_node_data<uint64_t>(table_entry, "smart_sectors").value_or(0);

		// Address, GPL/SL, RO/RW, Num Sectors (GPL, Smart) , Name
		// 0x00       GPL,SL  R/O      1  Log Directory
		lines.emplace_back(std::format(
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
		AtaStorageProperty p;
		p.set_name("General Purpose Log Directory", "ata_log_directory/_merged");
		p.section = AtaStorageProperty::Section::DirectoryLog;
		p.reported_value = hz::string_join(lines, "\n");
		p.value = p.reported_value;  // string-type value

		add_property(p);
	}


	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_error_log(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_selftest_log(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_selective_selftest_log(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_scttemp_log(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_scterc_log(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_devstat(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_sataphy(const nlohmann::json& json_root_node)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonAtaParser::parse_section_internal_capabilities(AtaStorageProperty& cap_prop)
{
	return hz::ExpectedVoid<SmartctlParserError>();
}



/// @}
