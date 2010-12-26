/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_ANY_CONVERT_H
#define HZ_ANY_CONVERT_H

#include "hz_config.h"  // feature macros

#include <string>

#include "string_num.h"  // string_is_numeric(), number_to_string()
#include "type_categories.h"  // type_check_category

// Loose conversions:
// Try to convert any type to any other type as long as specialization
// for any_convert is defined.
// String conversions to numbers are _not_ strictly checked.
// Numeric types are converted by static_cast between each other.




namespace hz {





template<bool v = false>
struct AnyConvertibleValue {
	static const bool value = v;
};


/*
// The actual implementation. This class may be specialized to provide needed conversions.
template<typename From, typename To,
		typename SpecCatFrom = typename type_check_category<From>::type,
		typename SpecCatTo = typename type_check_category<To>::type>
struct any_convert_impl : public AnyConvertibleValue<false> {
	static bool func(From from, From& to)
	{
		return false;
	}
};




// same-spec spec
template<typename From, typename To, typename Spec>
struct any_convert_impl<From, To, Spec, Spec> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};






// from bool to char
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_bool, type_cat_char> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from bool to int
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_bool, type_cat_int> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from bool to float
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_bool, type_cat_float> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};




// from char to bool
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_char, type_cat_bool> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from char to int
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_char, type_cat_int> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from char to float
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_char, type_cat_float> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};




// from int to bool
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_int, type_cat_bool> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from int to char
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_int, type_cat_char> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from int to float
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_int, type_cat_float> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};




// from float to bool
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_float, type_cat_bool> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(static_cast<int>(from));  // avoid "unsafe use of !=" warning
		return true;
	}
};

// from float to char
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_float, type_cat_char> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};

// from float to int
template<typename From, typename To>
struct any_convert_impl<From, To, type_cat_float, type_cat_int> : public AnyConvertibleValue<true> {
	static bool func(From from, To& to)
	{
		to = static_cast<To>(from);
		return true;
	}
};





// from bool to string
template<typename From>
struct any_convert_impl<From, std::string, type_cat_bool, type_cat_unknown> : public AnyConvertibleValue<true> {
	static bool func(From from, std::string& to)
	{
		to = hz::number_to_string(from);
		return true;
	}
};

// from char to string
template<typename From>
struct any_convert_impl<From, std::string, type_cat_char, type_cat_unknown> : public AnyConvertibleValue<true> {
	static bool func(From from, std::string& to)
	{
		to = hz::number_to_string(from);
		return true;
	}
};

// from int to string
template<typename From>
struct any_convert_impl<From, std::string, type_cat_int, type_cat_unknown> : public AnyConvertibleValue<true> {
	static bool func(From from, std::string& to)
	{
		to = hz::number_to_string(from);
		return true;
	}
};

// from float to string
template<typename From>
struct any_convert_impl<From, std::string, type_cat_float, type_cat_unknown> : public AnyConvertibleValue<true> {
	static bool func(From from, std::string& to)
	{
		to = hz::number_to_string(from);
		return true;
	}
};





// from string to bool
template<typename To>
struct any_convert_impl<std::string, To, type_cat_unknown, type_cat_bool> : public AnyConvertibleValue<true> {
	static bool func(const std::string& from, To& to)
	{
		return hz::string_is_numeric(from, to, false);
	}
};

// from string to char
template<typename To>
struct any_convert_impl<std::string, To, type_cat_unknown, type_cat_char> : public AnyConvertibleValue<true> {
	static bool func(const std::string& from, To& to)
	{
		return hz::string_is_numeric(from, to, false);
	}
};

// from string to int
template<typename To>
struct any_convert_impl<std::string, To, type_cat_unknown, type_cat_int> : public AnyConvertibleValue<true> {
	static bool func(const std::string& from, To& to)
	{
		return hz::string_is_numeric(from, to, false);
	}
};

// from string to float
template<typename To>
struct any_convert_impl<std::string, To, type_cat_unknown, type_cat_float> : public AnyConvertibleValue<true> {
	static bool func(const std::string& from, To& to)
	{
		return hz::string_is_numeric(from, to, false);
	}
};

*/



template<typename From, typename To>
inline bool any_convert(From from, To& to)
{
	return false;
}


// same-type spec
template<typename From>
inline bool any_convert(From from, From& to)
{
	to = from;
	return true;
}



// master struct, default to non-convertible
template<typename From, typename To>
struct any_convertible : public AnyConvertibleValue<false> { };


// same-type spec
template<typename From>
struct any_convertible<From, From> : public AnyConvertibleValue<true> { };


// specific type specs - see below.


/*
template<typename From, typename To>
inline bool any_convert(From from, To& to)
{
	return any_convert_impl<From, To>::func(from, to);
}
*/



/*
// you may use this to check if it's convertible or not
template<typename From, typename To>
struct is_any_convertible : public AnyConvertibleValue<
	any_convert_impl<From, To>::value || type_is_same<From, To>::value> { };
*/






#define DEFINE_ANY_CONVERT_SPEC(from_type, to_type) \
	template<> \
	struct any_convertible<from_type, to_type> : public AnyConvertibleValue<true> { }; \
	\
	template<> \
	inline bool any_convert<from_type, to_type>(from_type from, to_type& to) \



#define DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, to_type) \
	DEFINE_ANY_CONVERT_SPEC(from_type, to_type) \
	{ \
		to = static_cast<to_type>(from); \
		return true; \
	}

#define DEFINE_ANY_CONVERT_SPEC_STATIC_TOBOOL(from_type) \
	DEFINE_ANY_CONVERT_SPEC(from_type, bool) \
	{ \
		to = static_cast<bool>(static_cast<int>(from)); /* this disables gcc warning about unsafe != */ \
		return true; \
	}

#define DEFINE_ANY_CONVERT_SPEC_NUMTOSTRING(from_type) \
	DEFINE_ANY_CONVERT_SPEC(from_type, std::string) \
	{ \
		to = hz::number_to_string(from); \
		return true; \
	}

#define DEFINE_ANY_CONVERT_SPEC_STRINGTONUM(to_type) \
	DEFINE_ANY_CONVERT_SPEC(std::string, to_type) \
	{ \
		return hz::string_is_numeric(from, to, false); \
	} \
	DEFINE_ANY_CONVERT_SPEC(const std::string&, to_type) \
	{ \
		return hz::string_is_numeric(from, to, false); \
	}


#ifndef DISABLE_LL_INT
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_LLI(from_type) \
			DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, long long int)
