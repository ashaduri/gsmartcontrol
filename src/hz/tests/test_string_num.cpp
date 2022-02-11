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
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "hz/string_num.h"

#include <limits>
#include <cstdint>

#include "hz/main_tools.h"



TEST_CASE("NumericStrings", "[hz][parser]")
{
	using namespace hz;

	{
		int i = 10;
		REQUIRE(string_is_numeric_nolocale<int>("-1", i) == true);
		REQUIRE(i == -1);
	}
	{
		int i = 10;
		REQUIRE(string_is_numeric_nolocale<int>("-1.3", i) == false);
		REQUIRE(i == 10);
	}
	{
		int16_t int16_ = 10;
		REQUIRE(string_is_numeric_nolocale("32768", int16_) == false);  // overflow, false
		REQUIRE(int16_ == 10);

		uint16_t uint16_ = 10;
		REQUIRE(string_is_numeric_nolocale("65535", uint16_) == true);
		REQUIRE(uint16_ == 65535);

		int32_t int32_ = 10;
		REQUIRE(string_is_numeric_nolocale(" 1.33", int32_) == false);  // strict, false
		REQUIRE(int32_ == 10);

		int32_t int32_2 = 10;
		REQUIRE(string_is_numeric_nolocale("1.33", int32_2) == false);  // strict, false
		REQUIRE(int32_2 == 10);

		uint32_t uint32_ = 10;
		REQUIRE(string_is_numeric_nolocale("-1", uint32_) == false);  // underflow, false
		REQUIRE(uint32_ == 10);

		int64_t int64_ = 10;
		REQUIRE(string_is_numeric_nolocale("-1", int64_) == true);
		REQUIRE(int64_ == -1);

		uint64_t uint64_ = 10;
		REQUIRE(string_is_numeric_nolocale("1", uint64_) == true);
		REQUIRE(uint64_ == 1);

		uint64_t uint64_2 = 10;
		REQUIRE(string_is_numeric_nolocale("-1", uint64_2) == false);  // underflow
		REQUIRE(uint64_2 == 10);

		uint64_t uint64_3 = 10;
		REQUIRE(string_is_numeric_nolocale(" 1", uint64_3) == false);  // strict
		REQUIRE(uint64_3 == 10);

		uint64_t uint64_4 = 10;
		REQUIRE(string_is_numeric_nolocale(" 1", uint64_4, false) == true);  // ignore space
		REQUIRE(uint64_4 == 1);

		int int_ = 10;
		REQUIRE(string_is_numeric_nolocale("-1", int_) == true);
		REQUIRE(int_ == -1);

		unsigned int uint_ = 10;
		REQUIRE(string_is_numeric_nolocale("-1", uint_) == false);  // underflow, false
		REQUIRE(uint_ == 10);

		long long_int_ = 10;
		REQUIRE(string_is_numeric_nolocale("-1", long_int_) == true);
		REQUIRE(long_int_ == -1);

		unsigned long long_uint_ = 10;
		REQUIRE(string_is_numeric_nolocale("-1", long_uint_) == false);  // underflow, false
		REQUIRE(long_uint_ == 10);

		char char_ = 10;
		REQUIRE(string_is_numeric_nolocale("4", char_) == true);
		REQUIRE(char_ == 4);

		unsigned char uchar_ = 10;
		REQUIRE(string_is_numeric_nolocale("315", uchar_) == false);  // overflow, false
		REQUIRE(uchar_ == 10);

		unsigned char uchar_2 = 10;
		REQUIRE(string_is_numeric_nolocale("128", uchar_2) == true);
		REQUIRE(uchar_2 == 128);

		unsigned char uchar_3 = 10;
		REQUIRE(string_is_numeric_nolocale("-2", uchar_3) == false);  // underflow
		REQUIRE(uchar_3 == 10);

		signed char schar_2 = 10;
		REQUIRE(string_is_numeric_nolocale("128", schar_2) == false);  // overflow, false
		REQUIRE(schar_2 == 10);

		signed char schar_3 = 10;
		REQUIRE(string_is_numeric_nolocale("-128", schar_3) == true);
		REQUIRE(schar_3 == -128);

		bool b = false;
		REQUIRE(string_is_numeric_nolocale("true", b) == true);
		REQUIRE(b == true);
	}
	{
		const double eps = std::numeric_limits<double>::epsilon();  // it will do

		double d = 10;
		REQUIRE(string_is_numeric_nolocale("-1.3", d) == true);
		REQUIRE(std::abs(-1.3 - d) <= eps);

		d = 10;
		REQUIRE(string_is_numeric_nolocale(" inf", d) == false);
		REQUIRE(std::abs(10. - d) <= eps);

		d = 10;
		REQUIRE(string_is_numeric_nolocale(" inf", d, false) == true);  // non-strict
		REQUIRE(std::isinf(d));

		d = 10;
		REQUIRE(string_is_numeric_nolocale("infinity", d) == true);
		REQUIRE(std::isinf(d));

		d = 10;
		REQUIRE(string_is_numeric_nolocale("NAn", d) == true);
		REQUIRE(std::isnan(d));

		d = 10;
		REQUIRE(string_is_numeric_nolocale("3.e+4", d) == true);
		REQUIRE(std::abs(3e4 - d) <= eps);

		d = 10;
		REQUIRE(string_is_numeric_nolocale("-3E4", d) == true);
		REQUIRE(std::abs(-3e4 - d) <= eps);

		d = 10;
		REQUIRE(string_is_numeric_nolocale("123 ", d) == false);  // strict
		REQUIRE(std::abs(10 - d) <= eps);

		d = 10;
		REQUIRE(string_is_numeric_nolocale("123 ", d, false) == true);  // non-strict
		REQUIRE(std::abs(123 - d) <= eps);

		d = 10;
		REQUIRE(string_is_numeric_nolocale("e+3", d) == false);
		REQUIRE(std::abs(10 - d) <= eps);
	}
	{

		// Note: suffixes are case-insensitive.
		// Long int and long double are differentiated by '.' in constant.

		REQUIRE(number_to_string_nolocale(true) == "true");  // bool
		REQUIRE(number_to_string_nolocale('a') == "97");  // char
		REQUIRE(number_to_string_nolocale(1.L) == "1");  // long double
		REQUIRE(number_to_string_nolocale(2L) == "2");  // long int
		REQUIRE(number_to_string_nolocale(6) == "6");  // int

		REQUIRE(number_to_string_nolocale(3LL) == "3");  // long long int
		REQUIRE(number_to_string_nolocale(4ULL) == "4");  // unsigned long long int

		REQUIRE(number_to_string_nolocale(5.F) == "5");  // float
		REQUIRE(number_to_string_nolocale(1.33) == "1.33");  // double
		REQUIRE(number_to_string_nolocale(2.123456789123456789123456789L) == "2.123456789123456789");

		REQUIRE(number_to_string_nolocale(std::numeric_limits<double>::quiet_NaN()) == "nan");
		REQUIRE(number_to_string_nolocale(std::numeric_limits<float>::signaling_NaN()) == "nan");
		REQUIRE(number_to_string_nolocale(std::numeric_limits<long double>::infinity()) == "inf");
		REQUIRE(number_to_string_nolocale(-std::numeric_limits<long double>::infinity()) == "-inf");

		REQUIRE(number_to_string_nolocale(uint16_t(0x1133), 16) == "0x1133");
		REQUIRE(number_to_string_nolocale(uint32_t(0x00001234), 16) == "0x00001234");
		REQUIRE(number_to_string_nolocale(uint64_t(0x00001234), 16) == "0x0000000000001234");
		REQUIRE(number_to_string_nolocale(uint16_t(0), 16) == "0x0000");

		REQUIRE(number_to_string_nolocale(uint16_t(0xffff), 8) == "0177777");
		REQUIRE(number_to_string_nolocale(uint16_t(0), 8) == "00");

		REQUIRE(number_to_string_nolocale(uint16_t(0xffff), 2) == "65535");
		REQUIRE(number_to_string_nolocale(uint16_t(0), 2) == "0");

	}
}






/// @}
