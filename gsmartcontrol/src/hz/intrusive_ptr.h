/**************************************************************************
 Copyright:
      (C) 2001 - 2010  Peter Dimov
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_boost_1_0.txt file
***************************************************************************/
/// \file
/// \author Peter Dimov
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_INTRUSIVE_PTR_H
#define HZ_INTRUSIVE_PTR_H

#include "hz_config.h"  // feature macros

/**
\file
Intrusive reference-counting smart pointer, based on boost::intrusive_ptr.
Original notes and copyright info follow:

intrusive_ptr.hpp

Copyright (c) 2001, 2002 Peter Dimov

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)

See http://www.boost.org/libs/smart_ptr/intrusive_ptr.html for documentation.
*/


#include <cstddef>  // std::size_t
#include <cstring>  // strncpy / strlen
#include <exception>  // std::exception
// #include <iosfwd>  // std::ostream (for operator<<)
#include <typeinfo>  // std::type_info


/// \def INTRUSIVE_PTR_TRACING
/// Defined to 0 (default if undefined) or 1. This enables reference count tracing (very verbose).
#ifndef INTRUSIVE_PTR_TRACING
	#define INTRUSIVE_PTR_TRACING 0
#endif

/// \def INTRUSIVE_PTR_RUNTIME_CHECKS
/// Defined to 0 or 1 (default if undefined). This enables runtime checks
/// for errors (with exception throwing).
#ifndef INTRUSIVE_PTR_RUNTIME_CHECKS
	#define INTRUSIVE_PTR_RUNTIME_CHECKS 1
#endif



#if defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING
	// Don't use libdebug here - it uses us and bad things may happen
	// if there's something wrong with it.
	// Use plain std::cerr. It will be disabled by default anyway.
	#include <iostream>  // std::cerr
#endif



namespace hz {




/// This is thrown on refcount / null pointer errors, if \c DEBUG_BUILD
/// or \ref INTRUSIVE_PTR_TRACING are defined.
struct intrusive_ptr_error : virtual public std::exception {  // from <exception>

	/// Constructor
	intrusive_ptr_error(const char* msg)
			: type_(typeid(void))
	{
		std::size_t buf_len = std::strlen(msg) + 1;
		msg_ = std::strncpy(new char[buf_len], msg, buf_len);
	}

	/// Constructor
	intrusive_ptr_error(const char* msg, const std::type_info& type)
		: type_(type)
	{
		const char* tname = type.name();
		const char* tmsg = " Type: ";
		std::size_t msg_len = std::strlen(msg);
		std::size_t tmsg_len = std::strlen(tmsg);
		std::size_t tname_len = std::strlen(tname);
		msg_ = new char[msg_len + tmsg_len + tname_len + 1];
		std::strncpy(msg_, msg, msg_len);
		std::strncpy(msg_ + msg_len, tmsg, tmsg_len);
		std::strncpy(msg_ + msg_len + tmsg_len, tname, tname_len + 1);
	}

	/// Virtual destructor
	virtual ~intrusive_ptr_error() throw()
	{
		delete[] msg_;
		msg_ = 0;  // protect from double-deletion compiler bugs
	}

	/// Reimplemented from std::exception.
	/// Note: messages in exceptions are not newline-terminated.
	virtual const char* what() const throw()
	{
		return msg_;
	}

	char* msg_ = 0;  ///< Error message
	const std::type_info& type_;  ///< Type information of the pointee
};



/// \def INTRUSIVE_PTR_THROW(cond, type, msg)
/// If \ref INTRUSIVE_PTR_RUNTIME_CHECKS is 1, check
/// \c cond and throw \ref intrusive_ptr_error. If \ref INTRUSIVE_PTR_RUNTIME_CHECKS
/// is 0, ignore \c cond and don't throw anything.
/// If \ref INTRUSIVE_PTR_TRACING is 1, print message and some
/// additional information to std::cerr.


#if defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING \
		&& defined INTRUSIVE_PTR_RUNTIME_CHECKS && INTRUSIVE_PTR_RUNTIME_CHECKS
	#define INTRUSIVE_PTR_THROW(cond, type, msg) \
		if (cond) { \
			std::cerr << (msg) << " Type: " << type.name() << "\n"; \
			throw intrusive_ptr_error(msg, type); \
		}

#elif defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING
	#define INTRUSIVE_PTR_THROW(cond, type, msg) \
		{ \
			std::cerr << (msg) << " Type: " << type.name() << "\n"; \
		}

#elif defined INTRUSIVE_PTR_RUNTIME_CHECKS && INTRUSIVE_PTR_RUNTIME_CHECKS
	#define INTRUSIVE_PTR_THROW(cond, type, msg) \
		if (cond) { \
			throw intrusive_ptr_error(msg, type); \
		}

#else
	#define INTRUSIVE_PTR_THROW(cond, type, msg) { }
#endif


/// \def INTRUSIVE_PTR_TRACING(msg)
/// Print a message to std::cerr if \ref INTRUSIVE_PTR_TRACING is 1.
#if defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING
	#define INTRUSIVE_PTR_TRACE_MSG(msg) { std::cerr << msg << "\n"; }
#else
	#define INTRUSIVE_PTR_TRACE_MSG(msg) { }
#endif



/// Default refcount policy for intrusive_ptr wrapped classes.
/// The wrapped class MUST provide: inc_ref(), dec_ref(), ref_count().
template<class PtrType>
struct IntrusivePtrRefFunctionsDefault {

