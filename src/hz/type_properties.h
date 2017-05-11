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

#ifndef HZ_TYPE_PROPERTIES_H
#define HZ_TYPE_PROPERTIES_H

#include "hz_config.h"  // feature macros

#include <cstddef>  // std::size_t


/**
\file
Type transformations, properties, etc...
This file provides some type facilities in absence of C++11's type_traits header.
*/



namespace hz {


// ---------------------- Type Examination



#define TYPE_DEFINE_SPEC_0(Trait, Type, Value) \
	template<> struct Trait<Type> : public type_integral_constant<bool, Value> { }; \
	template<> struct Trait<Type const> : public type_integral_constant<bool, Value> { }; \
	template<> struct Trait<Type volatile> : public type_integral_constant<bool, Value> { }; \
	template<> struct Trait<Type const volatile> : public type_integral_constant<bool, Value> { }

#define TYPE_DEFINE_SPEC_1(Trait, Type, Value) \
	template<typename T> struct Trait<Type> : public type_integral_constant<bool, Value> { }; \
	template<typename T> struct Trait<Type const> : public type_integral_constant<bool, Value> { }; \
	template<typename T> struct Trait<Type volatile> : public type_integral_constant<bool, Value> { }; \
	template<typename T> struct Trait<Type const volatile> : public type_integral_constant<bool, Value> { }



template<typename T, T v>
struct type_integral_constant {
	static const T value = v;
	typedef T value_type;
	typedef type_integral_constant<T, v> type;
};

template<typename T, T v>
const T type_integral_constant<T, v>::value;


typedef type_integral_constant<bool, true> type_true_type;
typedef type_integral_constant<bool, false> type_false_type;



// needed for some other checks
template<typename T>
struct type_is_void : public type_false_type { };

TYPE_DEFINE_SPEC_0(type_is_void, void, true);



template<typename T>
struct type_is_integral : public type_false_type { };

TYPE_DEFINE_SPEC_0(type_is_integral, bool, true);
TYPE_DEFINE_SPEC_0(type_is_integral, char, true);
TYPE_DEFINE_SPEC_0(type_is_integral, signed char, true);
TYPE_DEFINE_SPEC_0(type_is_integral, unsigned char, true);
TYPE_DEFINE_SPEC_0(type_is_integral, wchar_t, true);

TYPE_DEFINE_SPEC_0(type_is_integral, short, true);
TYPE_DEFINE_SPEC_0(type_is_integral, unsigned short, true);
TYPE_DEFINE_SPEC_0(type_is_integral, int, true);
TYPE_DEFINE_SPEC_0(type_is_integral, unsigned int, true);
TYPE_DEFINE_SPEC_0(type_is_integral, long, true);
TYPE_DEFINE_SPEC_0(type_is_integral, unsigned long, true);
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	TYPE_DEFINE_SPEC_0(type_is_integral, long long, true);
#endif
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	TYPE_DEFINE_SPEC_0(type_is_integral, unsigned long long, true);
#endif


template<typename T>
struct type_is_signed : public type_false_type { };

TYPE_DEFINE_SPEC_0(type_is_signed, signed char, true);
TYPE_DEFINE_SPEC_0(type_is_signed, short, true);
TYPE_DEFINE_SPEC_0(type_is_signed, int, true);
TYPE_DEFINE_SPEC_0(type_is_signed, long, true);
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	TYPE_DEFINE_SPEC_0(type_is_signed, long long, true);
#endif


template<typename T>
struct type_is_unsigned : public type_false_type { };

TYPE_DEFINE_SPEC_0(type_is_unsigned, unsigned char, true);
TYPE_DEFINE_SPEC_0(type_is_unsigned, unsigned short, true);
TYPE_DEFINE_SPEC_0(type_is_unsigned, unsigned int, true);
TYPE_DEFINE_SPEC_0(type_is_unsigned, unsigned long, true);
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	TYPE_DEFINE_SPEC_0(type_is_unsigned, unsigned long long, true);
#endif


template<typename T>
struct type_is_floating_point : public type_false_type { };

TYPE_DEFINE_SPEC_0(type_is_floating_point, float, true);
TYPE_DEFINE_SPEC_0(type_is_floating_point, double, true);
TYPE_DEFINE_SPEC_0(type_is_floating_point, long double, true);




template<typename T>
struct type_is_arithmetic
	: public type_integral_constant<bool, (type_is_integral<T>::value || type_is_floating_point<T>::value)> { };




template<typename T>
struct type_is_pointer : public type_false_type { };

TYPE_DEFINE_SPEC_1(type_is_pointer, T*, true);



template<typename T>
struct type_is_reference : public type_false_type { };

template<typename T>
struct type_is_reference<T&> : public type_true_type { };



template<typename T>
struct type_is_array : public type_false_type { };

template<typename T, std::size_t size>
struct type_is_array<T[size]> : public type_true_type { };

template<typename T>
struct type_is_array<T[]> : public type_true_type { };



template<typename T>
struct type_is_const : public type_false_type { };

template<typename T>
struct type_is_const<T const> : public type_true_type { };

template<typename T>
struct type_is_volatile : public type_false_type { };

template<typename T>
struct type_is_volatile<T volatile> : public type_true_type { };




#undef TYPE_DEFINE_SPEC_0
#undef TYPE_DEFINE_SPEC_1



template<typename T1, typename T2>
struct type_is_same : public type_false_type { };

template<typename T>
struct type_is_same<T, T> : public type_true_type { };




namespace internal {

