/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "smartctl_json_nvme_parser.h"

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <chrono>

#include "json/json.hpp"

#include "storage_property.h"
#include "hz/debug.h"
#include "hz/string_algo.h"
// #include "smartctl_version_parser.h"
#include "hz/format_unit.h"
#include "hz/error_container.h"
#include "hz/string_num.h"
#include "smartctl_json_parser_helpers.h"
#include "smartctl_parser_types.h"



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse(std::string_view smartctl_output)
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
		StorageProperty p;
		p.section = StorageProperty::Section::Health;
		p.set_name("_parser/health_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_error_log(json_root_node);
		StorageProperty p;
		p.section = StorageProperty::Section::ErrorLog;
		p.set_name("_parser/error_log_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_selftest_log(json_root_node);
		StorageProperty p;
		p.section = StorageProperty::Section::SelftestLog;
		p.set_name("_parser/selftest_log_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_nvme_attributes(json_root_node);
		StorageProperty p;
		p.section = StorageProperty::Section::Devstat;
		p.set_name("_parser/devstat_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_info(const nlohmann::json& json_root_node)
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
						p.set_name(key, key, displayable_name);
						p.value = jval.value();
						p.show_in_ui = false;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"model_name", _("Device Model"), string_formatter()},
			{"serial_number", _("Serial Number"), string_formatter()},
			{"firmware_version", _("Firmware Version"), string_formatter()},

			{"nvme_total_capacity", _("Total Capacity"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} [{}; {} bytes]",
						hz::format_size(static_cast<uint64_t>(value), true),
						hz::format_size(static_cast<uint64_t>(value), false),
						hz::number_to_string_locale(value));
				})
			},

			{"nvme_unallocated_capacity", _("Unallocated Capacity"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} [{}; {} bytes]",
						hz::format_size(static_cast<uint64_t>(value), true),
						hz::format_size(static_cast<uint64_t>(value), false),
						hz::number_to_string_locale(value));
				})
			},

			{"user_capacity/bytes", _("Capacity"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} [{}; {} bytes]",
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
						p.set_name(key, key, displayable_name);
						p.readable_value = hz::format_size(static_cast<uint64_t>(jval.value()), true);
						p.value = jval.value();
						p.show_in_ui = false;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", "user_capacity/bytes"));
				}
			},

			{"logical_block_size", _("Logical Block Size"), integer_formatter<int64_t>("{} bytes")},
			{"power_cycle_count", _("Number of Power Cycles"), integer_formatter<int64_t>()},
			{"power_on_time/hours", _("Powered for"), integer_formatter<int64_t>("{} hours")},
			{"temperature/current", _("Current Temperature"), integer_formatter<int64_t>("{}° Celsius")},

			{"nvme_version/string", _("NVMe Version"), string_formatter()},
			{"local_time/asctime", _("Scanned on"), string_formatter()},

			{"smart_support/available", _("SMART Supported"), bool_formatter(_("Yes"), _("No"))},
			{"smart_support/enabled", _("SMART Enabled"), bool_formatter(_("Yes"), _("No"))},
	};

	bool any_found = false;
	for (const auto& [key, displayable_name, retrieval_func] : json_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StorageProperty::Section::Info;
			add_property(p.value());
			any_found = true;
		}
	}

	if (!any_found) {
		return hz::Unexpected(SmartctlParserError::KeyNotFound, "No keys info found in JSON data.");
	}
	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_health(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> health_keys = {
			{"smart_status/passed", _("Overall Health Self-Assessment Test"), bool_formatter(_("PASSED"), _("FAILED"))},

			// These are included when smart_status/passed is false
			{"smart_status/nvme/spare_below_threshold", _("Available Spare Fallen Below Threshold"), bool_formatter(_("Yes"), _("No"))},
			{"smart_status/nvme/temperature_above_or_below_threshold", _("Temperature Outside Limits"), bool_formatter(_("Yes"), _("No"))},
			{"smart_status/nvme/reliability_degraded", _("NVM Subsystem Reliability Degraded"), bool_formatter(_("Yes"), _("No"))},
			{"smart_status/nvme/media_read_only", _("Media Placed in Read-Only Mode"), bool_formatter(_("Yes"), _("No"))},
			{"smart_status/nvme/volatile_memory_backup_failed", _("Volatile Memory Backup Failed"), bool_formatter(_("Yes"), _("No"))},
			{"smart_status/nvme/persistent_memory_region_unreliable", _("Persistent Memory Region Is Read-Only or Unreliable"), bool_formatter(_("Yes"), _("No"))},
			{"smart_status/nvme/other", _("Unknown Critical Warnings"), bool_formatter(_("Yes"), _("No"))},
	};

	for (const auto& [key, displayable_name, retrieval_func] : health_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StorageProperty::Section::Health;
			add_property(p.value());

			section_properties_found = true;
		}
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", StorageProperty::get_readable_section_name(StorageProperty::Section::Health)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_error_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	// TODO
	// nvme_error_information_log

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", StorageProperty::get_readable_section_name(StorageProperty::Section::ErrorLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_selftest_log(const nlohmann::json& json_root_node)
{
	// nvme_self_test_log

	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	{
		StorageProperty p;
		p.set_name("nvme_self_test_log/current_self_test_operation/value/_decoded",
				"nvme_self_test_log/current_self_test_operation/value/_decoded", _("Current Self-Test Operation"));
		p.section = StorageProperty::Section::SelftestLog;

		auto value_val = get_node_data<uint8_t>(json_root_node, "nvme_self_test_log/current_self_test_operation/value");
		NvmeSelfTestCurrentOperationType operation = NvmeSelfTestCurrentOperationType::Unknown;
		if (value_val.has_value()) {
			switch (value_val.value()) {
				// Data from smartmontools/nvmeprint.cpp
				case 0x0: operation = NvmeSelfTestCurrentOperationType::None; break;
				case 0x1: operation = NvmeSelfTestCurrentOperationType::ShortInProgress; break;
				case 0x2: operation = NvmeSelfTestCurrentOperationType::ExtendedInProgress; break;
				case 0xe: operation = NvmeSelfTestCurrentOperationType::VendorSpecificInProgress; break;
				default: break;  // Unknown
			}
			p.value = NvmeSelfTestCurrentOperationTypeExt::get_storable_name(operation);
			p.readable_value = NvmeSelfTestCurrentOperationTypeExt::get_displayable_name(operation);
			add_property(p);

			section_properties_found = true;
		}
	}

	{
		StorageProperty p;
		p.set_name("nvme_self_test_log/current_self_test_operation/current_self_test_completion_percent",
				"nvme_self_test_log/current_self_test_operation/current_self_test_completion_percent", _("Current Self-Test Completion Percentage"));
		p.section = StorageProperty::Section::SelftestLog;

		auto value_val = get_node_data<uint8_t>(json_root_node, "nvme_self_test_log/current_self_test_operation/current_self_test_completion_percent");
		if (value_val.has_value()) {
			p.value = value_val.value();
			p.readable_value = std::format("{} %", value_val.value());
			add_property(p);
		}
	}

	const std::string table_key = "nvme_self_test_log/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		uint32_t entry_num = 1;
		for (const auto& table_entry : table_node.value()) {
			NvmeStorageSelftestEntry entry;
			entry.test_num = entry_num;

			NvmeSelfTestType test_type = NvmeSelfTestType::Unknown;
			if (get_node_exists(table_entry, "self_test_code/value").value_or(false)) {
				const int32_t type_value = get_node_data<int32_t>(table_entry, "self_test_code/value").value_or(int(NvmeSelfTestType::Unknown));
				switch(type_value) {
					case 0x1: test_type = NvmeSelfTestType::Short; break;
					case 0x2: test_type = NvmeSelfTestType::Extended; break;
					case 0xe: test_type = NvmeSelfTestType::VendorSpecific; break;
					default: break;  // Unknown
				}
			}

			NvmeSelfTestResultType test_result = NvmeSelfTestResultType::Unknown;
			if (get_node_exists(table_entry, "self_test_code/value").value_or(false)) {
				const int32_t type_value = get_node_data<int32_t>(table_entry, "self_test_code/value").value_or(int(NvmeSelfTestType::Unknown));
				switch(type_value) {
					case 0x0: test_result = NvmeSelfTestResultType::CompletedNoError; break;
					case 0x1: test_result = NvmeSelfTestResultType::AbortedSelfTestCommand; break;
					case 0x2: test_result = NvmeSelfTestResultType::AbortedControllerReset; break;
					case 0x3: test_result = NvmeSelfTestResultType::AbortedNamespaceRemoved; break;
					case 0x4: test_result = NvmeSelfTestResultType::AbortedFormatNvmCommand; break;
					case 0x5: test_result = NvmeSelfTestResultType::FatalOrUnknownTestError; break;
					case 0x6: test_result = NvmeSelfTestResultType::CompletedUnknownFailedSegment; break;
					case 0x7: test_result = NvmeSelfTestResultType::CompletedFailedSegments; break;
					case 0x8: test_result = NvmeSelfTestResultType::AbortedUnknownReason; break;
					case 0x9: test_result = NvmeSelfTestResultType::AbortedSanitizeOperation; break;
					default: break;  // Unknown
				}
			}

			entry.type = test_type;
			entry.result = test_result;
			entry.power_on_hours = get_node_data<uint32_t>(table_entry, "power_on_hours").value_or(0);
			entry.lba = get_node_data<uint64_t>(table_entry, "lba").value_or(0);

			StorageProperty p;
			p.set_name(std::format("Self-test entry {}", entry.test_num));
			p.section = StorageProperty::Section::SelftestLog;
			p.value = entry;
			add_property(p);

			++entry_num;
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", StorageProperty::get_readable_section_name(StorageProperty::Section::SelftestLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_nvme_attributes(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> health_keys = {
			{"nvme_smart_health_information_log/available_spare", _("Available Spare"), integer_formatter<int64_t>("{}%")},
			{"nvme_smart_health_information_log/available_spare_threshold", _("Available Spare Threshold"), integer_formatter<int64_t>("{}%")},
			{"nvme_smart_health_information_log/percentage_used", _("Percentage Used"), integer_formatter<int64_t>("{}%")},
			{"nvme_smart_health_information_log/data_units_read", _("Data Units Read"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/data_units_written", _("Data Units Written"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/host_reads", _("Host Read Commands"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/host_writes", _("Host Write Commands"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/controller_busy_time", _("Controller Busy Time"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/power_cycles", _("Power Cycles"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/power_on_hours", _("Power On Hours"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/unsafe_shutdowns", _("Unsafe Shutdowns"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/media_errors", _("Media and Data Integrity Errors"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/num_err_log_entries", _("Error Information Log Entries"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/warning_temp_time", _("Warning  Comp. Temperature Time"), integer_formatter<int64_t>()},
			{"nvme_smart_health_information_log/critical_comp_time", _("Critical Comp. Temperature Time"), integer_formatter<int64_t>()},
	};

	for (const auto& [key, displayable_name, retrieval_func] : health_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StorageProperty::Section::NvmeAttributes;
			add_property(p.value());

			section_properties_found = true;
		}
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", StorageProperty::get_readable_section_name(StorageProperty::Section::NvmeAttributes)));
	}

	return {};
}



/// @}
