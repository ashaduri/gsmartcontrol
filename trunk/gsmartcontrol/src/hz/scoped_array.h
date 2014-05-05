/**************************************************************************
 Copyright:
      (C) 1998 - 2010  Greg Colvin and Beman Dawes
      (C) 2001 - 2010  Peter Dimov
      (C) 2008 - 2013  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/
/// \file
/// \author Greg Colvin and Beman Dawes
/// \author Peter Dimov
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_SCOPED_ARRAY_H
#define HZ_SCOPED_ARRAY_H

#include "hz_config.h"  // feature macros

/**
\file
Scoped non-reference-counting auto-deletign array.
Based on boost::scoped_array.

Original notes and copyright info follow:

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_array.htm
//
*/


#include <cstddef> // std::ptrdiff_t

#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif



namespace hz {


///  scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
///  is guaranteed, either on destruction of the scoped_array or via an explicit
///  reset(). Use shared_array or std::vector if your needs are more complex.
template<typename T>
class scoped_array {  // noncopyable

	private:
		T* ptr;  ///< The underlying array pointer

		/// Private copy constructor
		scoped_array(const scoped_array&);

		/// Private assignment operator
		scoped_array& operator=(const scoped_array&);

		typedef scoped_array<T> this_type;  ///< Own type
		typedef T* this_type::*unspecified_bool_type;  ///< Helper for bool conversions

		/// Private operator
		void operator==(const scoped_array&) const;

		/// Private operator
		void operator!=(const scoped_array&) const;

	public:

		/// Underlying type (without pointer)
		typedef T element_type;

		/// Constructor, gets hold of the array ownership
		explicit scoped_array(T* p = 0) : ptr(p) // never throws
		{ }

		/// Destructor, deletes the array
		~scoped_array() // never throws
		{
			delete[] (ptr);
			ptr = 0;  // exception-safety (needed here?)
		}

		/// Delete the old array and switch to the new one
		void reset(T* p = 0) // never throws
		{
			ASSERT(p == 0 || p != ptr);  // catch self-reset errors
			this_type(p).swap(*this);
		}

		/// Element access
		T& operator[](std::ptrdiff_t i) const  // never throws
		{
			ASSERT(ptr != 0);
			ASSERT(i >= 0);
			return ptr[i];
		}

		/// Get the underlying pointer
		T* get() const // never throws
		{
			return ptr;
		}


		/// Get a reference to the stored pointer.
		/// Useful for passing to functions who expect T**.
		T*& get_ref()  // never throws
		{
			return ptr;
		}


		/// Bool-like conversion
		operator unspecified_bool_type () const
		{
			return ptr == 0 ? 0 : &this_type::ptr;
		}

		/// Null-check
		bool operator! () const // never throws
		{
			return ptr == 0;
		}

		/// Swap with another array
		void swap(scoped_array& b) // never throws
		{
			T * tmp = b.ptr;
			b.ptr = ptr;
			ptr = tmp;
		}

};



/// Swap two arrays
template<typename T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b)  // never throws
{
	a.swap(b);
}



}  // ns



#endif  // hg

/// @}
