/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_OPTIONAL_VALUE_H
#define HZ_OPTIONAL_VALUE_H

#include "hz_config.h"  // feature macros

#include <ostream>  // std::ostream, operator<<(ostream, const char*). iosfwd is not enough.


// A simple wrapper for type T, providing additional
// state - defined / undefined.


namespace hz {


template<class T>
class OptionalValue {

	public:

		typedef T value_type;


		OptionalValue() : value_(), defined_(true)
		{ }


		OptionalValue(const T& v) : value_(v), defined_(true)
		{ }


		OptionalValue operator= (const OptionalValue<T>& other)
		{
			value_ = other.value_;
			defined_ = other.defined_;
			return *this;
		}

		OptionalValue operator= (OptionalValue<T>& other)
		{
			value_ = other.value_;
			defined_ = other.defined_;
			return *this;
		}

		OptionalValue operator= (const T& v)
		{
			value_ = v;
			defined_ = true;
			return *this;
		}

		OptionalValue operator= (T& v)
		{
			value_ = v;
			defined_ = true;
			return *this;
		}


		bool operator== (const OptionalValue<T>& v)
		{
			return (defined_ == v.defined_ && value_ == v.value_);
		}


		bool operator== (const T& v)
		{
			return (defined_ && value == v);
		}


		void reset()
		{
			defined_ = false;
			value_ = T();  // how else? (it should be needed, e.g. to clear ref-ptr)
		}


		const T& value() const
		{
			return value_;
		}

		bool defined() const
		{
			return defined_;
		}


	private:

		T value_;

		bool defined_;

};



template<class T> inline
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
