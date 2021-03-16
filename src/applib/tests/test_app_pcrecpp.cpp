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
	REQUIRE(app_pcre_get_options("i").caseless() == true);

}



/// @}
