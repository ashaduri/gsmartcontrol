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
#include "applib/smartctl_json_ata_parser.h"
#include "applib/smartctl_json_nvme_parser.h"
#include "hz/string_num.h"
#include <cstdint>
#include <limits>
#include <optional>
#include <string>



namespace {

	constexpr std::uint64_t mib = 1024ULL * 1024;
	constexpr std::uint64_t gib = 1024ULL * 1024 * 1024;


	DataAmountUnitSizes make_unit_sizes(std::optional<std::uint64_t> logical_sector_size,
			std::optional<std::uint64_t> host_writes_unit_size = std::nullopt,
			std::optional<std::uint64_t> host_reads_unit_size = std::nullopt)
	{
		DataAmountUnitSizes unit_sizes;
		unit_sizes.logical_sector_size = logical_sector_size;
		unit_sizes.host_writes_unit_size = host_writes_unit_size;
		unit_sizes.host_reads_unit_size = host_reads_unit_size;
		return unit_sizes;
	}


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


	StorageProperty make_statistic(const std::string& reported_name, std::int64_t value)
	{
		AtaStorageStatistic stat;
		stat.value_int = value;
		stat.value = hz::number_to_string_nolocale(value);
		StorageProperty p;
		p.reported_name = reported_name;
		p.section = StoragePropertySection::Statistics;
		p.value = stat;
		return p;
	}

}



TEST_CASE("AtaAttributeDataUnitSize", "[app][data_amount]")
{
	SECTION("Sector / LBA counts have vendor-specific units and are not converted")
	{
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Total_LBAs_Written").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Total_LBAs_Read").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Host_LBAs_Written").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Write_Sectors_Tot_Ct").has_value());
		REQUIRE_FALSE(ata_attribute_get_data_unit_size("Total_Host_Sector_Write").has_value());
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
	SECTION("ATA statistics use the logical sector size")
	{
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Written", make_unit_sizes(512)) == 512);
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Read", make_unit_sizes(512)) == 512);
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Written", make_unit_sizes(4096)) == 4096);
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Read", make_unit_sizes(4096)) == 4096);
		REQUIRE_FALSE(ata_statistic_get_data_unit_size("Logical Sectors Written", make_unit_sizes(std::nullopt)).has_value());
		REQUIRE_FALSE(ata_statistic_get_data_unit_size("Number of Reallocated Logical Sectors", make_unit_sizes(512)).has_value());
	}

	SECTION("ATA statistics prefer smartctl-reported unit sizes")
	{
		const auto unit_sizes = make_unit_sizes(512, 4096, 4096);
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Written", unit_sizes) == 4096);
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Read", unit_sizes) == 4096);
		REQUIRE(ata_statistic_get_data_unit_size("Logical Sectors Written", make_unit_sizes(std::nullopt, 4096)) == 4096);
	}

	SECTION("NVMe data units")
	{
		const auto none = make_unit_sizes(std::nullopt);
		REQUIRE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/data_units_written", none) == 512000);
		REQUIRE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/data_units_read", none) == 512000);
		REQUIRE_FALSE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/host_reads", none).has_value());

		const auto reported = make_unit_sizes(std::nullopt, 1000, 2000);
		REQUIRE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/data_units_written", reported) == 1000);
		REQUIRE(nvme_attribute_get_data_unit_size("nvme_smart_health_information_log/data_units_read", reported) == 2000);
	}
}



