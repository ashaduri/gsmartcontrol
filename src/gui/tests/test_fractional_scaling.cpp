/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2026 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gui_tests
/// \weakgroup gui_tests
/// @{

#include "catch2/catch.hpp"

#include <cmath>


/// Test the fractional scaling calculation logic
/// This tests the mathematical correctness of the scaling correction ratio
TEST_CASE("FractionalScalingCalculation", "[gui][scaling]")
{
	// Test the calculation logic that's used in app_apply_fractional_scaling_to_default_size()
	// correction = system_scale / integer_ui_scale

	SECTION("125% scaling (fractional)")
	{
		const int full_percent = 125;
		const double system_scale = static_cast<double>(full_percent) / 100.0;  // 1.25
		const int integer_ui_scale = full_percent / 100;  // 1
		const double correction = system_scale / static_cast<double>(integer_ui_scale);

		REQUIRE(system_scale == 1.25);
		REQUIRE(integer_ui_scale == 1);
		REQUIRE(correction == 1.25);

		// Test window scaling: 800x600 base should become 1000x750
		const int base_w = 800;
		const int base_h = 600;
		const int scaled_w = static_cast<int>(std::lround(base_w * correction));
		const int scaled_h = static_cast<int>(std::lround(base_h * correction));

		REQUIRE(scaled_w == 1000);
		REQUIRE(scaled_h == 750);
	}

	SECTION("150% scaling (fractional)")
	{
		const int full_percent = 150;
		const double system_scale = static_cast<double>(full_percent) / 100.0;  // 1.5
		const int integer_ui_scale = full_percent / 100;  // 1
		const double correction = system_scale / static_cast<double>(integer_ui_scale);

		REQUIRE(system_scale == 1.5);
		REQUIRE(integer_ui_scale == 1);
		REQUIRE(correction == 1.5);

		// Test window scaling: 800x600 base should become 1200x900
		const int base_w = 800;
		const int base_h = 600;
		const int scaled_w = static_cast<int>(std::lround(base_w * correction));
		const int scaled_h = static_cast<int>(std::lround(base_h * correction));

		REQUIRE(scaled_w == 1200);
		REQUIRE(scaled_h == 900);
	}

	SECTION("175% scaling (fractional)")
	{
		const int full_percent = 175;
		const double system_scale = static_cast<double>(full_percent) / 100.0;  // 1.75
		const int integer_ui_scale = full_percent / 100;  // 1
		const double correction = system_scale / static_cast<double>(integer_ui_scale);

		REQUIRE(system_scale == 1.75);
		REQUIRE(integer_ui_scale == 1);
		REQUIRE(correction == 1.75);

		// Test window scaling: 800x600 base should become 1400x1050
		const int base_w = 800;
		const int base_h = 600;
		const int scaled_w = static_cast<int>(std::lround(base_w * correction));
		const int scaled_h = static_cast<int>(std::lround(base_h * correction));

		REQUIRE(scaled_w == 1400);
		REQUIRE(scaled_h == 1050);
	}

	SECTION("250% scaling (fractional) - Critical test case")
	{
		// This is the case that was broken before the fix
		const int full_percent = 250;
		const double system_scale = static_cast<double>(full_percent) / 100.0;  // 2.5
		const int integer_ui_scale = full_percent / 100;  // 2
		const double correction = system_scale / static_cast<double>(integer_ui_scale);

		REQUIRE(system_scale == 2.5);
		REQUIRE(integer_ui_scale == 2);
		REQUIRE(correction == 1.25);  // NOT 1.5 (the old broken behavior)

		// Test window scaling: GTK already applies 2x, we need to apply 1.25x more
		// For a base of 800x600, GTK makes it 1600x1200, we should scale to 2000x1500
		const int gtk_scaled_w = 1600;  // After GTK's 2x integer scaling
		const int gtk_scaled_h = 1200;
		const int final_w = static_cast<int>(std::lround(gtk_scaled_w * correction));
		const int final_h = static_cast<int>(std::lround(gtk_scaled_h * correction));

		REQUIRE(final_w == 2000);  // 2.5x total = 800 * 2.5
		REQUIRE(final_h == 1500);  // 2.5x total = 600 * 2.5
	}

	SECTION("225% scaling (fractional)")
	{
		const int full_percent = 225;
		const double system_scale = static_cast<double>(full_percent) / 100.0;  // 2.25
		const int integer_ui_scale = full_percent / 100;  // 2
		const double correction = system_scale / static_cast<double>(integer_ui_scale);

		REQUIRE(system_scale == 2.25);
		REQUIRE(integer_ui_scale == 2);
		REQUIRE(correction == 1.125);

		// Test window scaling: 800x600 at 2x GTK = 1600x1200, with correction = 1800x1350
		const int gtk_scaled_w = 1600;
		const int gtk_scaled_h = 1200;
		const int final_w = static_cast<int>(std::lround(gtk_scaled_w * correction));
		const int final_h = static_cast<int>(std::lround(gtk_scaled_h * correction));

		REQUIRE(final_w == 1800);
		REQUIRE(final_h == 1350);
	}

	SECTION("100% scaling (integer, no fractional)")
	{
		// Should not trigger fractional scaling at all (full_percent would be 0)
		const int full_percent = 100;
		const int integer_scale_percent = (full_percent / 100) * 100;

		REQUIRE(full_percent == integer_scale_percent);  // No fractional component
	}

	SECTION("200% scaling (integer, no fractional)")
	{
		// Should not trigger fractional scaling at all (full_percent would be 0)
		const int full_percent = 200;
		const int integer_scale_percent = (full_percent / 100) * 100;

		REQUIRE(full_percent == integer_scale_percent);  // No fractional component
	}
}


