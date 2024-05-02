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

#include "ata_storage_property.h"
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

	// Add properties for each parsed section so that the UI knows which tabs to show or hide
	{
		auto section_parse_status = parse_section_health(json_root_node);
		AtaStorageProperty p;
		p.section = AtaStorageProperty::Section::Health;
		p.set_name("_parser/health_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_error_log(json_root_node);
		AtaStorageProperty p;
		p.section = AtaStorageProperty::Section::ErrorLog;
		p.set_name("_parser/error_log_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_selftest_log(json_root_node);
		AtaStorageProperty p;
		p.section = AtaStorageProperty::Section::SelftestLog;
		p.set_name("_parser/selftest_log_section_available");
		p.value = section_parse_status.has_value() || section_parse_status.error().data() != SmartctlParserError::NoSection;
	}
	{
		auto section_parse_status = parse_section_health_log(json_root_node);
		AtaStorageProperty p;
		p.section = AtaStorageProperty::Section::Devstat;
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
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					if (auto jval = get_node_data<std::string>(root_node, "device/type"); jval.has_value()) {
						AtaStorageProperty p;
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
						-> hz::ExpectedValue<AtaStorageProperty, SmartctlParserError>
				{
					if (auto jval = get_node_data<int64_t>(root_node, "user_capacity/bytes"); jval) {
						AtaStorageProperty p;
						p.set_name(key, key, displayable_name);
						p.readable_value = hz::format_size(static_cast<uint64_t>(jval.value()), true);
						p.value = jval.value();
						p.show_in_ui = false;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", "user_capacity/bytes"));
				}
			},

			{"logical_block_size", _("Logical Block Size"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} bytes", value);
				})
			},

			{"power_cycle_count", _("Number of Power Cycles"), string_formatter()},
			{"power_on_time/hours", _("Powered for"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{} hours", value);
				})
			},

			{"temperature/current", _("Current Temperature"),
				custom_string_formatter<int64_t>([](int64_t value)
				{
					return std::format("{}° Celsius", value);
				})
			},

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



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_health(const nlohmann::json& json_root_node)
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



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_error_log(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	// TODO
	// nvme_error_information_log

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", AtaStorageProperty::get_readable_section_name(AtaStorageProperty::Section::ErrorLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_selftest_log(const nlohmann::json& json_root_node)
{
	// nvme_self_test_log

	using namespace SmartctlJsonParserHelpers;

	bool section_properties_found = false;

	// Revision
	if (get_node_exists(json_root_node, "ata_smart_self_test_log/extended/revision").value_or(false)) {
		AtaStorageProperty p;
		p.set_name("ata_smart_self_test_log/extended/revision", "ata_smart_self_test_log/extended/revision", _("SMART extended self-test log version"));
		p.section = AtaStorageProperty::Section::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_self_test_log/extended/revision").value_or(0);
		add_property(p);
		section_properties_found = true;
	}

	std::vector<std::string> counts;

	// Count
	{
		AtaStorageProperty p;
		p.set_name("ata_smart_self_test_log/extended/count", "ata_smart_self_test_log/extended/count", _("Self-test count"));
		p.section = AtaStorageProperty::Section::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_self_test_log/extended/count").value_or(0);
		p.show_in_ui = false;
		add_property(p);
		counts.emplace_back(std::format("Self-test entries: {}", p.get_value<int64_t>()));
	}
	// Error Count
	{
		AtaStorageProperty p;
		p.set_name("ata_smart_self_test_log/extended/error_count_total", "ata_smart_self_test_log/extended/error_count_total", _("Total error count"));
		p.section = AtaStorageProperty::Section::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_self_test_log/extended/error_count_total").value_or(0);
		p.show_in_ui = false;
		add_property(p);
		counts.emplace_back(std::format("Total error count: {}", p.get_value<int64_t>()));
	}
	// Outdated Error Count
	{
		AtaStorageProperty p;
		p.set_name("ata_smart_self_test_log/extended/error_count_outdated", "ata_smart_self_test_log/extended/error_count_outdated", _("Outdated error count"));
		p.section = AtaStorageProperty::Section::SelftestLog;
		p.value = get_node_data<int64_t>(json_root_node, "ata_smart_self_test_log/extended/error_count_outdated").value_or(0);
		p.show_in_ui = false;
		add_property(p);
		counts.emplace_back(std::format("Outdated error count: {}", p.get_value<int64_t>()));
	}

	// Displayed Counts
	if (!counts.empty()) {
		AtaStorageProperty p;
		p.set_name("ata_smart_self_test_log/extended/_counts", "ata_smart_self_test_log/extended/_counts", _("Entries"));
		p.section = AtaStorageProperty::Section::SelftestLog;
		p.value = hz::string_join(counts, "; ");
		add_property(p);

		section_properties_found = true;
	}

	const std::string table_key = "ata_smart_self_test_log/extended/table";
	auto table_node = get_node(json_root_node, table_key);

	// Entries
	if (table_node.has_value() && table_node->is_array()) {
		uint32_t entry_num = 1;
		for (const auto& table_entry : table_node.value()) {
			AtaStorageSelftestEntry entry;
			entry.test_num = entry_num;
			entry.type = get_node_data<std::string>(table_entry, "type/string").value_or(std::string());  // FIXME use type/value for i18n
			entry.status_str = get_node_data<std::string>(table_entry, "status/string").value_or(std::string());
			entry.remaining_percent = get_node_data<int8_t>(table_entry, "status/remaining_percent").value_or(0);
			entry.lifetime_hours = get_node_data<uint32_t>(table_entry, "lifetime_hours").value_or(0);
			entry.passed = get_node_data<bool>(table_entry, "status/passed").value_or(false);

			if (get_node_exists(table_entry, "lba").value_or(false)) {
				entry.lba_of_first_error = std::format("0x{:X}", get_node_data<uint64_t>(table_entry, "lba").value_or(0));
			} else {
				entry.lba_of_first_error = "-";
			}

			if (get_node_exists(table_entry, "status/value").value_or(false)) {
				const uint8_t status_value = get_node_data<uint8_t>(table_entry, "status/value").value_or(0);
				entry.status = static_cast<AtaStorageSelftestEntry::Status>(status_value >> 4);
			} else {
				entry.status = AtaStorageSelftestEntry::Status::Unknown;
			}

			AtaStorageProperty p;
			p.set_name(std::format("Self-test entry {}", entry.test_num));
			p.section = AtaStorageProperty::Section::SelftestLog;
			p.value = entry;
			add_property(p);

			++entry_num;
		}

		section_properties_found = true;
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", AtaStorageProperty::get_readable_section_name(AtaStorageProperty::Section::SelftestLog)));
	}

	return {};
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonNvmeParser::parse_section_health_log(const nlohmann::json& json_root_node)
{
	// nvme_smart_health_information_log

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

			AtaStorageProperty page_prop;
			page_prop.set_name(get_node_data<std::string>(page_entry, "name").value_or(std::string()));
			page_prop.section = AtaStorageProperty::Section::Devstat;
			page_prop.value = page_stat;
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

					AtaStorageProperty p;
					p.set_name(get_node_data<std::string>(table_entry, "name").value_or(std::string()));
					p.section = AtaStorageProperty::Section::Devstat;
					p.value = s;
					add_property(p);
				}
			}

			section_properties_found = true;
		}
	}

	if (!section_properties_found) {
		return hz::Unexpected(SmartctlParserError::NoSection,
				std::format("No section {} parsed.", AtaStorageProperty::get_readable_section_name(AtaStorageProperty::Section::Devstat)));
	}

	return {};
}



/// @}
