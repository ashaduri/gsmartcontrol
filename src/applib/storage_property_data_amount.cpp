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

	/// Size of a logical sector (LBA) assumed for ATA counters.
	/// SATA SSDs use 512-byte logical sectors (4Kn SATA SSDs are practically nonexistent),
	/// and the attributes counting LBAs use 512-byte units regardless.
	constexpr std::uint64_t ata_sector_size = 512;

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

	// Sector / LBA counts: Total_LBAs_Written, Total_LBAs_Read, Write_Sectors_Tot_Ct, Total_Host_Sector_Write, ...
	if (app_regex_partial_match("/(^|_)(LBAs?|Sectors?)_(Write|Written|Read)(_|$)/i", reported_name)
			|| app_regex_partial_match("/(^|_)(Write|Written|Read)_(LBAs?|Sectors?)(_|$)/i", reported_name)) {
		return ata_sector_size;
	}

	return std::nullopt;
}



std::optional<std::uint64_t> ata_statistic_get_data_unit_size(const std::string& reported_name)
{
	if (reported_name == "Logical Sectors Written" || reported_name == "Logical Sectors Read") {
		return ata_sector_size;
	}
	return std::nullopt;
}



std::optional<std::uint64_t> nvme_attribute_get_data_unit_size(const std::string& generic_name)
{
	if (generic_name == "nvme_smart_health_information_log/data_units_read"
			|| generic_name == "nvme_smart_health_information_log/data_units_written") {
		return nvme_data_unit_size;
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



void storage_property_autoset_data_amount(StorageProperty& p)
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
		if (auto unit_size = ata_statistic_get_data_unit_size(p.reported_name)) {
			stat.value_bytes = data_amount_to_bytes(stat.value_int, unit_size.value());
		}

	} else if (p.is_value_type<int64_t>()) {
		if (auto unit_size = nvme_attribute_get_data_unit_size(p.generic_name)) {
			const auto count = p.get_value<int64_t>();
			if (auto bytes = data_amount_to_bytes(count, unit_size.value())) {
				p.readable_value = hz::number_to_string_locale(count) + " [" + hz::format_size(bytes.value(), true) + "]";
			}
		}
	}
}



/// @}
