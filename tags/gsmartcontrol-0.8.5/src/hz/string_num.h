/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_STRING_NUM_H
#define HZ_STRING_NUM_H

#include "hz_config.h"  // feature macros

#include <string>
#include <sstream>
#include <iomanip>  // setbase, setprecision, setw
#include <ios>  // std::fixed, std::internal
#include <locale>  // std::locale::classic()
#include <limits>  // std::numeric_limits
#include <cerrno>  // errno (not std::errno, it may be a macro)
#include <cstring>  // std::strncmp

#include "type_properties.h"  // type_is_*
#include "type_categories.h"  // type_check_category
#include "static_assert.h"  // HZ_STATIC_ASSERT
#include "ascii.h"  // ascii_*



// String to number and number to string conversions



namespace hz {


	// Check whether a string represents a numeric value (the value must
	// be represented in C locale).

	// strict == true indicates that the whole string must represent a number exactly.
	// strict == false allows the string to contain the number only in its beginning
	// (ignores any leading spaces and trailing garbage).
	// base has an effect only for integral types. For bool it specifies whether to
	// accept "true" and "false" (as opposed to 1 and 0). For others, base should
	// be between 2 and 36 inclusive.

	// This function has no definition, only specializations.
	template<typename T> inline
	bool string_is_numeric(const std::string& s, T& number, bool strict, int base_or_boolalpha);

	// Short version with default base. (Needed because default base is different for bool and int).
	template<typename T> inline
	bool string_is_numeric(const std::string& s, T& number, bool strict = true);


	// Convert numeric value to string. alpha_or_base_or_precision means:
	// for bool, 0 means 1/0, 1 means true/false;
	// for int family (including char), it's the base to format in (8, 10, 16 are definitely supported);
	// for float family, it controls the number of digits after comma.
	template<typename T> inline
	std::string number_to_string(T number, int alpha_or_base_or_precision);

	// Short version with default base / precision.
	template<typename T> inline
	std::string number_to_string(T number);


}




// ------------------------------------------- Implementation



namespace hz {



// Definition of the one without base parameter.
template<typename T> inline
bool string_is_numeric(const std::string& s, T& number, bool strict)
{
	int base = 0;
	if (type_is_same<bool, T>::value) {
		base = 1;  // use alpha (true / false), as opposed to 1 / 0.

	} else if (type_is_integral<T>::value) {
		base = 0;  // auto-detect base 10, 16 (0xNUM), 8 (0NUM).
	}
	// base is ignored for other types

	return string_is_numeric<T>(s, number, strict, base);
}



// Note: Parameter "base" is ignored for floating point types.
#define DEFINE_STRING_IS_NUMERIC(type) \
template<> inline \
bool string_is_numeric<type>(const std::string& s, type& number, bool strict, int base) \
{ \
	if (s.empty() || (strict && hz::ascii_isspace(s[0])))  /* ascii_strtoi() skips the leading spaces */ \
		return false; \
	const char* str = s.c_str(); \
	char* end = 0; \
	errno = 0; \
	type tmp = hz::ascii_strton<type>(str, &end, base); \
	if ( (strict ? (*end == '\0') : (end != str)) && errno == 0 ) {  /* if end is 0 byte, then the whole string was parsed */ \
		number = tmp; \
		return true; \
	} \
	return false; \
}




// Define string_is_numeric<T>() function specializations for various types:

// Specialization for bool
template<> inline
bool string_is_numeric<bool>(const std::string& s, bool& number, bool strict, int boolalpha_enabled)
{
	if (s.empty() || (strict && hz::ascii_isspace(s[0])))  // ascii_strtoi() skips the leading spaces
		return false;
	const char* str = s.c_str();

	if (boolalpha_enabled) {
		// skip spaces. won't do anything in strict mode (we already ruled out spaces there)
		while (hz::ascii_isspace(*str)) {
			++str;
		}

		// contains "true" at start, or equals to "true" if strict.
		if (std::strncmp(str, "true", 4) == 0 && (!strict || str[4] == '\0')) {  // str is 0-terminated, so no violation here
			number = true;
			return true;
		}
		// same for "false"
		if (std::strncmp(str, "false", 5) == 0 && (!strict || str[5] == '\0')) {
			number = false;
			return true;
		}
		return false;
	}

	char* end = 0;
	errno = 0;
	// we use ascii_strtoi here to support of +001, etc...
	bool tmp = hz::ascii_strtoi<bool>(str, &end, 0);  // auto-base
	if ( (strict ? (*end == '\0') : (end != str)) && errno == 0 ) {  // if end is 0 byte, then the whole string was parsed
		number = tmp;
		return true;
	}
	return false;
}




DEFINE_STRING_IS_NUMERIC(char)
DEFINE_STRING_IS_NUMERIC(signed char)
DEFINE_STRING_IS_NUMERIC(unsigned char)
DEFINE_STRING_IS_NUMERIC(wchar_t)

DEFINE_STRING_IS_NUMERIC(short int)
DEFINE_STRING_IS_NUMERIC(unsigned short int)

DEFINE_STRING_IS_NUMERIC(int)
DEFINE_STRING_IS_NUMERIC(unsigned int)

DEFINE_STRING_IS_NUMERIC(long int)
DEFINE_STRING_IS_NUMERIC(unsigned long int)

#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	DEFINE_STRING_IS_NUMERIC(long long int)
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	DEFINE_STRING_IS_NUMERIC(unsigned long long int)
#endif

DEFINE_STRING_IS_NUMERIC(float)
DEFINE_STRING_IS_NUMERIC(double)
DEFINE_STRING_IS_NUMERIC(long double)



#undef DEFINE_STRING_IS_NUMERIC





// ------------------------------- number_to_string



namespace internal {