	/// Increase reference count
	static int inc_ref(PtrType* p)
	{
		INTRUSIVE_PTR_THROW(!p, typeid(PtrType), "IntrusivePtrRefFunctionsDefault::inc_ref(): Error: NULL pointer passed!");
		INTRUSIVE_PTR_TRACE_MSG("IntrusivePtrRefFunctionsDefault::inc_ref(): increasing from " << p->ref_count());
		return p->inc_ref();
	}

	/// Decrease reference count
	static int dec_ref(PtrType* p)
	{
		INTRUSIVE_PTR_THROW(!p, typeid(PtrType), "IntrusivePtrRefFunctionsDefault::dec_ref(): Error: NULL pointer passed!");
		INTRUSIVE_PTR_TRACE_MSG("IntrusivePtrRefFunctionsDefault::dec_ref(): decreasing from " << p->ref_count());

		int c = p->dec_ref();
		if (c == 0) {
			INTRUSIVE_PTR_TRACE_MSG("IntrusivePtrRefFunctionsDefault::dec_ref(): delete " << p);
			delete p;
		}
		return c;
	}

};



/// Convenience class to use as parent for user classes to provide
/// intrusive_ptr support (reference counting functions).
class intrusive_ptr_referenced {
	public:

		// Note: methods are const to allow construction of intrusive_ptr<const T>
		// from const data while still incrementing the refcount.

		/// Increase reference count
		int inc_ref() const
		{
			return (++ref_count_);
		}

		/// Decrease reference count
		int dec_ref() const
		{
			INTRUSIVE_PTR_THROW(ref_count_ <= 0, typeid(void), "intrusive_ptr_referenced::dec_ref(): ref_count <= 0 and decrease request received!");
			return (--ref_count_);
		}

		/// Get reference count
		int	ref_count() const
		{
			return ref_count_;
		}


	protected:

		mutable int ref_count_ = 0;  ///< Reference count

};





/// Intrusive reference-counting smart pointer.
/// \tparam T Pointee type (without *).
/// \tparam RefFunctions Policy for reference counting
template<class T, class RefFunctions = IntrusivePtrRefFunctionsDefault<T> >
class intrusive_ptr {
	private:

		typedef intrusive_ptr<T, RefFunctions> this_type;  ///< Borland-specific workaround

		typedef T* this_type::*unspecified_bool_type;  ///< Helper type for if(p)-style casts.


	public:

		/// Wrapped type
		typedef T element_type;

		/// Constructor, creates a null pointer
		intrusive_ptr() : p_(0)
		{ }

		/// Constructor. Note that the reference count of \c p is preserved
		/// and possibly increased if \c add_ref is true.
		intrusive_ptr(T* p, bool add_ref = true) : p_(p)
		{
			if (p_ != 0 && add_ref)
				RefFunctions::inc_ref(p_);
		}


		/// Construct from other intrusive_ptr with implicitly-convertible pointer type,
		/// increases reference count.
		template<class U, class R>
		intrusive_ptr(const intrusive_ptr<U, R>& rhs) : p_(rhs.get())
		{
			if (p_ != 0)
				RefFunctions::inc_ref(p_);
		}


		/// Copy constructor, increases reference count.
		intrusive_ptr(const intrusive_ptr& rhs) : p_(rhs.p_)
		{
			if (p_ != 0)
				RefFunctions::inc_ref(p_);
		}


		/// Destructor, decreases reference count.
		~intrusive_ptr()
		{
			if (p_ != 0)
				RefFunctions::dec_ref(p_);
		}


		/// Assignment operator from implicitly-convertible pointer type,
		/// increases reference count.
		template<class U, class R>
		intrusive_ptr& operator=(const intrusive_ptr<U, R>& rhs)
		{
			this_type(rhs).swap(*this);
			return *this;
		}


