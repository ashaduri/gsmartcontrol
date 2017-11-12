/**************************************************************************
 Copyright:
      (C) 2000 - 2010  Kevlin Henney
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/
/// \file
/// \author Kevlin Henney
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_ANY_TYPE_H
#define HZ_ANY_TYPE_H

#include "hz_config.h"  // feature macros

/**
\file
any_type library (any type one-element container), based on boost::any.

Original notes and copyright info follow:

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// Compilation options:
// - Define DISABLE_RTTI=1 to disable RTTI checks and typeinfo-getter
// functions. NOT recommended.

// - Define DISABLE_ANY_CONVERT=1 to disable all .convert functions
// (avoids dependency on any_convert.h)
*/


#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	#include <typeinfo>  // std::type_info
#endif

#include <iosfwd>  // std::ostream
#include <string>  // to enable std::string printing. <string> is included already from any_type_holder.h.

#include "type_properties.h"
#include "bad_cast_exception.h"
#include "any_type_holder.h"




namespace hz {



/// An object of this type can contain any variable of any type in it.
class any_type {
	public:

		/// Constructor
		any_type()
			: content(0)
		{ }

		/// Construct from a value
		template<typename ValueType>
		any_type(const ValueType& value)
			: content(new internal::AnyHolder<ValueType>(value))
		{ }

		/// Copy constructor
		any_type(const any_type& other)
			: content(other.content ? other.content->clone() : 0)
		{ }

		/// Destructor
        ~any_type()
        {
            delete content;
        }

		/// Swap *this with rhs
		inline any_type& swap(any_type& rhs);


		/// Check whether the wrapped variable is set
		bool empty() const
		{
			return !content;
		}

		/// Remove the wrapped variable
		void clear()
		{
			delete content;
			content = 0;
		}


#if !(defined DISABLE_RTTI && DISABLE_RTTI)
		/// Get type info of the wrapped variable
		const std::type_info& type() const
		{
			return (content ? content->type() : typeid(void));
		}


		/// Check whether the wrapped variable is of type T.
		template<typename T> inline
		bool is_type() const;  // e.g. is_type<int>
#endif



		/// Get the value into a variable of _exactly_ the same type.
		/// \return false if casting fails
		template<typename T> inline
		bool get(T& put_it_here) const;


		/// Get the value using _exactly_ the same type.
		/// \throw bad_any_cast if casting fails
		template<typename T> inline
		T get() const;  // this may throw if cast fails!


#if !(defined DISABLE_ANY_CONVERT && DISABLE_ANY_CONVERT)
		/// Convert the value into a castable type using any_convert().
		template<typename T> inline
		bool convert(T& val) const;

		/// Convert the value into a castable type using any_convert().
		/// \throw bad_any_cast if casting fails
		template<typename T> inline
		T convert() const;
#endif



		/// Assignment operator from specific type
		template<typename ValueType>
		any_type& operator= (const ValueType& value)
		{
			delete content;
			content = new internal::AnyHolder<ValueType>(value);
			return *this;
		}


		/// Assignment operator from another any_type
		any_type& operator= (const any_type& other)
		{
			delete content;
			content = other.content->clone();
			return *this;
		}



		/// A helper class for easy operator<<() usage.
		struct StreamHelper {
			StreamHelper(internal::AnyHolderBase* c) : content(c) { }
			internal::AnyHolderBase* content;
		};


		/// A helper function to send any_type to a stream.
		/// Example:  std::cerr << any_type_object.to_stream()
		StreamHelper to_stream() const
		{
			return StreamHelper(content);
		}



		// helper classes for enabling <<.

		/// A helper struct for operator<<()
		template<typename T>
		struct is_default_printable
			: public type_integral_constant<bool, (type_is_integral<T>::value
				|| type_is_floating_point<T>::value || type_is_pointer<T>::value)> { };


		/// A helper struct for operator<<()
		template<bool b>
		struct printable {
			static const bool value = b;
		};

		/// A helper struct for operator<<().
		// You may specialize this to <A>, with "public printable<true>" to enable default printing
		template<typename T>
		struct set_printable : public printable<false> { };

		/// A helper struct for operator<<()
		template<typename T>
		struct is_printable {
			static const bool value = (is_default_printable<T>::value || set_printable<T>::value);
		};



