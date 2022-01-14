/******************************************************************************
License: Zlib
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_STRING_NUM_H
#define HZ_STRING_NUM_H

#include <string>
#include <sstream>
#include <iomanip>  // setbase, setprecision, setw
#include <ios>  // std::fixed, std::internal
#include <locale>  // std::locale
#include <limits>  // std::numeric_limits
#include <cstring>  // std::strncmp
#include <exception>
#include <type_traits>

#include "locale_tools.h"


/**
\file
String to number and number to string conversions, with or without locale.
*/



namespace hz {


	/// Check whether a string represents a numeric value.
	/// strict == true indicates that the whole string must represent a number exactly.
	/// strict == false allows the string to contain the number only in its beginning
	/// (ignores any leading spaces and trailing garbage).
	/// base has an effect only for integral types. For bool it specifies whether to
	/// accept "true" and "false" (as opposed to 1 and 0). For others, base should
	/// be between 2 and 36 inclusive.
	/// This function has no definition, only specializations.
	/// A version that operates in global locale.
	template<typename T>
	bool string_is_numeric_locale(const std::string& s, T& number, bool strict, int base_or_boolalpha);

	/// A version that operates in global locale.
	template<typename T>
	bool string_is_numeric_locale(const std::string& s, T& number, bool strict = true);


	/// A version that operates in classic locale.
	template<typename T>
	bool string_is_numeric_nolocale(const std::string& s, T& number, bool strict, int base_or_boolalpha);

	/// A version that operates in classic locale.
	template<typename T>
	bool string_is_numeric_nolocale(const std::string& s, T& number, bool strict = true);


	/// A convenience string_is_numeric wrapper.
	/// Note that in strict mode, T() is returned for invalid values.
	/// A version that operates in global locale.
	template<typename T>
	T string_to_number_locale(const std::string& s, bool strict, int base_or_boolalpha);

	/// Short version with default base. (Needed because default base is different for bool and int).
	/// A version that operates in global locale.
	template<typename T>
	T string_to_number_locale(const std::string& s, bool strict = true);


	/// A version that operates in classic locale.
	template<typename T>
	T string_to_number_nolocale(const std::string& s, bool strict, int base_or_boolalpha);

	/// A version that operates in classic locale.
	template<typename T>
	T string_to_number_nolocale(const std::string& s, bool strict = true);



	/// Convert numeric value to string. alpha_or_base_or_precision means:
	/// for bool, 0 means 1/0, 1 means true/false;
	/// for int family (including char), it's the base to format in (8, 10, 16 are definitely supported);
	/// for float family, it controls the number of digits after comma.
	/// A version that operates in global locale.
	template<typename T>
	std::string number_to_string_locale(T number, int alpha_or_base_or_precision, bool fixed_prec = false);

	/// Short version with default base / precision.
	/// A version that operates in global locale.
	template<typename T>
	std::string number_to_string_locale(T number);


	/// A version that operates in classic locale.
	template<typename T>
	std::string number_to_string_nolocale(T number, int alpha_or_base_or_precision, bool fixed_prec = false);

	/// A version that operates in classic locale.
	template<typename T>
	std::string number_to_string_nolocale(T number);


}




// ------------------------------------------- Implementation



namespace hz {



namespace internal {


	// Version for integral / floating point types
	template<typename T>
	bool string_is_numeric_impl_global_locale(const std::string& s, T& number, bool strict, [[maybe_unused]] int base, const std::locale& loc = std::locale())
	{
		static_assert(std::is_arithmetic_v<T>, "Type T not convertible to a number");

		if (s.empty() || (strict && std::isspace(s[0], loc)))  // sto* functions skip leading space
			return false;

		std::size_t num_read = 0;
		T value = T();

		try {
			if constexpr(std::is_same_v<T, char> || std::is_same_v<T, unsigned char> || std::is_same_v<T, signed char>
					|| std::is_same_v<T, wchar_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>
					|| std::is_same_v<T, short> || std::is_same_v<T, int>) {
				int tmp = std::stoi(s, &num_read, base);
				if (tmp != static_cast<T>(tmp)) {
					return false;  // out of range
				}
				value = static_cast<T>(tmp);
			} else if constexpr(std::is_same_v<T, long>) {
				value = std::stol(s, &num_read, base);
			} else if constexpr(std::is_same_v<T, long long>) {
				value = std::stoll(s, &num_read, base);
			} else if constexpr(std::is_same_v<T, unsigned short> || std::is_same_v<T, unsigned int> ||
					std::is_same_v<T, unsigned long>) {
				unsigned long tmp = std::stoul(s, &num_read, base);
				if (tmp != static_cast<T>(tmp)) {
					return false;  // out of range
				}
				value = static_cast<T>(tmp);
			} else if constexpr(std::is_same_v<T, unsigned long long>) {
				value = std::stoull(s, &num_read, base);
			} else if constexpr(std::is_same_v<T, float>) {
				value = std::stof(s, &num_read);
			} else if constexpr(std::is_same_v<T, double>) {
				value = std::stod(s, &num_read);
			} else if constexpr(std::is_same_v<T, long double>) {
				value = std::stold(s, &num_read);
			}
		}
		catch (std::exception&) {  // std::invalid_argument, std::out_of_range
			return false;
		}

		// If strict, check that everything was parsed. If not strict, check that
		// at least one character was parsed (not sure if this includes whitespace).
		if (strict ? (num_read == s.size()) : (num_read != 0)) {
			number = value;
			return true;
		}

		return false;
	}