	template<typename T>
	struct type_is_polymorphic_helper {
		private:
			template<typename U> struct first : public U { };

			template<typename U> struct second : public U {
				virtual void internal_dummy();
				virtual ~second();
			};

		public:
			static const bool value = (sizeof(first<T>) == sizeof(second<T>));
	};

}  // ns


template<typename T>
struct type_is_polymorphic
	: public type_integral_constant<bool, internal::type_is_polymorphic_helper<T>::value> { };



template<class From, class To>
struct type_is_convertible {
	protected:  // instead of private, to avoid gcc warnings
		typedef char (&yes) [1];
		typedef char (&no) [2];

		static yes f(To*);
		static no f(...);

	public:
		static const bool value = (sizeof( f((From*)0) ) == sizeof(yes));
};




// ---------------------- Type Reference / Pointer / CV-Qualifier Modification



template<typename T>
struct type_remove_reference { typedef T type; };

template<typename T>
struct type_remove_reference<T&> { typedef T type; };


template<typename T>
struct type_add_reference { typedef typename type_remove_reference<T>::type& type; };

// there's no void&, and this may be needed for function return types (T&)
template<>
struct type_add_reference<void> { typedef void type; };



template<typename T>  // int -> int
struct type_remove_pointer { typedef T type; };

template<typename T>  // int* -> int
struct type_remove_pointer<T*> { typedef T type; };

template<typename T>  // int* const -> int
struct type_remove_pointer<T* const> { typedef T type; };

template<typename T>  // int* volatile -> int
struct type_remove_pointer<T* volatile> { typedef T type; };

template<typename T>  // int* const volatile -> int
struct type_remove_pointer<T* const volatile> { typedef T type; };


template<typename T>
struct type_add_pointer { typedef typename type_remove_reference<T>::type* type; };




template<typename T>
struct type_remove_const { typedef T type; };

template<typename T>
struct type_remove_const<T const> { typedef T type; };

template<typename T>
struct type_remove_volatile { typedef T type; };

template<typename T>
struct type_remove_volatile<T volatile> { typedef T type; };

template<typename T>
struct type_remove_cv { typedef typename type_remove_const<typename type_remove_volatile<T>::type>::type type; };



template<typename T>
struct type_add_const { typedef T const type; };

template<typename T>
struct type_add_volatile { typedef T volatile type; };

template<typename T>
struct type_add_cv { typedef typename type_add_const<typename type_add_volatile<T>::type>::type type; };




// All-in-one transformator

template<typename T>
struct type_transform {

	typedef typename type_remove_const<typename type_remove_reference<T>::type>::type clean_type;

	typedef typename type_add_reference<clean_type>::type reference_type;
	typedef typename type_add_const<clean_type>::type const_type;

	typedef typename type_add_const<reference_type>::type const_reference_type;
};






// ---------------------- Type Sign Modification


template<typename T>
struct type_make_signed { typedef T type; };

template<typename T>
struct type_make_unsigned { typedef T type; };


// list unsigned types
template<> struct type_make_signed<char> { typedef signed char type; };
template<> struct type_make_signed<unsigned char> { typedef signed char type; };

template<> struct type_make_signed<unsigned short int> { typedef short int type; };
template<> struct type_make_signed<unsigned int> { typedef int type; };
template<> struct type_make_signed<unsigned long int> { typedef long int type; };
#if !(defined DISABLE_ULL_INT && DISABLE_ULL_INT)
	template<> struct type_make_signed<unsigned long long int> { typedef long long int type; };
#endif


// list signed types
template<> struct type_make_unsigned<char> { typedef unsigned char type; };
template<> struct type_make_unsigned<signed char> { typedef unsigned char type; };

template<> struct type_make_unsigned<short int> { typedef unsigned short int type; };
template<> struct type_make_unsigned<int> { typedef unsigned int type; };
template<> struct type_make_unsigned<long int> { typedef unsigned long int type; };
#if !(defined DISABLE_LL_INT && DISABLE_LL_INT)
	template<> struct type_make_unsigned<long long int> { typedef unsigned long long int type; };
#endif







}  // ns






#endif

/// @}
