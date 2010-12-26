/**************************************************************************
 Copyright:
      (C) 1998 - 2009  Greg Colvin and Beman Dawes
      (C) 2001 - 2009  Peter Dimov
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/

#ifndef HZ_SCOPED_PTR_H
#define HZ_SCOPED_PTR_H

#include "hz_config.h"  // feature macros

/*
Scoped non-reference-counting smart pointer with custom cleanup
function. Based on boost::scoped_ptr.

Original notes and copyright info follow:
*/

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_ptr.htm
//

#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif



namespace hz {



namespace internal {

	template<typename T>
	struct scoped_ptr_cleaner_base {
		virtual ~scoped_ptr_cleaner_base()
		{ }

		virtual void cleanup(T* p) = 0;
	};

	template<typename T, typename F>
	struct scoped_ptr_cleaner : public scoped_ptr_cleaner_base<T> {
		scoped_ptr_cleaner(F f) : clean_func(f)
		{ }

		void cleanup(T* p)
		{
			clean_func(p);
		}

		F clean_func;
	};

}




template<typename T>
class scoped_ptr {  // non-copyable

	private:
	    T* ptr;
	    internal::scoped_ptr_cleaner_base<T>* cleaner;

    	scoped_ptr (const scoped_ptr&);
    	scoped_ptr& operator= (const scoped_ptr&);

	    typedef scoped_ptr<T> this_type;
		typedef T* this_type::*unspecified_bool_type;

	    void operator== (const scoped_ptr&) const;
    	void operator!= (const scoped_ptr&) const;


	public:

    	typedef T element_type;

	    explicit scoped_ptr(T* p = 0)  // never throws
	    		: ptr(p), cleaner(0)
    	{ }

		template<typename F>
		explicit scoped_ptr(T* p, F cleanup_func)
				: ptr(p), cleaner(new internal::scoped_ptr_cleaner<T, F>(cleanup_func))
		{ }

	    ~scoped_ptr()  // never throws
    	{
			if (cleaner) {
				if (ptr)
					cleaner->cleanup(ptr);
				delete cleaner;
			} else {
				delete ptr;
			}
    	}

		void reset(T* p = 0)  // never throws
		{
			ASSERT(p == 0 || p != ptr);  // catch self-reset errors
			this_type(p).swap(*this);
		}

		T& operator*() const  // never throws
		{
			ASSERT(ptr);
			return *ptr;
		}

		T* operator->() const  // never throws
		{
			ASSERT(ptr);
			return ptr;
		}

		T* get() const  // never throws
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

		bool operator! () const  // never throws
		{
			return ptr == 0;
		}


		void swap(scoped_ptr& b)  // never throws
		{
			T * tmp = b.ptr;
			b.ptr = ptr;
			ptr = tmp;
		}

};



template<typename T> inline
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b)  // never throws
{
	a.swap(b);
}


// get_pointer(p) is a generic way to say p.get()

template<typename T> inline
T* get_pointer(const scoped_ptr<T>& p)
{
	return p.get();
}



}  // ns




#endif  // hg
