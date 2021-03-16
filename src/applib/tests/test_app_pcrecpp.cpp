/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_tests
/// \weakgroup applib_tests
/// @{

#include "catch2/catch_test_macros.hpp"
#include "applib/app_pcrecpp.h"



TEST_CASE("AppPcreCpp", "[app][pcrecpp]")
{
	// Default flags
	CHECK(app_pcre_get_options("").all_options() == 0x00500000);  // PCRE_NEWLINE_ANYCRLF

	REQUIRE(app_pcre_get_options("i").caseless() == true);
	REQUIRE(app_pcre_get_options("m").multiline() == true);
	REQUIRE(app_pcre_get_options("s").dotall() == true);
	REQUIRE(app_pcre_get_options("E").dollar_endonly() == true);
	REQUIRE(app_pcre_get_options("X").extra() == true);
	REQUIRE(app_pcre_get_options("x").extended() == true);
	REQUIRE(app_pcre_get_options("8").utf8() == true);
	REQUIRE(app_pcre_get_options("U").ungreedy() == true);
	REQUIRE(app_pcre_get_options("N").no_auto_capture() == true);

	// Multiple flags
	REQUIRE(app_pcre_get_options("imX").caseless() == true);
	REQUIRE(app_pcre_get_options("imX").multiline() == true);
	REQUIRE(app_pcre_get_options("imX").dotall() == false);
	REQUIRE(app_pcre_get_options("imX").dollar_endonly() == false);
	REQUIRE(app_pcre_get_options("imX").extra() == true);
	REQUIRE(app_pcre_get_options("imX").extended() == false);
	REQUIRE(app_pcre_get_options("imX").utf8() == false);
	REQUIRE(app_pcre_get_options("imX").ungreedy() == false);
	REQUIRE(app_pcre_get_options("imX").no_auto_capture() == false);

	// Invalid flags
	CHECK(app_pcre_get_options("Z").all_options() == 0x00500000);  // PCRE_NEWLINE_ANYCRLF
	REQUIRE(app_pcre_get_options("Zi").caseless() == true);
}



/// @}
