/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2022 Alexander Shaduri <ashaduri@gmail.com>
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

#include "applib/smartctl_parser.h"



TEST_CASE("SmartctlFormatDetection", "[app][parser]")
{
	REQUIRE(SmartctlParser::detect_output_type({}).error() == SmartctlParserError::EmptyInput);

	REQUIRE(SmartctlParser::detect_output_type("smart").error() == SmartctlParserError::UnsupportedFormat);

	REQUIRE(SmartctlParser::detect_output_type("{  }").value() == SmartctlParserType::Json);

	REQUIRE(SmartctlParser::detect_output_type(" \n {  } ").value() == SmartctlParserType::Json);

	REQUIRE(SmartctlParser::detect_output_type("smartctl").value() == SmartctlParserType::Text);

	REQUIRE(SmartctlParser::detect_output_type(
R"(smartctl 7.2 2020-12-30 r5155 [x86_64-linux-5.3.18-lp152.66-default] (SUSE RPM)
Copyright (C) 2002-20, Bruce Allen, Christian Franke, www.smartmontools.org

)").value() == SmartctlParserType::Text);

}



/// @}