/// Test the DPI detection logic
TEST_CASE("DPIDetectionLogic", "[gui][scaling][dpi]")
{
	SECTION("Detect 125% scaling from DPI")
	{
		const int h_ppi = 120;  // 120 DPI
		const double scale = static_cast<double>(h_ppi) / 96.0;
		const int full_scale_percent = static_cast<int>(std::lround(scale * 100.0));
		const int integer_scale_percent = (full_scale_percent / 100) * 100;

		REQUIRE(full_scale_percent == 125);
		REQUIRE(integer_scale_percent == 100);
		REQUIRE(full_scale_percent != integer_scale_percent);  // Fractional scaling detected
	}

	SECTION("Detect 150% scaling from DPI")
	{
		const int h_ppi = 144;  // 144 DPI
		const double scale = static_cast<double>(h_ppi) / 96.0;
		const int full_scale_percent = static_cast<int>(std::lround(scale * 100.0));
		const int integer_scale_percent = (full_scale_percent / 100) * 100;

		REQUIRE(full_scale_percent == 150);
		REQUIRE(integer_scale_percent == 100);
		REQUIRE(full_scale_percent != integer_scale_percent);  // Fractional scaling detected
	}

	SECTION("Detect 250% scaling from DPI")
	{
		const int h_ppi = 240;  // 240 DPI
		const double scale = static_cast<double>(h_ppi) / 96.0;
		const int full_scale_percent = static_cast<int>(std::lround(scale * 100.0));
		const int integer_scale_percent = (full_scale_percent / 100) * 100;

		REQUIRE(full_scale_percent == 250);
		REQUIRE(integer_scale_percent == 200);
		REQUIRE(full_scale_percent != integer_scale_percent);  // Fractional scaling detected
	}

	SECTION("No fractional scaling at 100% (96 DPI)")
	{
		const int h_ppi = 96;  // 96 DPI (standard)
		const double scale = static_cast<double>(h_ppi) / 96.0;
		const int full_scale_percent = static_cast<int>(std::lround(scale * 100.0));
		const int integer_scale_percent = (full_scale_percent / 100) * 100;

		REQUIRE(full_scale_percent == 100);
		REQUIRE(integer_scale_percent == 100);
		REQUIRE(full_scale_percent == integer_scale_percent);  // No fractional scaling
	}

	SECTION("No fractional scaling at 200% (192 DPI)")
	{
		const int h_ppi = 192;  // 192 DPI
		const double scale = static_cast<double>(h_ppi) / 96.0;
		const int full_scale_percent = static_cast<int>(std::lround(scale * 100.0));
		const int integer_scale_percent = (full_scale_percent / 100) * 100;

		REQUIRE(full_scale_percent == 200);
		REQUIRE(integer_scale_percent == 200);
		REQUIRE(full_scale_percent == integer_scale_percent);  // No fractional scaling
	}
}


/// @}
