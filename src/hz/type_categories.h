/**************************************************************************
 Copyright:
      (C) 2003 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_TYPE_CATEGORIES_H
#define HZ_TYPE_CATEGORIES_H

#include "hz_config.h"  // feature macros



// Type categories for easy specialization generation


namespace hz {



// split by integral / floating point

struct type_arithm_unknown { };
struct type_arithm_integral { };
struct type_arithm_floating_point { };

template<typename T>
struct type_check_arithmetic {
	typedef type_arithm_unknown type;
};

template<> struct type_check_arithmetic<bool> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<char> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<signed char> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<unsigned char> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<wchar_t> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<short int> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<unsigned short int> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<int> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<unsigned int> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<long int> { typedef type_arithm_integral type; };
template<> struct type_check_arithmetic<unsigned long int> { typedef type_arithm_integral type; };
#ifndef DISABLE_LL_INT
	template<> struct type_check_arithmetic<long long int> { typedef type_arithm_integral type; };
#endif
#ifndef DISABLE_ULL_INT
	template<> struct type_check_arithmetic<unsigned long long int> { typedef type_arithm_integral type; };
#endif

template<> struct type_check_arithmetic<double> { typedef type_arithm_floating_point type; };
template<> struct type_check_arithmetic<float> { typedef type_arithm_floating_point type; };
template<> struct type_check_arithmetic<long double> { typedef type_arithm_floating_point type; };



// split by category - bool, char, int, float

struct type_cat_unknown { };
struct type_cat_bool { };
struct type_cat_char { };
struct type_cat_int { };
struct type_cat_float { };

template<typename T>
struct type_check_category {
	typedef type_cat_unknown type;
};

template<> struct type_check_category<bool> { typedef type_cat_bool type; };

template<> struct type_check_category<char> { typedef type_cat_char type; };
template<> struct type_check_category<signed char> { typedef type_cat_char type; };
template<> struct type_check_category<unsigned char> { typedef type_cat_char type; };
template<> struct type_check_category<wchar_t> { typedef type_cat_char type; };

template<> struct type_check_category<short int> { typedef type_cat_int type; };
template<> struct type_check_category<unsigned short int> { typedef type_cat_int type; };
template<> struct type_check_category<int> { typedef type_cat_int type; };
template<> struct type_check_category<unsigned int> { typedef type_cat_int type; };
template<> struct type_check_category<long int> { typedef type_cat_int type; };
template<> struct type_check_category<unsigned long int> { typedef type_cat_int type; };
#ifndef DISABLE_LL_INT
	template<> struct type_check_category<long long int> { typedef type_cat_int type; };
#endif
#ifndef DISABLE_ULL_INT
	template<> struct type_check_category<unsigned long long int> { typedef type_cat_int type; };
#endif

template<> struct type_check_category<double> { typedef type_cat_float type; };
template<> struct type_check_category<float> { typedef type_cat_float type; };
template<> struct type_check_category<long double> { typedef type_cat_float type; };






}  // ns






#endif
