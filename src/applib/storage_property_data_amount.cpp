/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#include "storage_property_data_amount.h"

#include <cctype>  // std::toupper
#include <limits>
#include <variant>

#include "hz/string_num.h"  // number_to_string_*
#include "hz/format_unit.h"  // format_size
#include "app_regex.h"



namespace {

	/// NVMe data unit: 1000 units of 512 bytes (see NVMe specification, SMART / Health Information).
	constexpr std::uint64_t nvme_data_unit_size = 512 * 1000;

}



std::optional<std::uint64_t> ata_attribute_get_data_unit_size(const std::string& reported_name)
{
	// Attributes counting errors, retries, etc. of sectors are not data amounts.
	if (app_regex_partial_match("/(Err|Fail|Retr|Realloc|Pend|Uncor|Bad)/i", reported_name)) {
		return std::nullopt;
	}

	// Explicit units: Host_Writes_32MiB, NAND_Writes_1GiB, Lifetime_Writes_GiB, Total_NAND_Prog_Ct_GiB, ...
	std::string count_str, unit_str;
	if (app_regex_partial_match("/_([0-9]*)([KMGT])iB$/i", reported_name, {&count_str, &unit_str})) {
		std::uint64_t count = 1;
		if (!count_str.empty() && (!hz::string_is_numeric_nolocale(count_str, count, false) || count == 0)) {
			return std::nullopt;
		}
		std::uint64_t unit = 0;
		switch (std::toupper(static_cast<unsigned char>(unit_str.front()))) {
			case 'K': unit = 1024ULL; break;
			case 'M': unit = 1024ULL * 1024; break;
			case 'G': unit = 1024ULL * 1024 * 1024; break;
			case 'T': unit = 1024ULL * 1024 * 1024 * 1024; break;
			default: return std::nullopt;
		}
		if (count > std::numeric_limits<std::uint64_t>::max() / unit) {
			return std::nullopt;
		}
		return count * unit;
	}

	// Sector / LBA counts (Total_LBAs_Written, ...) are not converted: their units vary between vendors,
	// devices and even firmware versions, so they can't be reliably converted to bytes without
	// a drive database entry (which would give the attribute a unit suffix).
	// Device Statistics provide the same information reliably.

	return std::nullopt;
}



std::optional<std::uint64_t> ata_statistic_get_data_unit_size(const std::string& reported_name,
		const DataAmountUnitSizes& unit_sizes)
{
	// These statistics count actual logical sectors. If smartctl reports the unit size, prefer it.
	if (reported_name == "Logical Sectors Written") {
		return unit_sizes.host_writes_unit_size ? unit_sizes.host_writes_unit_size : unit_sizes.logical_sector_size;
	}
	if (reported_name == "Logical Sectors Read") {
		return unit_sizes.host_reads_unit_size ? unit_sizes.host_reads_unit_size : unit_sizes.logical_sector_size;
	}
	return std::nullopt;
}



std::optional<std::uint64_t> parse_logical_sector_size(const std::string& sector_size_str)
{
	// "512 bytes logical, 4096 bytes physical" (JSON, text "Sector Sizes"),
	// "4096 bytes logical/physical" (text "Sector Size").
	std::string size_str;
	std::uint64_t size = 0;
	if (app_regex_partial_match("/([0-9]+) bytes logical/i", sector_size_str, {&size_str})
			&& hz::string_is_numeric_nolocale(size_str, size, false) && size > 0) {
		return size;
	}
	return std::nullopt;
}



DataAmountUnitSizes storage_property_get_data_amount_unit_sizes(const StoragePropertyRepository& properties)
{
	DataAmountUnitSizes unit_sizes;

	if (const auto p = properties.lookup_property("physical_block_size/_and/logical_block_size");
			!p.empty() && p.is_value_type<std::string>()) {
		unit_sizes.logical_sector_size = parse_logical_sector_size(p.get_value<std::string>());
	}

	auto get_unit_size = [&properties](const std::string& generic_name) -> std::optional<std::uint64_t>
	{
		const auto p = properties.lookup_property(generic_name);
		if (!p.empty() && p.is_value_type<std::int64_t>() && p.get_value<std::int64_t>() > 0) {
			return static_cast<std::uint64_t>(p.get_value<std::int64_t>());
		}
		return std::nullopt;
	};
	unit_sizes.host_writes_unit_size = get_unit_size("host_writes/bytes_per_unit");
	unit_sizes.host_reads_unit_size = get_unit_size("host_reads/bytes_per_unit");

	return unit_sizes;
}



std::optional<std::uint64_t> nvme_attribute_get_data_unit_size(const std::string& generic_name,
		const DataAmountUnitSizes& unit_sizes)
{
	// If smartctl reports the unit size, prefer it.
	if (generic_name == "nvme_smart_health_information_log/data_units_written") {
		return unit_sizes.host_writes_unit_size.value_or(nvme_data_unit_size);
	}
	if (generic_name == "nvme_smart_health_information_log/data_units_read") {
		return unit_sizes.host_reads_unit_size.value_or(nvme_data_unit_size);
	}
	return std::nullopt;
}



std::optional<std::uint64_t> data_amount_to_bytes(std::int64_t count, std::uint64_t unit_size)
{
	if (count <= 0 || unit_size == 0) {
		return std::nullopt;
	}
	const auto ucount = static_cast<std::uint64_t>(count);
	if (ucount > std::numeric_limits<std::uint64_t>::max() / unit_size) {
		return std::nullopt;  // overflow
	}
	return ucount * unit_size;
}



void storage_property_autoset_data_amount(StorageProperty& p, const DataAmountUnitSizes& unit_sizes)
{
	if (p.is_value_type<AtaStorageAttribute>()) {
		auto& attr = std::get<AtaStorageAttribute>(p.value);
		// Only plain numbers. Some raw values contain additional information, e.g. "37 (Min/Max 20/45)".
		if (hz::number_to_string_nolocale(attr.raw_value_int) != attr.raw_value) {
			return;
		}
		if (auto unit_size = ata_attribute_get_data_unit_size(p.reported_name)) {
			attr.raw_value_bytes = data_amount_to_bytes(attr.raw_value_int, unit_size.value());
		}

	} else if (p.is_value_type<AtaStorageStatistic>()) {
		auto& stat = std::get<AtaStorageStatistic>(p.value);
		if (stat.is_header || hz::number_to_string_nolocale(stat.value_int) != stat.value) {
			return;
		}
		if (auto unit_size = ata_statistic_get_data_unit_size(p.reported_name, unit_sizes)) {
			stat.value_bytes = data_amount_to_bytes(stat.value_int, unit_size.value());
		}

	} else if (p.is_value_type<int64_t>()) {
		if (auto unit_size = nvme_attribute_get_data_unit_size(p.generic_name, unit_sizes)) {
			const auto count = p.get_value<int64_t>();
			if (auto bytes = data_amount_to_bytes(count, unit_size.value())) {
				p.readable_value = hz::number_to_string_locale(count) + " [" + hz::format_size(bytes.value(), true) + "]";
			}
		}
	}
}



/// @}
