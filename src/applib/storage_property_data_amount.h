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

#ifndef STORAGE_PROPERTY_DATA_AMOUNT_H
#define STORAGE_PROPERTY_DATA_AMOUNT_H

#include <cstdint>
#include <optional>
#include <string>

#include "storage_property.h"
#include "storage_property_repository.h"



/// Device-wide unit sizes (in bytes) used to convert data amount counters to bytes.
struct DataAmountUnitSizes {
	std::optional<std::uint64_t> logical_sector_size;  ///< Logical sector size, from the "Sector Size(s)" property
	std::optional<std::uint64_t> host_writes_unit_size;  ///< smartctl's host_writes/bytes_per_unit (JSON, ATA and NVMe)
	std::optional<std::uint64_t> host_reads_unit_size;  ///< smartctl's host_reads/bytes_per_unit (JSON, ATA and NVMe)
};


/// Get the size (in bytes) of one unit of an ATA attribute raw value, if the attribute
/// reports an amount of data with an explicit unit in its name (as reported by smartctl).
/// For example, Host_Writes_32MiB -> 32 MiB, NAND_Writes_1GiB -> 1 GiB.
/// Attributes counting LBAs (e.g. Total_LBAs_Written) are not converted, since their units are vendor-specific.
[[nodiscard]] std::optional<std::uint64_t> ata_attribute_get_data_unit_size(const std::string& reported_name);


/// Get the size (in bytes) of one unit of an ATA Device Statistics value, if the statistic
/// reports an amount of data (e.g. "Logical Sectors Written" -> logical sector size).
/// These statistics count actual logical sectors, so std::nullopt is returned if
/// the unit size is unknown.
[[nodiscard]] std::optional<std::uint64_t> ata_statistic_get_data_unit_size(const std::string& reported_name,
		const DataAmountUnitSizes& unit_sizes);


/// Get the size (in bytes) of one unit of an NVMe health information value, if it reports
/// an amount of data (e.g. "Data Units Written" -> 512,000).
[[nodiscard]] std::optional<std::uint64_t> nvme_attribute_get_data_unit_size(const std::string& generic_name,
		const DataAmountUnitSizes& unit_sizes);


/// Parse the logical sector size from the "Sector Size(s)" property value,
/// e.g. "512 bytes logical, 4096 bytes physical" or "4096 bytes logical/physical".
[[nodiscard]] std::optional<std::uint64_t> parse_logical_sector_size(const std::string& sector_size_str);


/// Get the device-wide unit sizes from its properties.
[[nodiscard]] DataAmountUnitSizes storage_property_get_data_amount_unit_sizes(const StoragePropertyRepository& properties);


/// Convert a count of units to bytes.
/// \return std::nullopt if count is not positive, or on overflow.
[[nodiscard]] std::optional<std::uint64_t> data_amount_to_bytes(std::int64_t count, std::uint64_t unit_size);


/// If the property reports an amount of data, set its value in bytes, so that it's displayed
/// in human-readable form (e.g. "11,538,326,532 [5.91 TB]").
/// This handles ATA attributes, ATA Device Statistics and NVMe health information.
void storage_property_autoset_data_amount(StorageProperty& p, const DataAmountUnitSizes& unit_sizes);



#endif

/// @}
