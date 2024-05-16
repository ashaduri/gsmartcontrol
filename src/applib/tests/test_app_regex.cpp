/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2024 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_tests
/// \weakgroup applib_tests
/// @{

// Catch2 v3
//#include "catch2/catch_test_macros.hpp"

// Catch2 v2
#include "catch2/catch.hpp"

#include "applib/app_regex.h"
#include <regex>



TEST_CASE("AppRegexFlags", "[app][regex]")
{
	// Default flags
	REQUIRE(static_cast<bool>(app_regex_get_options("i") & std::regex::icase) == true);
	REQUIRE(static_cast<bool>(app_regex_get_options("m") & std::regex::multiline) == true);

	// Multiple flags
	REQUIRE(static_cast<bool>(app_regex_get_options("im") & std::regex::icase) == true);
	REQUIRE(static_cast<bool>(app_regex_get_options("im") & std::regex::multiline) == true);
}



TEST_CASE("AppRegexBasic", "[app][regex]")
{
	const std::vector<std::string> input_lines = {
		"major minor",
		"31  0     128 mtdblock0",
		"3     1    1638598 ide/host0/bus0/target0/lun0/part1 0 0 0 0 0 0 0 0 0 0 0",
		"\t8     0  156290904 sda",
	};

	{
		std::smatch matches;
		const bool matched = app_regex_partial_match(R"(/^[ \t]*[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+([^ \t\n]+)/)", input_lines.at(0), matches);
		REQUIRE(matched == false);
	}
	{
		std::smatch matches;
		const bool matched = app_regex_partial_match(R"(/^[ \t]*[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+([^ \t\n]+)/)", input_lines.at(1), matches);
		REQUIRE(matched == true);
		REQUIRE(matches.size() == 2);
		REQUIRE(matches[1].str() == "mtdblock0");
	}
	{
		std::smatch matches;
		const bool matched = app_regex_partial_match(R"(/^[ \t]*[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+([^ \t\n]+)/)", input_lines.at(2), matches);
		REQUIRE(matched == true);
		REQUIRE(matches.size() == 2);
		REQUIRE(matches[1].str() == "ide/host0/bus0/target0/lun0/part1");
	}
	{
		std::smatch matches;
		const bool matched = app_regex_partial_match(R"(/^[ \t]*[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+[^ \t\n]+[ \t]+([^ \t\n]+)/)", input_lines.at(3), matches);
		REQUIRE(matched == true);
		REQUIRE(matches.size() == 2);
		REQUIRE(matches[1].str() == "sda");
	}
}



TEST_CASE("AppRegexLines", "[app][regex]")
{
	const std::string input = R"(Device Model:     ST3500630AS)";

	std::string name, value;
	const bool matched = app_regex_full_match("/^([^:]+):[ \\t]+(.*)$/i", input, {&name, &value});
	REQUIRE(matched == true);
	REQUIRE(name == "Device Model");
	REQUIRE(value == "ST3500630AS");
}



TEST_CASE("AppRegexMultiline", "[app][regex]")
{
	const std::string input = R"(
Copyright (C) 2002-23, Bruce Allen, Christian Franke, www.smartmontools.org

=== START OF OFFLINE IMMEDIATE AND SELF-TEST SECTION ===
Sending command: "Execute SMART Short self-test routine immediately in off-line mode".
Drive command "Execute SMART Short self-test routine immediately in off-line mode" successful.
Testing has begun.
Please wait 2 minutes for test to complete.
Test will complete after Thu May 16 14:31:06 2024 +04
Use smartctl -X to abort test.
)";

	const bool matched = app_regex_partial_match(R"(/^Drive command .* successful\.\nTesting has begun\.$/mi)", input);
	REQUIRE(matched == true);
}





/// @}
