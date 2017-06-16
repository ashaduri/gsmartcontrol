/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_ANY_CONVERT_H
#define HZ_ANY_CONVERT_H

#include "hz_config.h"  // feature macros

#include <string>

#include "string_num.h"  // string_is_numeric(), number_to_string()
#include "type_categories.h"  // type_check_category


/// \file
/// Loose conversions:
/// Try to convert any type to any other type as long as specialization
/// for any_convert is defined.
/// String conversions to numbers are _not_ strictly checked.
/// Numeric types are converted by static_cast between each other.




namespace hz {




/// This can be used for inheritance to provide a const static \c value member
/// that is either true or false.
template<bool v = false>
struct AnyConvertibleValue {
	static const bool value = v;
};



/// Convert \c from to \c to using loose conversion methods.
template<typename From, typename To>
inline bool any_convert(From from, To& to)
{
	return false;
}


/// Convert \c from to \c to using loose conversion methods.
/// This is a same-type specialization.
template<typename From>
inline bool any_convert(From from, From& to)
{
	to = from;
	return true;
}



/// The structure that says whether type \c From is convertible to type \c To
/// using loose conversion methods.
/// This is the master struct, defaults to non-convertible.
template<typename From, typename To>
struct any_convertible : public AnyConvertibleValue<false> { };


/// The structure that says whether type \c From is convertible to type \c To
/// using loose conversion methods.
// Same-type specialization
template<typename From>
struct any_convertible<From, From> : public AnyConvertibleValue<true> { };


// specific type specs - see below.


/// Define \c any_convertible specialization for \c from_type and \c to_type, saying that
/// they're convertible. Also print \c any_convert function header.
#define DEFINE_ANY_CONVERT_SPEC(from_type, to_type) \
	template<> \
	struct any_convertible<from_type, to_type> : public AnyConvertibleValue<true> { }; \
	\
	template<> \
	inline bool any_convert<from_type, to_type>(from_type from, to_type& to) \



/// Define \c any_convertible and \c any_convert specializations using static_cast.
#define DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, to_type) \
	DEFINE_ANY_CONVERT_SPEC(from_type, to_type) \
	{ \
		to = static_cast<to_type>(from); \
		return true; \
	}

/// Define \c any_convertible and \c any_convert specializations using static_cast using bool.
#define DEFINE_ANY_CONVERT_SPEC_STATIC_TOBOOL(from_type) \
	DEFINE_ANY_CONVERT_SPEC(from_type, bool) \
	{ \
		to = static_cast<bool>(static_cast<int>(from)); /* this disables gcc warning about unsafe != */ \
		return true; \
	}

/// Define \c any_convertible and \c any_convert specializations using hz::number_to_string().
#define DEFINE_ANY_CONVERT_SPEC_NUMTOSTRING(from_type) \
	DEFINE_ANY_CONVERT_SPEC(from_type, std::string) \
	{ \
		to = hz::number_to_string(from); \
		return true; \
	}

/// Define \c any_convertible and \c any_convert specializations using hz::string_is_numeric().
#define DEFINE_ANY_CONVERT_SPEC_STRINGTONUM(to_type) \
	DEFINE_ANY_CONVERT_SPEC(std::string, to_type) \
	{ \
		return hz::string_is_numeric(from, to, false); \
	} \
	DEFINE_ANY_CONVERT_SPEC(const std::string&, to_type) \
	{ \
		return hz::string_is_numeric(from, to, false); \
	}


#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_LLI(from_type) \
			DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, long long int)
#else
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_LLI(from_type)
#endif

#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_ULLI(from_type) \
			DEFINE_ANY_CONVERT_SPEC_STATIC(from_type, unsigned long long int)
#else
	#define DEFINE_ANY_CONVERT_SPEC_STATIC_ULLI(from_type)
#endif


/// Define \c any_convertible and \c any_convert specializations for all built-in types as \c To.
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
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	DEFINE_ANY_CONVERT_SPEC_ALL(long long int)
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
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

/// @}
