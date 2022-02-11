/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2022 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz_tests
/// \weakgroup hz_tests
/// @{

// Catch2 v3
//#include "catch2/catch_test_macros.hpp"

// Catch2 v2
#include "catch2/catch.hpp"

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation via std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "hz/format_unit.h"

#include <cstdint>



TEST_CASE("FormatUnitSize", "[hz][formatting]")
{
	REQUIRE(hz::format_size(3LL * 1024 * 1024) == "3.00 MiB");
	REQUIRE(hz::format_size(4LL * 1000 * 1000, true, true) == "4.00 Mbit");
	REQUIRE(hz::format_size(100LL * 1000 * 1000 * 1000) == "93.13 GiB");  // aka how the hard disk manufactures screw you
	REQUIRE(hz::format_size(100LL * 1024 * 1024, true) == "104.86 MB");  // 100 MiB in decimal MB

	REQUIRE(hz::format_size(5) == "5 B");
	REQUIRE(hz::format_size(6, true, true) == "6 bit");
	REQUIRE(hz::format_size(uint64_t(2.5 * 1024)) == "2.50 KiB");
	REQUIRE(hz::format_size(uint64_t(2.5 * 1024 * 1024)) == "2.50 MiB");
	REQUIRE(hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024)) == "2.50 GiB");
	REQUIRE(hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024 * 1024)) == "2.50 TiB");
	REQUIRE(hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024 * 1024 * 1024)) == "2.50 PiB");
	REQUIRE(hz::format_size(uint64_t(2.5 * 1024 * 1024 * 1024 * 1024 * 1024 * 1024)) == "2.50 EiB");

	// Common size of 1 TiB hdd in decimal
	REQUIRE(hz::format_size(uint64_t(1000204886016ULL), true) == "1.00 TB");
}



TEST_CASE("FormatUnitTimeLength", "[hz][formatting]")
{
	using namespace std::literals;
	using days = std::chrono::duration<int, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

	REQUIRE(hz::format_time_length(5s) == "5 sec");
	REQUIRE(hz::format_time_length(90s) == "90 sec");
	REQUIRE(hz::format_time_length(5min + 30s) == "6 min");  // rounded to the nearest minute
	REQUIRE(hz::format_time_length(130min) == "2 h 10 min");
	REQUIRE(hz::format_time_length(5h + 30min) == "5 h 30 min");
	REQUIRE(hz::format_time_length(10h + 40min) == "11 h");  // rounded to nearest hour
	REQUIRE(hz::format_time_length(24h + 20min) == "24 h");  // rounded to nearest hour
	REQUIRE(hz::format_time_length(130h + 30min) == "5 d 11 h");
	REQUIRE(hz::format_time_length(days(5) + 15h + 30min) == "5 d 16 h");  // rounded to nearest hour
	REQUIRE(hz::format_time_length(days(20) - 8h) == "20 d");  // rounded to nearest day
}





/// @}