		/// Assignment operator, increases reference count.
		intrusive_ptr& operator=(const intrusive_ptr& rhs)
		{
			this_type(rhs).swap(*this);
			return *this;
		}


		/// Assignment operator from plain pointer
		intrusive_ptr& operator=(T* rhs)
		{
			this_type(rhs).swap(*this);
			return *this;
		}


		/// Get the pointed object
		T* get() const
		{
			return p_;
		}


		/// Dereference operator.
		/// \throw intrusive_ptr_error if this is a null pointer.
		T& operator*() const
		{
			INTRUSIVE_PTR_THROW(!p_, typeid(T), "intrusive_ptr::operator*(): Attempting to dereference NULL pointer!");
			return *p_;
		}

		/// Arrow operator.
		/// \throw intrusive_ptr_error if this is a null pointer.
		T* operator->() const
		{
			INTRUSIVE_PTR_THROW(!p_, typeid(T), "intrusive_ptr::operator->(): Attempting to dereference NULL pointer!");
			return p_;
		}


		/// Bool-like conversion for null checks
		operator unspecified_bool_type () const
		{
			return p_ == 0 ? 0 : &this_type::p_;
		}


		/// Operator! is a Borland-specific workaround
		bool operator! () const
		{
			return p_ == 0;
		}


		/// Swap with other pointer
		void swap(intrusive_ptr& rhs)
		{
			T* tmp = p_;
			p_ = rhs.p_;
			rhs.p_ = tmp;
		}


	private:

		T* p_;  ///< Pointed object

};



/// Comparison operator
template<class T, class R, class U, class S> inline
bool operator==(const intrusive_ptr<T, R>& a, const intrusive_ptr<U, S>& b)
{
	return a.get() == b.get();
}

/// Comparison operator
template<class T, class R, class U, class S> inline
bool operator!=(const intrusive_ptr<T, R>& a, const intrusive_ptr<U, S>& b)
{
	return a.get() != b.get();
}

/// Comparison operator
template<class T, class R, class U> inline
bool operator==(const intrusive_ptr<T, R>& a, U* b)
{
	return a.get() == b;
}

/// Comparison operator
template<class T, class R, class U> inline
bool operator!=(const intrusive_ptr<T, R>& a, U* b)
{
	return a.get() != b;
}

/// Comparison operator
template<class T, class U, class S> inline
bool operator==(T* a, const intrusive_ptr<U, S>& b)
{
	return a == b.get();
}

/// Comparison operator
template<class T, class U, class S> inline
bool operator!=(T* a, const intrusive_ptr<U, S>& b)
{
	return a != b.get();
}


/// Comparison operator
template<class T, class R> inline
bool operator<(const intrusive_ptr<T, R>& a, const intrusive_ptr<T, R>& b)
{
//	return std::less<T *>()(a.get(), b.get());  // needs <functional>
	return a.get() < b.get();
}

/// Swap two pointers
template<class T, class R> inline
void swap(intrusive_ptr<T, R>& lhs, intrusive_ptr<T, R>& rhs)
{
	lhs.swap(rhs);
}


/// Get pointed object. This is for mem_fn support.
template<class T, class R> inline
T* get_pointer(const intrusive_ptr<T, R>& p)
{
	return p.get();
}


/// Perform a static cast on intrusive_ptr
template<class T, class R, class U> inline
intrusive_ptr<T, R> ptr_static_cast(const intrusive_ptr<U, R>& p)
{
    return static_cast<T*>(p.get());
}

/// Perform a const cast on intrusive_ptr
template<class T, class U, class S> inline
intrusive_ptr<T> ptr_const_cast(const intrusive_ptr<U, S>& p)  // return type's second parameter defaulted.
{
    return const_cast<T*>(p.get());
}

/// Perform a const cast on intrusive_ptr
template<class T, class R, class U, class S> inline
intrusive_ptr<T, R> ptr_const_cast(const intrusive_ptr<U, S>& p)
{
    return const_cast<T*>(p.get());
}

/// Perform a dynamic cast on intrusive_ptr
template<class T, class R, class U> inline
intrusive_ptr<T> ptr_dynamic_cast(const intrusive_ptr<U, R>& p)
{
    return dynamic_cast<T*>(p.get());
}



// Don't define this - it may conflict with actual intrusive_ptr<T, R> overloads
// (especially with sun compiler).
/*
// operator<<
template<class T, class R> inline
std::ostream& operator<< (std::ostream& os, const intrusive_ptr<T, R>& p)
{
	os << p.get();
	return os;
}
*/




}  // ns



#endif

/// @}