TEST_CASE("DataAmountUnitSizes", "[app][data_amount]")
{
	SECTION("Parsing logical sector size")
	{
		REQUIRE(parse_logical_sector_size("512 bytes logical, 4096 bytes physical") == 512);
		REQUIRE(parse_logical_sector_size("4096 bytes logical/physical") == 4096);
		REQUIRE(parse_logical_sector_size("512 bytes logical/physical") == 512);
		REQUIRE_FALSE(parse_logical_sector_size("4096 bytes physical").has_value());
		REQUIRE_FALSE(parse_logical_sector_size("").has_value());
	}

	SECTION("From properties")
	{
		StoragePropertyRepository properties;
		{
			const auto unit_sizes = storage_property_get_data_amount_unit_sizes(properties);
			REQUIRE_FALSE(unit_sizes.logical_sector_size.has_value());
			REQUIRE_FALSE(unit_sizes.host_writes_unit_size.has_value());
			REQUIRE_FALSE(unit_sizes.host_reads_unit_size.has_value());
		}

		StorageProperty p;
		p.set_name("physical_block_size/_and/logical_block_size", "Sector Size");
		p.readable_value = "4096 bytes logical, 4096 bytes physical";
		p.value = p.readable_value;
		properties.add_property(p);

		StorageProperty w;
		w.set_name("host_writes/bytes_per_unit", "Host Writes Unit Size");
		w.value = std::int64_t(4096);
		properties.add_property(w);

		const auto unit_sizes = storage_property_get_data_amount_unit_sizes(properties);
		REQUIRE(unit_sizes.logical_sector_size == 4096);
		REQUIRE(unit_sizes.host_writes_unit_size == 4096);
		REQUIRE_FALSE(unit_sizes.host_reads_unit_size.has_value());
	}

	SECTION("From smartctl ATA JSON output")
	{
		SmartctlJsonAtaParser parser;
		REQUIRE(parser.parse(R"({
			"smartctl": {"version": [7, 5]},
			"model_name": "Test HDD",
			"logical_block_size": 4096,
			"physical_block_size": 4096,
			"host_writes": {"units": 2000000000, "bytes_per_unit": 4096, "string": "8.19 TB"},
			"host_reads": {"units": 3000000000, "bytes_per_unit": 4096, "string": "12.2 TB"}
		})").has_value());
		const auto unit_sizes = storage_property_get_data_amount_unit_sizes(parser.get_property_repository());
		REQUIRE(unit_sizes.logical_sector_size == 4096);
		REQUIRE(unit_sizes.host_writes_unit_size == 4096);
		REQUIRE(unit_sizes.host_reads_unit_size == 4096);
		REQUIRE_FALSE(parser.get_property_repository().lookup_property("host_writes/bytes_per_unit").show_in_ui);
	}

	SECTION("From smartctl NVMe JSON output")
	{
		SmartctlJsonNvmeParser parser;
		REQUIRE(parser.parse(R"({
			"smartctl": {"version": [7, 5]},
			"model_name": "Test NVMe",
			"host_writes": {"units": 16174087, "bytes_per_unit": 512000, "string": "8.28 TB"},
			"host_reads": {"units": 70813348, "bytes_per_unit": 512000, "string": "36.2 TB"}
		})").has_value());
		const auto unit_sizes = storage_property_get_data_amount_unit_sizes(parser.get_property_repository());
		REQUIRE(unit_sizes.host_writes_unit_size == 512000);
		REQUIRE(unit_sizes.host_reads_unit_size == 512000);
		REQUIRE_FALSE(parser.get_property_repository().lookup_property("host_reads/bytes_per_unit").show_in_ui);
	}
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
	SECTION("ATA attribute with explicit unit")
	{
		auto p = make_attribute("Host_Writes_32MiB", 100000);
		storage_property_autoset_data_amount(p, make_unit_sizes(512));
		const auto& attr = p.get_value<AtaStorageAttribute>();
		REQUIRE(attr.raw_value_bytes == 100000ULL * 32 * mib);
		REQUIRE(attr.format_raw_value().find("[3.36 TB]") != std::string::npos);
	}

	SECTION("ATA attribute counting LBAs is left alone")
	{
		auto p = make_attribute("Total_LBAs_Written", 11538326532);
		storage_property_autoset_data_amount(p, make_unit_sizes(512, 512, 512));
		REQUIRE_FALSE(p.get_value<AtaStorageAttribute>().raw_value_bytes.has_value());
	}

	SECTION("ATA attribute with non-numeric raw value is left alone")
	{
		auto p = make_attribute("Host_Writes_32MiB", 100, "100 (some info)");
		storage_property_autoset_data_amount(p, make_unit_sizes(512));
		REQUIRE_FALSE(p.get_value<AtaStorageAttribute>().raw_value_bytes.has_value());
	}

	SECTION("Unrelated ATA attribute is left alone")
	{
		auto p = make_attribute("Power_On_Hours", 1000);
		storage_property_autoset_data_amount(p, make_unit_sizes(512));
		REQUIRE_FALSE(p.get_value<AtaStorageAttribute>().raw_value_bytes.has_value());
	}

	SECTION("ATA statistic, 512-byte logical sectors")
	{
		auto p = make_statistic("Logical Sectors Written", 2000000000);
		storage_property_autoset_data_amount(p, make_unit_sizes(512));
		REQUIRE(p.get_value<AtaStorageStatistic>().value_bytes == 2000000000ULL * 512);
		REQUIRE(p.get_value<AtaStorageStatistic>().format_value().find("[1.02 TB]") != std::string::npos);
	}

	SECTION("ATA statistic, 4096-byte logical sectors")
	{
		auto p = make_statistic("Logical Sectors Written", 2000000000);
		storage_property_autoset_data_amount(p, make_unit_sizes(4096));
		REQUIRE(p.get_value<AtaStorageStatistic>().value_bytes == 2000000000ULL * 4096);
		REQUIRE(p.get_value<AtaStorageStatistic>().format_value().find("[8.19 TB]") != std::string::npos);
	}

	SECTION("ATA statistic, smartctl-reported unit size")
	{
		auto p = make_statistic("Logical Sectors Read", 2000000000);
		storage_property_autoset_data_amount(p, make_unit_sizes(std::nullopt, std::nullopt, 4096));
		REQUIRE(p.get_value<AtaStorageStatistic>().value_bytes == 2000000000ULL * 4096);
	}

	SECTION("ATA statistic with unknown logical sector size is left alone")
	{
		auto p = make_statistic("Logical Sectors Written", 2000000000);
		storage_property_autoset_data_amount(p, make_unit_sizes(std::nullopt));
		REQUIRE_FALSE(p.get_value<AtaStorageStatistic>().value_bytes.has_value());
	}

	SECTION("NVMe data units")
	{
		StorageProperty p;
		p.generic_name = "nvme_smart_health_information_log/data_units_written";
		p.value = std::int64_t(16174087);
		storage_property_autoset_data_amount(p, make_unit_sizes(std::nullopt));
		REQUIRE(p.readable_value.find("[8.28 TB]") != std::string::npos);
	}

	SECTION("NVMe data units, smartctl-reported unit size")
	{
		StorageProperty p;
		p.generic_name = "nvme_smart_health_information_log/data_units_written";
		p.value = std::int64_t(16174087);
		storage_property_autoset_data_amount(p, make_unit_sizes(std::nullopt, 512000));
		REQUIRE(p.readable_value.find("[8.28 TB]") != std::string::npos);
	}
}



/// @}