	template<typename T, typename SpecCat = typename type_check_category<T>::type>
	struct number_to_string_impl {
// 		static std::string func(T number, int alpha_or_base_or_precision, bool ignored_param)
// 		{
// 			HZ_STATIC_ASSERT(hz::static_false<T>::value, not_a_number);
// 			return std::string();
// 		}
	};


	// bool spec
	template<typename T>
	struct number_to_string_impl<T, type_cat_bool> {
		static std::string func(T number, int boolalpha_enabled, bool ignored_param)
		{
			if (boolalpha_enabled)
				return (number ? "true" : "false");
			return (number ? "1" : "0");
		}
	};


	// int spec
	template<typename T>
	struct number_to_string_impl<T, type_cat_int> {
		static std::string func(T number, int base, bool ignored_param)
		{
			if (number == 0) {
				if (base == 16) {
					return "0x" + std::string(sizeof(T) * 2, '0');  // 0 doesn't print as 0x0000, but as 000000. fix that.

				} else if (base == 8) {  // same here, 0 prints as 0.
					return "00";  // better than simply 0 (at least it's clearly octal).
				}
				// base 10 can possibly have some funny formatting, so continue...
			}

			std::ostringstream ss;
			ss.imbue(std::locale::classic());  // make it use classic locale

			if (base == 16) {
				// setfill & internal: leading 0s between 0x and XXXX.
				// setw: e.g. for int32, we need 4*2 (size * 2 chars for byte) + 2 (0x) width.
				ss << std::setfill('0') << std::internal << std::setw(static_cast<int>((sizeof(T) * 2) + 2));
			}

			ss << std::showbase << std::setbase(base) << number;

			return ss.str();
		}
	};


	// char spec
	template<typename T>
	struct number_to_string_impl<T, type_cat_char> {
		static std::string func(T number, int base, bool ignored_param)
		{
			return number_to_string(static_cast<long int>(number), base);  // long int should be > (u)char
		}
	};


	// floats spec
	template<typename T>
	struct number_to_string_impl<T, type_cat_float> {
		static std::string func(T number, int precision, bool fixed_prec)
		{
			std::ostringstream ss;
			ss.imbue(std::locale::classic());  // make it use classic locale
			// without std::fixed, precision is counted as all digits, as opposed to only after comma.
			if (fixed_prec)
				ss << std::fixed;
			ss << std::setprecision(precision) << number;
			return ss.str();
		}
	};


}  // ns internal




// public function with 2 parameters
template<typename T> inline
std::string number_to_string(T number, int boolalpha_or_base_or_precision)
{
	return internal::number_to_string_impl<T>::func(number, boolalpha_or_base_or_precision, true);
}



// public function - short version with default base / precision
template<typename T> inline
std::string number_to_string(T number)
{
	int base = 0;
	if (type_is_same<bool, T>::value) {
		base = 1;  // alpha (true / false), as opposed to 1 / 0.

	} else if (type_is_integral<T>::value) {
		base = 10;  // default base - 10

	} else if (type_is_floating_point<T>::value) {
		base = std::numeric_limits<T>::digits10 + 1;  // precision. 1 is for sign
	}

	// don't use fixed prec here, digits10 is for the whole number
	return internal::number_to_string_impl<T>::func(number, base, false);
}






}  // ns




#endif
