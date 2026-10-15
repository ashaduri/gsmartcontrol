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

#include "applib/smartctl_output_export.h"
#include <string>



namespace {

	const std::string json_with_text = R"json({
  "json_format_version": [1, 0],
  "smartctl": {
    "version": [7, 4],
    "output": [
      "smartctl 7.4 2023-08-01 r5530 [x86_64-linux] (local build)",
      "",
      "=== START OF INFORMATION SECTION ==="
    ]
  }
})json";

	const std::string json_without_text = R"json({
  "json_format_version": [1, 0],
  "smartctl": {
    "version": [7, 4]
  }
})json";

	const std::string text_output = "smartctl 7.4 2023-08-01 r5530 [x86_64-linux] (local build)\n"
			"\n=== START OF INFORMATION SECTION ===\n";

}



TEST_CASE("SmartctlOutputVariants", "[app][output_export]")
{
	SECTION("JSON with embedded text output")
	{
		const auto variants = smartctl_output_get_variants(json_with_text);
		REQUIRE(variants.has(SmartctlOutputFormat::Json));
		REQUIRE(variants.has(SmartctlOutputFormat::Text));
		REQUIRE(variants.get(SmartctlOutputFormat::Json) == json_with_text);
		REQUIRE(variants.get(SmartctlOutputFormat::Text) == text_output);
	}

	SECTION("JSON without embedded text output")
	{
		const auto variants = smartctl_output_get_variants(json_without_text);
		REQUIRE(variants.has(SmartctlOutputFormat::Json));
		REQUIRE_FALSE(variants.has(SmartctlOutputFormat::Text));
	}

	SECTION("Invalid JSON")
	{
		const auto variants = smartctl_output_get_variants("{ \"smartctl\": ");
		REQUIRE(variants.has(SmartctlOutputFormat::Json));
		REQUIRE_FALSE(variants.has(SmartctlOutputFormat::Text));
	}

	SECTION("Text output")
	{
		const auto variants = smartctl_output_get_variants(text_output);
		REQUIRE_FALSE(variants.has(SmartctlOutputFormat::Json));
		REQUIRE(variants.has(SmartctlOutputFormat::Text));
		REQUIRE(variants.get(SmartctlOutputFormat::Text) == text_output);
	}

	SECTION("Unknown output is treated as text")
	{
		const auto variants = smartctl_output_get_variants("Some error message");
		REQUIRE_FALSE(variants.has(SmartctlOutputFormat::Json));
		REQUIRE(variants.get(SmartctlOutputFormat::Text) == "Some error message");
	}
}



TEST_CASE("SmartctlOutputFileExtension", "[app][output_export]")
{
	using F = SmartctlOutputFormat;
	REQUIRE(smartctl_output_set_file_extension("a.json", F::Text) == hz::fs::path("a.txt"));
	REQUIRE(smartctl_output_set_file_extension("a.txt", F::Json) == hz::fs::path("a.json"));
	REQUIRE(smartctl_output_set_file_extension("a.json", F::Json) == hz::fs::path("a.json"));
	REQUIRE(smartctl_output_set_file_extension("a", F::Text) == hz::fs::path("a.txt"));
	REQUIRE(smartctl_output_set_file_extension("a.log", F::Text) == hz::fs::path("a.log.txt"));
	REQUIRE(smartctl_output_set_file_extension("dir.json/a", F::Json) == hz::fs::path("dir.json/a.json"));
}



TEST_CASE("SmartctlOutputSaveTarget", "[app][output_export]")
{
	using F = SmartctlOutputFormat;
	const auto both = smartctl_output_get_variants(json_with_text);
	const auto json_only = smartctl_output_get_variants(json_without_text);
	const auto text_only = smartctl_output_get_variants(text_output);

	SECTION("Format-specific filter overrides the extension")
	{
		// The default file name has .json extension; selecting the Text filter must save text.
		REQUIRE(smartctl_output_resolve_save_target("a.json", F::Text, F::Json, both)
				== std::pair(hz::fs::path("a.txt"), F::Text));
		REQUIRE(smartctl_output_resolve_save_target("a.txt", F::Json, F::Json, both)
				== std::pair(hz::fs::path("a.json"), F::Json));
		REQUIRE(smartctl_output_resolve_save_target("a", F::Text, F::Json, both)
				== std::pair(hz::fs::path("a.txt"), F::Text));
	}

	SECTION("Without a format-specific filter, the extension decides")
	{
		REQUIRE(smartctl_output_resolve_save_target("a.txt", std::nullopt, F::Json, both)
				== std::pair(hz::fs::path("a.txt"), F::Text));
		REQUIRE(smartctl_output_resolve_save_target("a.json", std::nullopt, F::Text, both)
				== std::pair(hz::fs::path("a.json"), F::Json));
	}

	SECTION("Without a format-specific filter or a known extension, the default format is used")
	{
		REQUIRE(smartctl_output_resolve_save_target("a", std::nullopt, F::Json, both)
				== std::pair(hz::fs::path("a.json"), F::Json));
		REQUIRE(smartctl_output_resolve_save_target("a", std::nullopt, F::Text, both)
				== std::pair(hz::fs::path("a.txt"), F::Text));
	}

	SECTION("Unavailable format falls back to the available one")
	{
		REQUIRE(smartctl_output_resolve_save_target("a.json", std::nullopt, F::Json, text_only)
				== std::pair(hz::fs::path("a.txt"), F::Text));
		REQUIRE(smartctl_output_resolve_save_target("a.txt", F::Text, F::Text, json_only)
				== std::pair(hz::fs::path("a.json"), F::Json));
	}
}



/// @}
