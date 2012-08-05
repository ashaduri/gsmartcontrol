/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_SYNC_LOCK_PTR_H
#define HZ_SYNC_LOCK_PTR_H

#include "hz_config.h"  // feature macros

#include "type_properties.h"  // type_*
#include "intrusive_ptr.h"



namespace hz {


namespace internal {

	/// Generic data holder to allow references and pointers only.
	template<class Obj, class ScopedLock>
	struct sync_lock_ptr_data { };



	/// Reference specialization
	template<class Obj, class ScopedLock>
	struct sync_lock_ptr_data<Obj&, ScopedLock> : public hz::intrusive_ptr_referenced {

		typedef Obj ref_type;
		typedef typename type_add_const<ref_type>::type const_ref_type;
		typedef typename hz::type_remove_reference<Obj>::type* ptr_type;
		typedef typename hz::type_add_const<ptr_type>::type const_ptr_type;

		sync_lock_ptr_data(Obj o, ScopedLock* l) : obj(o), lock(l)
		{ }

		~sync_lock_ptr_data()
		{
			release_lock();
		}

		void release_lock()
		{
			delete lock;
			lock = 0;
		}

		ptr_type operator->()
		{
			return &obj;
		}

		const_ptr_type operator->() const
		{
			return &obj;
		}

		ref_type operator*()
		{
			return obj;
		}

		const_ref_type operator*() const
		{
			return obj;
		}

		Obj obj;
		ScopedLock* lock;
	};



	/// Pointer specialization
	template<class Obj, class ScopedLock>
	struct sync_lock_ptr_data<Obj*, ScopedLock> : public hz::intrusive_ptr_referenced {

		typedef typename hz::type_add_reference<typename hz::type_remove_pointer<Obj>::type>::type ref_type;
		typedef typename type_add_const<ref_type>::type const_ref_type;
		typedef Obj ptr_type;
		typedef typename hz::type_add_const<ptr_type>::type const_ptr_type;

		sync_lock_ptr_data(Obj o, ScopedLock* l) : obj(o), lock(l)
		{ }

		~sync_lock_ptr_data()
		{
			release_lock();
		}

		void release_lock()
		{
			delete lock;
			lock = 0;
		}

		ptr_type operator->()
		{
			return obj;
		}

		const_ptr_type operator->() const
		{
			return obj;
		}

		ref_type operator*()
		{
			return *obj;
		}

		const_ref_type operator*() const
		{
			return *obj;
		}

		Obj obj;
		ScopedLock* lock;
	};


}



/// This is a reference-counting smart pointer which:
/// 	- Accepts an object reference or pointer and scoped lock pointer pair.
/// 	- Overloads operator->() to access the object.
/// 	- Releases the scoped lock (via delete) when it dies.
/// 
/// This allows you to return a locked sync_lock_ptr<Object&> from
/// functions which would return Object& had there been no locking.
/// As soon as last copy of that is destroyed (out of scope, etc...),
/// the lock is released.
template<class Obj, class ScopedLock>
class sync_lock_ptr {

	private:
		typedef internal::sync_lock_ptr_data<Obj, ScopedLock> data_type;  ///< Implementation type
		typedef bool (sync_lock_ptr::*unspecified_bool_type)() const;  ///< Bool conversion helper

	public:

		/// Constructor
		sync_lock_ptr(Obj o, ScopedLock* lock) : data_(new data_type(o, lock))
		{ }

		/// Copy constructor
		sync_lock_ptr(const sync_lock_ptr& other) : data_(other.data_)
		{ }

		/// Assignment operator
		sync_lock_ptr& operator=(const sync_lock_ptr& other)
		{
			this->data_ = other.data_;  // copy intrusive_ptr
			return *this;
		}


		/// Release the lock
		void release_lock()
		{
			if (data_)
				(*data_).release_lock();
		}

		/// Get a copy of the object
		Obj get() const
		{
			return (*data_).obj;
		}

		/// Arrow operator
		typename data_type::ptr_type operator->() const
		{
			return (*data_).operator->();
		}

		/// Dereference operator
		typename data_type::ref_type operator*() const
		{
			return (*data_).operator*();
		}

		/// Null pointer check operator
		operator unspecified_bool_type() const
		{
			// in this case, &operator!, being a valid pointer-to-member-function,
			// serves as a "true" value, while 0 (being a null-pointer) serves as false.
			return (operator->() ? &sync_lock_ptr::operator-> : 0);
		}


	private:
		hz::intrusive_ptr<data_type> data_;  ///< The data (the object and the lock pointer)

};






}  // ns



#endif

/// @}
