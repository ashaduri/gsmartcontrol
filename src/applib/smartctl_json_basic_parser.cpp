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

#include "smartctl_json_basic_parser.h"

// #include <glibmm.h>
//#include <clocale>  // localeconv
#include <cstdint>
#include <format>
#include <string_view>
#include <tuple>
#include <vector>

// #include "hz/locale_tools.h"  // ScopedCLocale, locale_c_get().
#include "hz/format_unit.h"
#include "hz/string_algo.h"  // string_*
#include "hz/string_num.h"  // string_is_numeric, number_to_string
//#include "hz/debug.h"  // debug_*
#include "json/json.hpp"
#include "hz/error_container.h"

//#include "smartctl_text_ata_parser.h"
//#include "ata_storage_property_descr.h"
#include "storage_property.h"
#include "smartctl_json_parser_helpers.h"
#include "smartctl_parser_types.h"
//#include "smartctl_version_parser.h"



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonBasicParser::parse(std::string_view smartctl_output)
{
	using namespace SmartctlJsonParserHelpers;

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

	return parse_section_basic_info(json_root_node);
}



hz::ExpectedVoid<SmartctlParserError> SmartctlJsonBasicParser::parse_section_basic_info(const nlohmann::json& json_root_node)
{
	using namespace SmartctlJsonParserHelpers;

	// Here we list the properties that are:
	// 1. Essential for all devices, due to them being used in StorageDevice.
	// 2. Present in devices for which we do not have specialized parsers (USB, etc.)
	static const std::vector<std::tuple<std::string, std::string, PropertyRetrievalFunc>> info_keys = {

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

			{"vendor", _("Vendor"), string_formatter()},  // Flash drive
			{"scsi_vendor", _("Vendor"), string_formatter()},  // Flash drive

			{"product", _("Product"), string_formatter()},  // Flash drive
			{"scsi_product", _("Product"), string_formatter()},  // Flash drive

			{"model_family", _("Model Family"), string_formatter()},  // (S)ATA

			{"model_name", _("Device Model"), string_formatter()},
			{"scsi_model_name", _("Device Model"), string_formatter()},  // Flash drive

			{"revision", _("Revision"), string_formatter()},  // Flash drive
			{"scsi_revision", _("Revision"), string_formatter()},  // Flash drive

			{"scsi_version", _("SCSI Version"), string_formatter()},  // Flash drive

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

			{"physical_block_size/_and/logical_block_size", _("Sector Size"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					std::vector<std::string> values;
					if (auto jval1 = get_node_data<int64_t>(root_node, "logical_block_size"); jval1) {
						values.emplace_back(std::format("{} bytes logical", jval1.value()));
					}
					if (auto jval2 = get_node_data<int64_t>(root_node, "physical_block_size"); jval2) {
						values.emplace_back(std::format("{} bytes physical", jval2.value()));
					}
					if (!values.empty()) {
						StorageProperty p;
						p.set_name(key, key, displayable_name);
						p.readable_value = hz::string_join(values, ", ");
						p.value = p.readable_value;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"serial_number", _("Serial Number"), string_formatter()},
			{"firmware_version", _("Firmware Version"), string_formatter()},
			{"trim/supported", _("TRIM Supported"), bool_formatter(_("Yes"), _("No"))},
			{"in_smartctl_database", _("In Smartctl Database"), bool_formatter(_("Yes"), _("No"))},
			{"ata_version/string", _("ATA Version"), string_formatter()},
			{"sata_version/string", _("SATA Version"), string_formatter()},

			{"interface_speed/_merged", _("Interface Speed"),
				[](const nlohmann::json& root_node, const std::string& key, const std::string& displayable_name)
						-> hz::ExpectedValue<StorageProperty, SmartctlParserError>
				{
					std::vector<std::string> values;
					if (auto jval1 = get_node_data<std::string>(root_node, "interface_speed/max/string"); jval1) {
						values.emplace_back(std::format("Max: {}", jval1.value()));
					}
					if (auto jval2 = get_node_data<std::string>(root_node, "interface_speed/current/string"); jval2) {
						values.emplace_back(std::format("Current: {}", jval2.value()));
					}
					if (!values.empty()) {
						StorageProperty p;
						p.set_name(key, key, displayable_name);
						p.readable_value = hz::string_join(values, ", ");
						p.value = p.readable_value;
						return p;
					}
					return hz::Unexpected(SmartctlParserError::KeyNotFound, std::format("Error getting key {} from JSON data.", key));
				}
			},

			{"local_time/asctime", _("Scanned on"), string_formatter()},

			{"smart_support/available", _("SMART Supported"), bool_formatter(_("Yes"), _("No"))},
			{"smart_support/enabled", _("SMART Enabled"), bool_formatter(_("Yes"), _("No"))},

			{"smart_status/passed", _("Overall Health Self-Assessment Test"), bool_formatter(_("PASSED"), _("FAILED"))},

			// (S)ATA, used to detect HDD vs SSD
			{"rotation_rate", _("Rotation Rate"), integer_formatter<int64_t>("{} RPM")},

			{"form_factor/name", _("Form Factor"), string_formatter()},
	};

	for (const auto& [key, displayable_name, retrieval_func] : info_keys) {
		DBG_ASSERT(retrieval_func != nullptr);

		auto p = retrieval_func(json_root_node, key, displayable_name);
		if (p.has_value()) {  // ignore if not found
			p->section = StoragePropertySection::Info;
			add_property(p.value());
		}
	}

	return {};
}



/// @}
