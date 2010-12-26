/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "string_num.h"

#include <limits>
#include <iostream>

#include "cstdint.h"




int main()
{
	using namespace hz;


// 	int i = 0;
// 	double d = 0;

// 	std::cerr << (string_is_numeric<int>("-1", i) ? "true" : "false");
// 	std::cerr << ", parsed: i=" << i << "\n";


	{
		int16_t int16_;
		std::cerr << (string_is_numeric("32768", int16_) ? "true" : "false");  // overflow, false
		std::cerr << "\n";

		uint16_t uint16_;
		std::cerr << (string_is_numeric("65535", uint16_) ? "true" : "false");  // true
		std::cerr << "\n";

		int32_t int32_;
		std::cerr << (string_is_numeric(" 1.33", int32_) ? "true" : "false");  // strict, false
		std::cerr << "\n";

		int32_t int32_2;
		std::cerr << (string_is_numeric("1.33", int32_2) ? "true" : "false");  // strict, false
		std::cerr << "\n";

		uint32_t uint32_;
		std::cerr << (string_is_numeric("-1", uint32_) ? "true" : "false");  // underflow, false
		std::cerr << "\n";

		int64_t int64_;
		std::cerr << (string_is_numeric("-1", int64_) ? "true" : "false");  // true
		std::cerr << "\n";

		uint64_t uint64_;
		std::cerr << (string_is_numeric("1", uint64_) ? "true" : "false");  // true
		std::cerr << "\n";

		int int_;
		std::cerr << (string_is_numeric("-1", int_) ? "true" : "false");  // true
		std::cerr << "\n";

		unsigned int uint_;
		std::cerr << (string_is_numeric("-1", uint_) ? "true" : "false");  // underflow, false
		std::cerr << "\n";

		long long_int_;
		std::cerr << (string_is_numeric("-1", long_int_) ? "true" : "false");  // true
		std::cerr << "\n";

		unsigned long long_uint_;
		std::cerr << (string_is_numeric("-1", long_uint_) ? "true" : "false");  // underflow, false
		std::cerr << "\n";

		char char_;
		std::cerr << (string_is_numeric("4", char_) ? "true" : "false");  // true
		std::cerr << "\n";

		unsigned char uchar_;
		std::cerr << (string_is_numeric("315", uchar_) ? "true" : "false");  // overflow, false
		std::cerr << "\n";

		signed char schar_2;
		std::cerr << (string_is_numeric("128", schar_2) ? "true" : "false");  // overflow, false
		std::cerr << "\n";

		signed char schar_3;
		std::cerr << (string_is_numeric("-128", schar_3) ? "true" : "false");  // true
		std::cerr << "\n";

		unsigned char uchar_2;
		std::cerr << (string_is_numeric("128", uchar_2) ? "true" : "false");  // true
		std::cerr << "\n";

		bool b;
		std::cerr << (string_is_numeric("true", b) ? "true" : "false");  // true
		std::cerr << "\n";

	}


// 	std::cerr << (string_is_numeric<int>("-1.3", i) ? "true" : "false");
// 	std::cerr << ", parsed: i=" << i << "\n";


	{
		double d = 0;

		std::cerr << (string_is_numeric("-1.3", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";

		std::cerr << (string_is_numeric(" inf", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";

		std::cerr << (string_is_numeric("infinity", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";

		std::cerr << (string_is_numeric("NAn", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";

		std::cerr << (string_is_numeric("3.e+4", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";

		std::cerr << (string_is_numeric("123 ", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";

		std::cerr << (string_is_numeric("e+3", d) ? "true" : "false");
		std::cerr << ", parsed: d=" << d << "\n";
	}



	{

		// Note: suffixes are case-insensitive.
		// Long int and long double are differentiated by '.' in constant.

		std::cerr << number_to_string(true) << "\n";  // bool
		std::cerr << number_to_string('a') << "\n";  // char
		std::cerr << number_to_string(1.L) << "\n";  // long double
		std::cerr << number_to_string(2L) << "\n";  // long int
		std::cerr << number_to_string(6) << "\n";  // int

#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
		std::cerr << number_to_string(3LL) << "\n";  // long long int
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
		std::cerr << number_to_string(4ULL) << "\n";  // unsigned long long int
#endif

		std::cerr << number_to_string(5.f) << "\n";  // float
		std::cerr << number_to_string(1.33) << "\n";  // double
		std::cerr << number_to_string(2.123456789123456789123456789L) << "\n";

		std::cerr << number_to_string(std::numeric_limits<double>::quiet_NaN()) << "\n";
		std::cerr << number_to_string(std::numeric_limits<float>::signaling_NaN()) << "\n";
		std::cerr << number_to_string(std::numeric_limits<long double>::infinity()) << "\n";

		std::cerr << number_to_string(uint16_t(0x1133), 16) << "\n";
		std::cerr << number_to_string(uint32_t(0x00001234), 16) << "\n";
		std::cerr << number_to_string(uint64_t(0x00001234), 16) << "\n";
		std::cerr << number_to_string(uint16_t(0), 16) << "\n";

		std::cerr << number_to_string(uint16_t(0xffff), 8) << "\n";
		std::cerr << number_to_string(uint16_t(0), 8) << "\n";

// 		std::cerr << number_to_string("abc", 8) << "\n";

// 		std::cerr << number_to_string(uint16_t(0xffff), 2) << "\n";  // won't work
// 		std::cerr << number_to_string(uint16_t(0), 2) << "\n";

	}




	return 0;
}





