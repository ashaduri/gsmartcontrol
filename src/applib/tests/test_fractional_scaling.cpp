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

#include "applib/gui_utils.h"
#include <utility>



TEST_CASE("FractionalScalingDetection", "[app][scaling]")
{
	SECTION("Fractional scales")
	{
		REQUIRE(app_is_fractional_scaling(125));
		REQUIRE(app_is_fractional_scaling(150));
		REQUIRE(app_is_fractional_scaling(175));
		REQUIRE(app_is_fractional_scaling(225));
		REQUIRE(app_is_fractional_scaling(250));
	}

	SECTION("Integer scales")
	{
		REQUIRE_FALSE(app_is_fractional_scaling(100));
		REQUIRE_FALSE(app_is_fractional_scaling(200));
		REQUIRE_FALSE(app_is_fractional_scaling(300));
	}

	SECTION("Invalid and sub-100% scales")
	{
		REQUIRE_FALSE(app_is_fractional_scaling(0));
		REQUIRE_FALSE(app_is_fractional_scaling(-125));
		REQUIRE_FALSE(app_is_fractional_scaling(75));
		REQUIRE_FALSE(app_is_fractional_scaling(99));
	}
}



TEST_CASE("FractionalScalingPart", "[app][scaling]")
{
	REQUIRE(app_get_fractional_scaling_part_percent(125) == 25);
	REQUIRE(app_get_fractional_scaling_part_percent(150) == 50);
	REQUIRE(app_get_fractional_scaling_part_percent(250) == 50);
	REQUIRE(app_get_fractional_scaling_part_percent(225) == 25);

	REQUIRE(app_get_fractional_scaling_part_percent(100) == 0);
	REQUIRE(app_get_fractional_scaling_part_percent(200) == 0);
	REQUIRE(app_get_fractional_scaling_part_percent(75) == 0);
	REQUIRE(app_get_fractional_scaling_part_percent(0) == 0);
	REQUIRE(app_get_fractional_scaling_part_percent(-50) == 0);
}



TEST_CASE("FractionalScalingCorrection", "[app][scaling]")
{
	SECTION("Fractional scales")
	{
		REQUIRE(app_compute_fractional_scaling_correction(125) == Approx(1.25));
		REQUIRE(app_compute_fractional_scaling_correction(150) == Approx(1.5));
		REQUIRE(app_compute_fractional_scaling_correction(175) == Approx(1.75));
		// GTK already applies 2x at these scales, so only the remainder is corrected
		REQUIRE(app_compute_fractional_scaling_correction(225) == Approx(1.125));
		REQUIRE(app_compute_fractional_scaling_correction(250) == Approx(1.25));  // not 1.5
	}

	SECTION("No correction for integer, invalid or sub-100% scales")
	{
		REQUIRE(app_compute_fractional_scaling_correction(100) == 0.0);
		REQUIRE(app_compute_fractional_scaling_correction(200) == 0.0);
		REQUIRE(app_compute_fractional_scaling_correction(0) == 0.0);
		REQUIRE(app_compute_fractional_scaling_correction(-125) == 0.0);
		REQUIRE(app_compute_fractional_scaling_correction(75) == 0.0);  // used to divide by zero
		REQUIRE(app_compute_fractional_scaling_correction(50) == 0.0);
	}
}



TEST_CASE("FractionalScalingWindowSize", "[app][scaling]")
{
	SECTION("Scaled sizes")
	{
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, app_compute_fractional_scaling_correction(125))
				== std::pair(1000, 750));
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, app_compute_fractional_scaling_correction(150))
				== std::pair(1200, 900));
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, app_compute_fractional_scaling_correction(175))
				== std::pair(1400, 1050));
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, app_compute_fractional_scaling_correction(225))
				== std::pair(900, 675));
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, app_compute_fractional_scaling_correction(250))
				== std::pair(1000, 750));
	}

	SECTION("Rounding")
	{
		REQUIRE(app_apply_fractional_scaling_to_size(101, 3, 1.25) == std::pair(126, 4));  // 126.25, 3.75
	}

	SECTION("Unchanged when correction or size is not positive")
	{
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, 0.0) == std::pair(800, 600));
		REQUIRE(app_apply_fractional_scaling_to_size(800, 600, -1.0) == std::pair(800, 600));
		REQUIRE(app_apply_fractional_scaling_to_size(0, 600, 1.25) == std::pair(0, 600));
		REQUIRE(app_apply_fractional_scaling_to_size(800, -1, 1.25) == std::pair(800, -1));
	}
}



TEST_CASE("ScalingPercentFromDpi", "[app][scaling][dpi]")
{
	REQUIRE(app_compute_scaling_percent_from_dpi(96) == 100);
	REQUIRE(app_compute_scaling_percent_from_dpi(120) == 125);
	REQUIRE(app_compute_scaling_percent_from_dpi(144) == 150);
	REQUIRE(app_compute_scaling_percent_from_dpi(168) == 175);
	REQUIRE(app_compute_scaling_percent_from_dpi(192) == 200);
	REQUIRE(app_compute_scaling_percent_from_dpi(216) == 225);
	REQUIRE(app_compute_scaling_percent_from_dpi(240) == 250);
	REQUIRE(app_compute_scaling_percent_from_dpi(72) == 75);

	REQUIRE(app_compute_scaling_percent_from_dpi(0) == 0);
	REQUIRE(app_compute_scaling_percent_from_dpi(-96) == 0);
}



/// @}