#else
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_LLI(from_type)
#endif

#ifndef DISABLE_ULL_INT
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_ULLI(from_type) \
			DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, unsigned long long int)
#else
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_ULLI(from_type)
#endif


#define DEFINE_ANY_CONVERT_SPEC_ALL(from_type) \
	DEFINE_ANY_CONVERT_SPEC_STATIC_TOBOOL(from_type) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, char) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, signed char) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, unsigned char) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, wchar_t) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, short int) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, unsigned short int) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, int) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, unsigned int) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, long int) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, unsigned long int) \
	DEFINE_ANY_CONVERT_SPEC_STATIC_LLI(from_type) \
	DEFINE_ANY_CONVERT_SPEC_STATIC_ULLI(from_type) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, double) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, float) \
	DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, long double) \
	DEFINE_ANY_CONVERT_SPEC_NUMTOSTRING(from_type) \
	DEFINE_ANY_CONVERT_SPEC_STRINGTONUM(from_type)



DEFINE_ANY_CONVERT_SPEC_ALL(bool)

DEFINE_ANY_CONVERT_SPEC_ALL(char)
DEFINE_ANY_CONVERT_SPEC_ALL(signed char)
DEFINE_ANY_CONVERT_SPEC_ALL(unsigned char)
DEFINE_ANY_CONVERT_SPEC_ALL(wchar_t)

DEFINE_ANY_CONVERT_SPEC_ALL(short int)
DEFINE_ANY_CONVERT_SPEC_ALL(unsigned short int)
DEFINE_ANY_CONVERT_SPEC_ALL(int)
DEFINE_ANY_CONVERT_SPEC_ALL(unsigned int)
DEFINE_ANY_CONVERT_SPEC_ALL(long int)
DEFINE_ANY_CONVERT_SPEC_ALL(unsigned long int)
#ifndef DISABLE_LL_INT
	DEFINE_ANY_CONVERT_SPEC_ALL(long long int)
#endif
#ifndef DISABLE_ULL_INT
	DEFINE_ANY_CONVERT_SPEC_ALL(unsigned long long int)
#endif

DEFINE_ANY_CONVERT_SPEC_ALL(double)
DEFINE_ANY_CONVERT_SPEC_ALL(float)
DEFINE_ANY_CONVERT_SPEC_ALL(long double)



#undef DEFINE_ANY_CONVERT_SPEC
#undef DEFINE_ANY_CONVERT_SPEC_STATIC
#undef DEFINE_ANY_CONVERT_SPEC_NUMTOSTRING
#undef DEFINE_ANY_CONVERT_SPEC_STRINGTONUM
#undef DEFINE_ANY_CONVERT_SPEC_ALL




}  // ns



#endif
