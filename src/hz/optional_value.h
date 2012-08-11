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

#ifndef HZ_OPTIONAL_VALUE_H
#define HZ_OPTIONAL_VALUE_H

#include "hz_config.h"  // feature macros

#include <ostream>  // std::ostream, operator<<(ostream, const char*). iosfwd is not enough.

#if __cplusplus > 201100L
	#include <utility>  // move
#endif


namespace hz {


/// A simple wrapper around type T, providing additional state - defined / undefined.
template<class T>
class OptionalValue {
	public:

		/// Wrapped type
		typedef T value_type;


		/// Constructor. Initial value is default-constructed, and the state is undefined.
		OptionalValue() : value_(), defined_(false)
		{ }


#if __cplusplus > 201100L
		/// Construct from value \c v, setting the state to defined.
		explicit OptionalValue(T v) : value_(std::move(v)), defined_(true)
		{ }
#else
		/// Construct from value \c v, setting the state to defined.
		OptionalValue(const T& v) : value_(v), defined_(true)
		{ }
#endif


		// Don't Define any of the following: move, copy, destruct.
		// This will make the compiler generate the defaults.

// 		/// Assignment operator
// 		OptionalValue& operator= (const OptionalValue<T>& other)
// 		{
// 			value_ = other.value_;
// 			defined_ = other.defined_;
// 			return *this;
// 		}

// 		/// Assignment operator
// 		OptionalValue& operator= (OptionalValue<T>& other)
// 		{
// 			value_ = other.value_;
// 			defined_ = other.defined_;
// 			return *this;
// 		}


#if __cplusplus > 201100L
		/// Assignment operator.
		/// \post state is defined.
		OptionalValue& operator= (T v)
		{
			value_ = std::move(v);
			defined_ = true;
			return *this;
		}
#else
		/// Assignment operator.
		/// \post state is defined.
		OptionalValue& operator= (const T& v)
		{
			value_ = v;
			defined_ = true;
			return *this;
		}
#endif


// 		/// Assignment operator.
// 		/// \post state is defined.
// 		OptionalValue& operator= (T& v)
// 		{
// 			value_ = v;
// 			defined_ = true;
// 			return *this;
// 		}


		/// Comparison operator
		bool operator== (const OptionalValue<T>& v)
		{
			return (defined_ == v.defined_ && value_ == v.value_);
		}


		/// Comparison operator
		bool operator== (const T& v)
		{
			return (defined_ && value_ == v);
		}


		/// Set the state to undefined, set internal value to a default-constructed one.
		void reset()
		{
			defined_ = false;
			value_ = T();  // how else? (it should be needed, e.g. to clear ref-ptr)
		}


		/// Get the stored value. You may use this function only if defined() returns true.
		const T& value() const
		{
			return value_;
		}


		/// Check the defined state.
		bool defined() const
		{
			return defined_;
		}


	private:

		T value_;  ///< Wrapped value
		bool defined_;  ///< "Defined" state

};



/// Stream output operator
template<class T>
std::ostream& operator<< (std::ostream& os, const OptionalValue<T>& v)
{
	if (v.defined()) {
		os << v.value();
	} else {
		os << "[value undefined]";
	}
	return os;
}





}  // ns





#endif

/// @}
