/**************************************************************************
 Copyright:
      (C) 1998 - 2009  Greg Colvin and Beman Dawes
      (C) 2001 - 2009  Peter Dimov
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_SCOPED_ARRAY_H
#define HZ_SCOPED_ARRAY_H

#include "hz_config.h"  // feature macros

/*
Scoped non-reference-counting auto-deletign array.
Based on boost::scoped_array.

Original notes and copyright info follow:
*/

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_array.htm
//


#include <cstddef> // std::ptrdiff_t

#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif



namespace hz {


//  scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
//  is guaranteed, either on destruction of the scoped_array or via an explicit
//  reset(). Use shared_array or std::vector if your needs are more complex.

template<typename T>
class scoped_array {  // noncopyable

	private:
		T* ptr;

		scoped_array(const scoped_array&);
		scoped_array& operator=(const scoped_array&);

		typedef scoped_array<T> this_type;
		typedef T* this_type::*unspecified_bool_type;

		void operator==(const scoped_array&) const;
		void operator!=(const scoped_array&) const;

	public:

		typedef T element_type;

		explicit scoped_array(T* p = 0) : ptr(p) // never throws
		{ }

		~scoped_array() // never throws
		{
			delete[] (ptr);
			ptr = 0;  // exception-safety (needed here?)
		}

		void reset(T* p = 0) // never throws
		{
			ASSERT(p == 0 || p != ptr);  // catch self-reset errors
			this_type(p).swap(*this);
		}

		T& operator[](std::ptrdiff_t i) const  // never throws
		{
			ASSERT(ptr != 0);
			ASSERT(i >= 0);
			return ptr[i];
		}

		T* get() const // never throws
		{
			return ptr;
		}


		// get a reference to the stored pointer.
		// useful for passing to functions who expect T**.
		T*& get_ref()  // never throws
		{
			return ptr;
		}


		// bool-like conversion
		operator unspecified_bool_type () const
		{
			return ptr == 0 ? 0 : &this_type::ptr;
		}

		bool operator! () const // never throws
		{
			return ptr == 0;
		}


		void swap(scoped_array& b) // never throws
		{
			T * tmp = b.ptr;
			b.ptr = ptr;
			ptr = tmp;
		}

};



template<typename T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b)  // never throws
{
	a.swap(b);
}



}  // ns



#endif  // hg
