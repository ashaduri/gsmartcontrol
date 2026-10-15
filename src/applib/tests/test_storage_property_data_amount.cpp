/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_tests
/// \weakgroup applib_tests
/// @{

#include "catch2/catch.hpp"

#include "applib/storage_property_data_amount.h"
#include "hz/string_num.h"
#include <cstdint>
#include <limits>
#include <string>



namespace {

	constexpr std::uint64_t mib = 1024ULL * 1024;
	constexpr std::uint64_t gib = 1024ULL * 1024 * 1024;


	StorageProperty make_attribute(const std::string& reported_name, std::int64_t raw, const std::string& raw_str = {})
	{
		AtaStorageAttribute attr;
		attr.id = 241;
		attr.raw_value_int = raw;
		attr.raw_value = raw_str.empty() ? hz::number_to_string_nolocale(raw) : raw_str;
		StorageProperty p;
		p.reported_name = reported_name;
		p.section = StoragePropertySection::AtaAttributes;
		p.value = attr;
		return p;
	}

}



TEST_CASE("AtaAttributeDataUnitSize", "[app][data_amount]")
{
	SECTION("Sector / LBA counts")
	{
		REQUIRE(ata_attribute_get_data_unit_size("Total_LBAs_Written") == 512);
		REQUIRE(ata_attribute_get_data_unit_size("Total_LBAs_Read") == 512);
		REQUIRE(ata_attribute_get_data_unit_size("Host_LBAs_Written") == 512);
		REQUIRE(ata_attribute_get_data_unit_size("Write_Sectors_Tot_Ct") == 512);
		REQUIRE(ata_attribute_get_data_unit_size("Read_Sectors_Tot_Ct") == 512);
		REQUIRE(ata_attribute_get_data_unit_size("Total_Host_Sector_Write") == 512);
	}

	SECTION("Explicit units")
	{
		REQUIRE(ata_attribute_get_data_unit_size("Host_Writes_32MiB") == 32 * mib);
		REQUIRE(ata_attribute_get_data_unit_size("Host_Reads_32MiB") == 32 * mib);
		REQUIRE(ata_attribute_get_data_unit_size("Flash_Writes_32MiB") == 32 * mib);
		REQUIRE(ata_attribute_get_data_unit_size("NAND_Writes_1GiB") == gib);
		REQUIRE(ata_attribute_get_data_unit_size("Lifetime_Writes_GiB") == gib);
		REQUIRE(ata_attribute_get_data_unit_size("Total_NAND_Prog_Ct_GiB") == gib);
		REQUIRE(ata_attribute_get_data_unit_size("Host_Writes_MiB") == mib);
	}

	SECTION("Not data amounts")
	{
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Reallocated_Sector_Ct").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Current_Pending_Sector").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Offline_Uncorrectable").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Raw_Read_Error_Rate").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Sector_Read_Errors").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Power_On_Hours").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Unknown_SSD_Attribute").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Host_Writes_0MiB").has_value());
	}
}



TEST_CASE("OtherDataUnitSizes", "[app][data_amount]")
{
	REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Written") == 512);
	REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Read") == 512);
	REQUIRE_FALSE(ata_statistic_get_data_unit_size("Number of Reallocated Logical Sectors").has_value());

	REQUIRE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/data_units_written") == 512000);
	REQUIRE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/data_units_read") == 512000);
	REQUIRE_FALSE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/host_reads").has_value());
}



TEST_CASE("DataAmountToBytes", "[app][data_amount]")
{
	REQUIRE(data_amount_to_bytes(10, 512) == 5120);
	REQUIRE_FALSE(data_amount_to_bytes(0, 512).has_value());
	REQUIRE_FALSE(data_amount_to_bytes(-1, 512).has_value());
	REQUIRE_FALSE(data_amount_to_bytes(10, 0).has_value());
	REQUIRE_FALSE(data_amount_to_bytes(std::numeric_limits<std::int64_t>::max(), 512).has_value());  // overflow
}



TEST_CASE("AutosetDataAmount", "[app][data_amount]")
{
	SECTION("ATA attribute")
	{
		auto p = make_attribute("Total_LBAs_Written", 11538326532);
		storage_property_autoset_data_amount(p);
		const auto& attr = p.get_value<AtaStorageAttribute>();
		REQUIRE(attr.raw_value_bytes == 11538326532ULL * 512);
		REQUIRE(attr.format_raw_value().find("[5.91 TB]") != std::string::npos);
	}

	SECTION("ATA attribute with non-numeric raw value is left alone")
	{
		auto p = make_attribute("Total_LBAs_Written", 100, "100 (some info)");
		storage_property_autoset_data_amount(p);
		REQUIRE_FALSE(p.get_value<AtaStorageAttribute>().raw_value_bytes.has_value());
	}

	SECTION("Unrelated ATA attribute is left alone")
	{
		auto p = make_attribute("Power_On_Hours", 1000);
		storage_property_autoset_data_amount(p);
		REQUIRE_FALSE(p.get_value<AtaStorageAttribute>().raw_value_bytes.has_value());
	}

	SECTION("ATA statistic")
	{
		AtaStorageStatistic stat;
		stat.value_int = 2000000000;
		stat.value = "2000000000";
		StorageProperty p;
		p.reported_name = "Logical Sectors Written";
		p.section = StoragePropertySection::Statistics;
		p.value = stat;
		storage_property_autoset_data_amount(p);
		REQUIRE(p.get_value<AtaStorageStatistic>().value_bytes == 2000000000ULL * 512);
		REQUIRE(p.get_value<AtaStorageStatistic>().format_value().find("[1.02 TB]") != std::string::npos);
	}

	SECTION("NVMe data units")
	{
		StorageProperty p;
		p.generic_name = "nvme_smart_health_information_log/data_units_written";
		p.value = std::int64_t(16174087);
		storage_property_autoset_data_amount(p);
		REQUIRE(p.readable_value.find("[8.28 TB]") != std::string::npos);
	}
}



/// @}
