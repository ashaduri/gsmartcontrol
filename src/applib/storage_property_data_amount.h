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



/// Get the size (in bytes) of one unit of an ATA attribute raw value, if the attribute
/// reports an amount of data, judging by its name (as reported by smartctl).
/// For example, Total_LBAs_Written -> 512, Host_Writes_32MiB -> 32 MiB, NAND_Writes_1GiB -> 1 GiB.
[[nodiscard]] std::optional<std::uint64_t> ata_attribute_get_data_unit_size(const std::string& reported_name);


/// Get the size (in bytes) of one unit of an ATA Device Statistics value, if the statistic
/// reports an amount of data (e.g. "Logical Sectors Written" -> 512).
[[nodiscard]] std::optional<std::uint64_t> ata_statistic_get_data_unit_size(const std::string& reported_name);


/// Get the size (in bytes) of one unit of an NVMe health information value, if it reports
/// an amount of data (e.g. "Data Units Written" -> 512,000).
[[nodiscard]] std::optional<std::uint64_t> nvme_attribute_get_data_unit_size(const std::string& generic_name);


/// Convert a count of units to bytes.
/// \return std::nullopt if count is not positive, or on overflow.
[[nodiscard]] std::optional<std::uint64_t> data_amount_to_bytes(std::int64_t count, std::uint64_t unit_size);


/// If the property reports an amount of data, set its value in bytes, so that it's displayed
/// in human-readable form (e.g. "11,538,326,532 [5.91 TB]").
/// This handles ATA attributes, ATA Device Statistics and NVMe health information.
void storage_property_autoset_data_amount(StorageProperty& p);



#endif

/// @}
