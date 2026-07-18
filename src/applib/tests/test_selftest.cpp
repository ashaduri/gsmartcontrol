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

#include "applib/selftest.h"
#include "applib/storage_device.h"
#include <chrono>


TEST_CASE("SelfTest basic functionality", "[selftest]")
{
	using namespace std::literals;

	SECTION("Test type names are correct")
	{
		REQUIRE(SelfTest::get_test_displayable_name(SelfTest::TestType::ShortTest) != "[internal_error]");
		REQUIRE(SelfTest::get_test_displayable_name(SelfTest::TestType::LongTest) != "[internal_error]");
		REQUIRE(SelfTest::get_test_displayable_name(SelfTest::TestType::Conveyance) != "[internal_error]");
	}

	SECTION("Test status severity mapping")
	{
		REQUIRE(get_self_test_status_severity(SelfTestStatus::Unknown) == SelfTestStatusSeverity::None);
		REQUIRE(get_self_test_status_severity(SelfTestStatus::CompletedNoError) == SelfTestStatusSeverity::None);
		REQUIRE(get_self_test_status_severity(SelfTestStatus::ManuallyAborted) == SelfTestStatusSeverity::Warning);
		REQUIRE(get_self_test_status_severity(SelfTestStatus::Interrupted) == SelfTestStatusSeverity::Warning);
		REQUIRE(get_self_test_status_severity(SelfTestStatus::CompletedWithError) == SelfTestStatusSeverity::Error);
		REQUIRE(get_self_test_status_severity(SelfTestStatus::InProgress) == SelfTestStatusSeverity::None);
		REQUIRE(get_self_test_status_severity(SelfTestStatus::Reserved) == SelfTestStatusSeverity::None);
	}

	SECTION("Test not active by default")
	{
		auto device = std::make_shared<StorageDevice>("/dev/mock");
		SelfTest test(device, SelfTest::TestType::ShortTest);

		// Test should not be active immediately after construction
		REQUIRE(test.is_active() == false);
		REQUIRE(test.get_status() == SelfTestStatus::Unknown);
		REQUIRE(test.get_remaining_percent() == -1);
	}

	SECTION("Remaining seconds returns unknown when not running")
	{
		auto device = std::make_shared<StorageDevice>("/dev/mock");
		SelfTest test(device, SelfTest::TestType::ShortTest);

		// When no test is running, remaining seconds should be -1 (unknown)
		REQUIRE(test.get_remaining_seconds() == -1s);
	}

	SECTION("NVMe device without duration estimate")
	{
		auto device = std::make_shared<StorageDevice>("/dev/nvme0");
		device->set_detected_type(StorageDeviceDetectedType::Nvme);

		SelfTest test(device, SelfTest::TestType::ShortTest);

		// NVMe devices don't report duration, should return -1
		REQUIRE(test.get_min_duration_seconds() == -1s);

		// Without a running test, remaining should also be -1
		REQUIRE(test.get_remaining_seconds() == -1s);
	}

	SECTION("Test type is correctly stored")
	{
		auto device = std::make_shared<StorageDevice>("/dev/mock");

		SelfTest short_test(device, SelfTest::TestType::ShortTest);
		REQUIRE(short_test.get_test_type() == SelfTest::TestType::ShortTest);

		SelfTest long_test(device, SelfTest::TestType::LongTest);
		REQUIRE(long_test.get_test_type() == SelfTest::TestType::LongTest);

		SelfTest conveyance_test(device, SelfTest::TestType::Conveyance);
		REQUIRE(conveyance_test.get_test_type() == SelfTest::TestType::Conveyance);
	}

	SECTION("Poll time is initially unknown")
	{
		auto device = std::make_shared<StorageDevice>("/dev/mock");
		SelfTest test(device, SelfTest::TestType::ShortTest);

		// Before starting, poll time should be -1 (unknown)
		REQUIRE(test.get_poll_in_seconds() == -1s);
	}
}


TEST_CASE("SelfTest EXT enum helpers", "[selftest][enum_helpers]")
{
	SECTION("Status enum to string conversion")
	{
		// Verify that enum helper works for common statuses
		auto status_str = SelfTestStatusExt::get_displayable_name(SelfTestStatus::InProgress);
		REQUIRE(!status_str.empty());

		status_str = SelfTestStatusExt::get_displayable_name(SelfTestStatus::CompletedNoError);
		REQUIRE(!status_str.empty());

		status_str = SelfTestStatusExt::get_displayable_name(SelfTestStatus::Unknown);
		REQUIRE(!status_str.empty());
	}

	SECTION("Status enum storable name")
	{
		// Verify storable names (for serialization/deserialization)
		auto storable = SelfTestStatusExt::get_storable_name(SelfTestStatus::InProgress);
		REQUIRE(storable == "in_progress");

		storable = SelfTestStatusExt::get_storable_name(SelfTestStatus::ManuallyAborted);
		REQUIRE(storable == "manually_aborted");

		storable = SelfTestStatusExt::get_storable_name(SelfTestStatus::CompletedNoError);
		REQUIRE(storable == "completed_no_error");
	}

	SECTION("Default value is Unknown")
	{
		REQUIRE(SelfTestStatusExt::default_value == SelfTestStatus::Unknown);
	}
}


TEST_CASE("SelfTest support detection", "[selftest][support]")
{
	SECTION("ATA device capabilities check")
	{
		auto device = std::make_shared<StorageDevice>("/dev/sda");
		device->set_detected_type(StorageDeviceDetectedType::AtaSsd);

		// Without capability properties, tests should not be supported
		SelfTest short_test(device, SelfTest::TestType::ShortTest);
		REQUIRE(short_test.is_supported() == false);

		SelfTest long_test(device, SelfTest::TestType::LongTest);
		REQUIRE(long_test.is_supported() == false);
	}

	SECTION("NVMe conveyance test unsupported")
	{
		auto device = std::make_shared<StorageDevice>("/dev/nvme0");
		device->set_detected_type(StorageDeviceDetectedType::Nvme);

		// Conveyance test is not supported on NVMe
		SelfTest conveyance_test(device, SelfTest::TestType::Conveyance);
		REQUIRE(conveyance_test.is_supported() == false);
	}
}


/// @}

