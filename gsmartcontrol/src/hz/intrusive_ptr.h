/**************************************************************************
 Copyright:
      (C) 2001 - 2010  Peter Dimov
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
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

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	#include <typeinfo>  // std::type_info
#endif

#include "exceptions.h"  // THROW_FATAL


/// \def INTRUSIVE_PTR_TRACING
/// Defined to 0 (default if undefined) or 1. This enables reference count
/// and lock tracing (very verbose).
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
#if !(defined DISABLE_RTTI && DISABLE_RTTI)
			: type_(typeid(void))
#endif
	{
		std::size_t buf_len = std::strlen(msg) + 1;
		msg_ = std::strncpy(new char[buf_len], msg, buf_len);
	}

#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	/// Constructor
	intrusive_ptr_error(const char* msg, const std::type_info& type)
		: msg_(0), type_(type)
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
#endif

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

	char* msg_;  ///< Error message
#if !(defined DISABLE_RTTI && DISABLE_RTTI)
	const std::type_info& type_;  ///< Type information of the pointee
#endif
};



/// \def INTRUSIVE_PTR_THROW(cond, type, msg)
/// If \ref INTRUSIVE_PTR_RUNTIME_CHECKS is 1, check
/// \c cond and throw \ref intrusive_ptr_error. If \ref INTRUSIVE_PTR_RUNTIME_CHECKS
/// is 0, ignore \c cond and don't throw anything.
/// If \ref INTRUSIVE_PTR_TRACING is 1, print message and some
/// additional information to std::cerr.



#if !(defined DISABLE_RTTI && DISABLE_RTTI)

	#if defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING \
			&& defined INTRUSIVE_PTR_RUNTIME_CHECKS && INTRUSIVE_PTR_RUNTIME_CHECKS
		#define INTRUSIVE_PTR_THROW(cond, type, msg) \
			if (cond) { \
				std::cerr << (msg) << " Type: " << type.name() << "\n"; \
				THROW_FATAL(intrusive_ptr_error(msg, type)); \
			}

	#elif defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING
		#define INTRUSIVE_PTR_THROW(cond, type, msg) \
			{ \
				std::cerr << (msg) << " Type: " << type.name() << "\n"; \
			}

	#elif defined INTRUSIVE_PTR_RUNTIME_CHECKS && INTRUSIVE_PTR_RUNTIME_CHECKS
		#define INTRUSIVE_PTR_THROW(cond, type, msg) \
			if (cond) { \
				THROW_FATAL(intrusive_ptr_error(msg, type)); \
			}

	#else
		#define INTRUSIVE_PTR_THROW(cond, type, msg) { }
	#endif


#else  // no rtti

	#if defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING \
			&& defined INTRUSIVE_PTR_RUNTIME_CHECKS && INTRUSIVE_PTR_RUNTIME_CHECKS
		#define INTRUSIVE_PTR_THROW(cond, type, msg) \
			if (cond) { \
				std::cerr << (msg) << "\n"; \
				THROW_FATAL(intrusive_ptr_error(msg)); \
			}

	#elif defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING
		#define INTRUSIVE_PTR_THROW(cond, type, msg) \
			{ \
				std::cerr << (msg) << "\n"; \
			}

	#elif defined INTRUSIVE_PTR_RUNTIME_CHECKS && INTRUSIVE_PTR_RUNTIME_CHECKS
		#define INTRUSIVE_PTR_THROW(cond, type, msg) \
			if (cond) { \
				THROW_FATAL(intrusive_ptr_error(msg)); \
			}

	#else
		#define INTRUSIVE_PTR_THROW(cond, type, msg) { }
	#endif

#endif


/// \def INTRUSIVE_PTR_TRACING(msg)
/// Print a message to std::cerr if \ref INTRUSIVE_PTR_TRACING is 1.
#if defined INTRUSIVE_PTR_TRACING && INTRUSIVE_PTR_TRACING
	#define INTRUSIVE_PTR_TRACE_MSG(msg) { std::cerr << msg << "\n"; }
#else
	#define INTRUSIVE_PTR_TRACE_MSG(msg) { }
#endif



/// Default threading policy for intrusive_ptr wrapped classes.
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



/// If using intrusive_ptr_referenced_self_locked, this provides
/// the default functions to use when locking the wrapped object.
template<class Child>
struct IntrusivePtrChildLockFunctionsDefault {

	/// Increase reference count
	static void ref_lock(Child* p)
	{
		INTRUSIVE_PTR_THROW(!p, typeid(Child), "IntrusivePtrChildLockFunctionsDefault::ref_lock(): Error: NULL pointer passed!");
		INTRUSIVE_PTR_TRACE_MSG("IntrusivePtrChildLockFunctionsDefault::ref_lock() called.");
		p->ref_lock();
	}

	/// Decrease reference count
	static void ref_unlock(Child* p)
	{
		INTRUSIVE_PTR_THROW(!p, typeid(Child), "IntrusivePtrChildLockFunctionsDefault::ref_unlock(): Error: NULL pointer passed!");
		INTRUSIVE_PTR_TRACE_MSG("IntrusivePtrChildLockFunctionsDefault::ref_unlock() called.");
		p->ref_unlock();
	}

};



/// Convenience class to use as parent for user classes to provide
/// intrusive_ptr support (reference counting functions).
class intrusive_ptr_referenced {
	public:

		/// Constructor
		intrusive_ptr_referenced() : ref_count_(0)
		{ }

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

		mutable int ref_count_;  ///< Reference count

};



/// Same as intrusive_ptr_referenced, but with locking multi-threading policy
/// (mutex included) for thread-safe reference counting.
template<class LockPolicy>
class intrusive_ptr_referenced_locked {
	public:

		typedef LockPolicy intrusive_ptr_lock_policy;  ///< Locking policy
		typedef typename LockPolicy::ScopedLock intrusive_ptr_scoped_lock_type;  ///< Scoped lock type

		/// Constructor
		intrusive_ptr_referenced_locked() : ref_count_(0)
		{ }

		/// Increase reference count
		int inc_ref() const
		{
			intrusive_ptr_scoped_lock_type locker(ref_mutex_);
			return (++ref_count_);
		}

		/// Decrease reference count
		int dec_ref() const
		{
			intrusive_ptr_scoped_lock_type locker(ref_mutex_);
			INTRUSIVE_PTR_THROW(ref_count_ <= 0, typeid(void), "intrusive_ptr_referenced_locked::dec_ref(): ref_count <= 0 and decrease request received!");
			return (--ref_count_);
		}

		/// Get reference count
		int	ref_count() const
		{
			intrusive_ptr_scoped_lock_type locker(ref_mutex_);
			return ref_count_;
		}

		/// Get protecting mutex
		typename LockPolicy::Mutex& get_ref_mutex()
		{
			return ref_mutex_;
		}


	protected:

		mutable int ref_count_;  ///< Reference count
		mutable typename LockPolicy::Mutex ref_mutex_;  ///< Protecting mutex

};



/// Same as intrusive_ptr_referenced_locked, but with locking functions and
/// mutex inside the child class. This is useful if the child already has them.
template<class Child, class LockFunctions = IntrusivePtrChildLockFunctionsDefault<Child> >
class intrusive_ptr_referenced_self_locked {
	public:

		/// Constructor
		intrusive_ptr_referenced_self_locked() : ref_count_(0)
		{ }

		/// Increase reference count
		int inc_ref() const
		{
			LockFunctions::ref_lock(static_cast<Child*>(this));  // this uses less memory than Locker.
			int c = ++ref_count_;
			LockFunctions::ref_unlock(static_cast<Child*>(this));
			return c;
		}

		/// Decrease reference count
		int dec_ref() const
		{
			Locker locker(this);  // we have to use this here because of its fuzzy scope
			INTRUSIVE_PTR_THROW(ref_count_ <= 0, typeid(void), "intrusive_ptr_referenced_self_locked::dec_ref(): ref_count <= 0 and decrease request received!");
			return (--ref_count_);
		}

		/// Get reference count
		int	ref_count() const
		{
			LockFunctions::ref_lock(static_cast<Child*>(this));
			int c = ref_count_;
			LockFunctions::ref_unlock(static_cast<Child*>(this));
			return c;
		}


	protected:

		mutable int ref_count_;  /// Reference count


	private:

		/// Scoped locker object
		struct Locker {
			/// Constructor, locks \c self_
			Locker(intrusive_ptr_referenced_self_locked* self_) : self(self_)
			{
				LockFunctions::ref_lock(static_cast<Child*>(self));
			}

			/// Destructor, unlocks \c self_
			~Locker()
			{
				LockFunctions::ref_unlock(static_cast<Child*>(self));
			}

			intrusive_ptr_referenced_self_locked* self;  ///< An object to lock
		};

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


#if !(defined DISABLE_RTTI && DISABLE_RTTI)

/// Perform a dynamic cast on intrusive_ptr
template<class T, class R, class U> inline
intrusive_ptr<T> ptr_dynamic_cast(const intrusive_ptr<U, R>& p)
{
    return dynamic_cast<T*>(p.get());
}

#endif



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