		internal::AnyHolderBase* content;  ///< Pointer to internal::AnyHolder<T>

};



/// Operator<<() for any_type::StreamHelper. See any_type::to_stream().
inline std::ostream& operator<< (std::ostream& os, const any_type::StreamHelper& sh)
{
	if (sh.content)
		sh.content->to_stream(os);
	return os;
}




/// This is thrown in case of bad reference casts
DEFINE_BAD_CAST_EXCEPTION(bad_any_cast,
		"Data type mismatch for AnyType. Original type: \"%s\", requested type: \"%s\".",
		"Data type mismatch for AnyType.");




// ---------------------------------------------------------
// any_cast operators



/// Cast a "pointer to any_type" to a "pointer to value_type"
template<typename ValueType>
ValueType any_cast(any_type* operand)
{
	typedef typename type_remove_pointer<ValueType>::type nopointer;

	if (operand && operand->content
#if !(defined DISABLE_RTTI && DISABLE_RTTI)
		&& operand->type() == typeid(nopointer)
#endif
		) {
		return &static_cast<internal::AnyHolder<nopointer>*>(operand->content)->value;
	}

	return 0;
}


/// Cast a "pointer to any_type" to a "pointer to value_type"
template<typename ValueType>
const ValueType any_cast(const any_type* operand)
{
	return any_cast<ValueType>(const_cast<any_type*>(operand));
}




/// Cast an any_type object to value_type
template<typename ValueType>
ValueType any_cast(any_type& operand)
{
	typedef typename type_remove_reference<ValueType>::type nonref;

	if (!operand.content)
		THROW_CUSTOM_BAD_CAST(bad_any_cast, operand.type(), typeid(nonref));

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	if (operand.type() != typeid(nonref))
		THROW_CUSTOM_BAD_CAST(bad_any_cast, operand.type(), typeid(nonref));
#endif

	return static_cast<internal::AnyHolder<nonref>*>(operand.content)->value;
}


/// Cast an any_type object to value_type
template<typename ValueType>
ValueType any_cast(const any_type& operand)
{
	typedef typename type_remove_reference<ValueType>::type nonref;

	if (!operand.content)
		THROW_CUSTOM_BAD_CAST(bad_any_cast, operand.type(), typeid(nonref));

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	if (operand.type() != typeid(nonref))
		THROW_CUSTOM_BAD_CAST(bad_any_cast, operand.type(), typeid(nonref));
#endif

	return static_cast<const internal::AnyHolder<nonref>*>(operand.content)->value;
}




// ---------------------------------------------------------


inline any_type& any_type::swap(any_type& rhs)
{
	// don't use std::swap(), we don't need <algorithm> just for one function.
	internal::AnyHolderBase* tmp = rhs.content;
	rhs.content = content;
	content = tmp;
	return *this;
}


#if !(defined DISABLE_RTTI && DISABLE_RTTI)

template<typename T> inline
bool any_type::is_type() const
{
	// without rtti, any_cast<T*> will always return true.
	return static_cast<bool>(any_cast<T*>(this));
}

#endif


template<typename T> inline
bool any_type::get(T& put_it_here) const
{
	T* a = any_cast<T*>(this);
	if (!a)
		return false;
	put_it_here = *a;
	return true;
}


template<typename T> inline
T any_type::get() const
{
	return any_cast<T>(*this);
}



#if !(defined DISABLE_ANY_CONVERT && DISABLE_ANY_CONVERT)

template<typename T> inline
bool any_type::convert(T& val) const
{
	return (content ? content->convert(val) : false);
}


template<typename T> inline
T any_type::convert() const
{
	typedef typename type_remove_reference<T>::type nonref;
	nonref result;
	if (!this->convert(result))
		THROW_CUSTOM_BAD_CAST(bad_any_cast, this->type(), typeid(nonref));

	return result;
}

#endif



// ---------------------------------------------------------


namespace internal {

	/// You may specialize this for <T, true> to enable custom printing.
	template <class ValueType, bool b = !any_type::is_printable<ValueType>::value >
	struct AnyPrinter {
		static void to_stream(std::ostream& os, const ValueType& value)
		{
			os << "[non-representable]";
		}
	};

	/// Specialization
	template <class ValueType>
	struct AnyPrinter<ValueType, false> {
		static void to_stream(std::ostream& os, const ValueType& value)
		{
			os << value;
		}
	};


	template<typename ValueType> inline
	void AnyHolder<ValueType>::to_stream(std::ostream& os) const
	{
		AnyPrinter<ValueType>::to_stream(os, value);
	}


}




// Do NOT define operator<< for any_type !
// Since any C++ type is implicitly convertable to any_type, and it will
// serve all unrelated types and may even end up in an endless loop.



/// Use this to add supported types to any_type::to_stream().
/// Any type may be added if it supports operator <<.
#define ANY_TYPE_SET_PRINTABLE(type, truefalse) \
	namespace hz { \
		template<> struct any_type::set_printable<type> : public any_type::printable<truefalse> { }; \
	}



}  // ns



/// Make std::string printable using any_type::to_stream().
ANY_TYPE_SET_PRINTABLE(std::string, true)





#endif

/// @}