	// Version for integral / floating point types
	template<typename T>
	bool string_is_numeric_impl_classic_locale(const std::string& s, T& number, bool strict,  [[maybe_unused]] int base)
	{
		// TODO port to std::from_chars().
		ScopedCLocale loc;
		return string_is_numeric_impl_global_locale(s, number, strict, base, std::locale::classic());
	}



	// A version for bool
	inline bool string_is_numeric_impl_bool(const std::string& s, bool& number, bool strict, int boolalpha_enabled, bool use_classic_locale)
	{
		std::locale loc;
		if (use_classic_locale) {
			loc = std::locale::classic();
		}
		if (s.empty() || (strict && std::isspace(s[0], loc)))  // ascii_strtoi() skips the leading spaces
			return false;
		const char* str = s.c_str();

		if (boolalpha_enabled != 0) {
			// skip spaces. won't do anything in strict mode (we already ruled out spaces there)
			while (std::isspace(*str, loc)) {
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

		int value = 0;
		bool status = false;
		if (use_classic_locale) {
			status = string_is_numeric_impl_classic_locale(s, value, strict, 10);
		} else {
			status = string_is_numeric_impl_global_locale(s, value, strict, 10);
		}
		if (!status) {
			return false;
		}

		if (strict && value != 0 && value != 1) {
			return false;
		}

		number = static_cast<bool>(value);
		return true;
	}


}



template<typename T>
bool string_is_numeric_locale(const std::string& s, T& number, bool strict, int base_or_boolalpha)
{
	if constexpr(std::is_same_v<T, bool>) {
		return internal::string_is_numeric_impl_bool(s, number, strict, base_or_boolalpha, false);
	} else {
		return internal::string_is_numeric_impl_global_locale(s, number, strict, base_or_boolalpha);
	}
}



template<typename T>
bool string_is_numeric_locale(const std::string& s, T& number, bool strict)
{
	if constexpr(std::is_same_v<T, bool>) {
		return internal::string_is_numeric_impl_bool(s, number, strict, 1, false);  // use boolalpha
	} else {
		return internal::string_is_numeric_impl_global_locale(s, number, strict, 0);  // auto-detect base
	}
}



template<typename T>
bool string_is_numeric_nolocale(const std::string& s, T& number, bool strict, int base_or_boolalpha)
{
	if constexpr(std::is_same_v<T, bool>) {
		return internal::string_is_numeric_impl_bool(s, number, strict, base_or_boolalpha, true);
	} else {
		return internal::string_is_numeric_impl_classic_locale(s, number, strict, base_or_boolalpha);
	}
}


template<typename T>
bool string_is_numeric_nolocale(const std::string& s, T& number, bool strict)
{
	if constexpr(std::is_same_v<T, bool>) {
		return internal::string_is_numeric_impl_bool(s, number, strict, 1, true);  // use boolalpha
	} else {
		return internal::string_is_numeric_impl_classic_locale(s, number, strict, 0);  // auto-detect base
	}
}




// ------------------------------- string_to_number


template<typename T>
T string_to_number_locale(const std::string& s, bool strict, int base_or_boolalpha)
{
	T value = T();
	string_is_numeric_locale(s, value, strict, base_or_boolalpha);
	return value;
}



template<typename T>
T string_to_number_locale(const std::string& s, bool strict)
{
	T value = T();
	string_is_numeric_locale(s, value, strict);
	return value;
}




template<typename T>
T string_to_number_nolocale(const std::string& s, bool strict, int base_or_boolalpha)
{
	T value = T();
	string_is_numeric_nolocale(s, value, strict, base_or_boolalpha);
	return value;
}



template<typename T>
T string_to_number_nolocale(const std::string& s, bool strict)
{
	T value = T();
	string_is_numeric_nolocale(s, value, strict);
	return value;
}




// ------------------------------- number_to_string



namespace internal {

	inline std::string number_to_string_impl_bool(bool number, int boolalpha_enabled)
	{
		if (boolalpha_enabled != 0)
			return (number ? "true" : "false");
		return (number ? "1" : "0");
	}


	template<typename T>
	std::string number_to_string_impl_integral(T number, int base, bool use_classic_locale)
	{
		if (number == 0) {
			if (base == 16) {
				return "0x" + std::string(sizeof(T) * 2, '0');  // 0 doesn't print as 0x0000, but as 000000. fix that.
			}
			if (base == 8) {  // same here, 0 prints as 0.
				return "00";  // better than simply 0 (at least it's clearly octal).
			}
			// base 10 can possibly have some funny formatting, so continue...
		}

		std::ostringstream ss;
		if (use_classic_locale) {
			ss.imbue(std::locale::classic());
		}

		if (base == 16) {
			// setfill & internal: leading 0s between 0x and XXXX.
			// setw: e.g. for int32, we need 4*2 (size * 2 chars for byte) + 2 (0x) width.
			ss << std::setfill('0') << std::internal << std::setw(static_cast<int>((sizeof(T) * 2) + 2));
		}

		ss << std::showbase << std::setbase(base);

		if constexpr(std::is_same_v<char, T> || std::is_same_v<signed char, T> || std::is_same_v<unsigned char, T>
				|| std::is_same_v<char16_t, T> || std::is_same_v<char32_t, T> || std::is_same_v<wchar_t, T>) {
			ss << static_cast<int>(number);  // avoid printing them as characters
		} else {
			ss << number;
		}
		return ss.str();
	}


	template<typename T>
	std::string number_to_string_impl_floating(T number, int precision, bool fixed_prec, bool use_classic_locale)
	{
		std::ostringstream ss;
		if (use_classic_locale) {
			ss.imbue(std::locale::classic());
		}

		// without std::fixed, precision is counted as all digits, as opposed to only after comma.
		if (fixed_prec)
			ss << std::fixed;

		ss << std::setprecision(precision) << number;
		return ss.str();
	}



	template<typename T>
	std::string number_to_string_impl(T number, int boolalpha_or_base_or_precision,
			[[maybe_unused]] bool fixed_prec,  [[maybe_unused]] bool use_classic_locale)
	{
		static_assert(std::is_arithmetic_v<T>, "Type T not convertible to a number");

		if constexpr(std::is_same_v<bool, T>) {
			return internal::number_to_string_impl_bool(number, boolalpha_or_base_or_precision);
		}
		if constexpr(std::is_integral_v<T>) {
			return internal::number_to_string_impl_integral(number, boolalpha_or_base_or_precision, use_classic_locale);
		}
		if constexpr(std::is_floating_point_v<T>) {
			return internal::number_to_string_impl_floating(number, boolalpha_or_base_or_precision, fixed_prec, use_classic_locale);
		}
		// unreachable
		return {};
	}


}  // ns internal




template<typename T>
std::string number_to_string_locale(T number, int boolalpha_or_base_or_precision, bool fixed_prec)
{
	return internal::number_to_string_impl(number, boolalpha_or_base_or_precision, fixed_prec, false);
}


template<typename T>
std::string number_to_string_locale(T number)
{
	int base = 0;
	if constexpr(std::is_same_v<bool, T>) {
		base = 1;  // alpha (true / false), as opposed to 1 / 0.
	} else if (std::is_integral_v<T>) {
		base = 10;  // default base - 10
	} else if (std::is_floating_point_v<T>) {
		base = std::numeric_limits<T>::digits10 + 1;  // precision. 1 is for sign
	}
	// don't use fixed prec here, digits10 is for the whole number
	return number_to_string_locale(number, base, false);
}


template<typename T>
std::string number_to_string_nolocale(T number, int boolalpha_or_base_or_precision, bool fixed_prec)
{
	return internal::number_to_string_impl(number, boolalpha_or_base_or_precision, fixed_prec, true);
}


template<typename T>
std::string number_to_string_nolocale(T number)
{
	int base = 0;
	if constexpr(std::is_same_v<bool, T>) {
		base = 1;  // alpha (true / false), as opposed to 1 / 0.
	} else if constexpr(std::is_integral_v<T>) {
		base = 10;  // default base - 10
	} else if constexpr(std::is_floating_point_v<T>) {
		base = std::numeric_limits<T>::digits10 + 1;  // precision. 1 is for sign
	}
	// don't use fixed prec here, digits10 is for the whole number
	return number_to_string_nolocale(number, base, false);
}





}  // ns




#endif

/// @}
