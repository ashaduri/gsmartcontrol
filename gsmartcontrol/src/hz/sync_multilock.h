/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_SYNC_MULTILOCK_H
#define HZ_SYNC_MULTILOCK_H

#include "hz_config.h"  // feature macros

#include <cstddef>  // std::size_t

#include "common_types.h"  // NullType
#include "local_algo.h"  // shell_sort
#include "sync_part_get_policy.h"  // internal header, SyncGetPolicy struct.
#include "system_specific.h"  // HZ_GCC_CHECK_VERSION



namespace hz {



// This is a mutex order-based multi-locking facility.

// This link: http://lists.boost.org/Archives/boost/2008/02/132853.php
// has an interesting discussion about several approaches to multi-lock.
// However, the faster the method, the more details it uses from an underlying
// implementation. In such light, the most generic (straightforward) seems
// to be the order-based one. If the user requires a very fast method,
// he/she should roll out their own multi-lock implementation, depending on
// their needs.


// TODO:
// SyncMultiTryLock (.status() returns 0-based index of first failed lock, or -1 on success, as in boost),
// SyncMultiTryLock should unlock the locked mutexes in case of exception during locking.
// SyncMultiTryLockUniType, and SyncMultiLockEmpty variants.


struct MultiLockNullPolicy  // used only in typedef below
{ };

// Specialization for SyncGetPolicy on NullType, to avoid errors.
template<> struct SyncGetPolicy<NullType> {
	typedef MultiLockNullPolicy type;
};



namespace internal {

	// Helper class: index / mutex pointer holder
	struct MultiLockPair {
		MultiLockPair() : m(0), index(0)  // for default construction
		{ }

		MultiLockPair(void* m_, int index_) : m(m_), index(index_)
		{ }

		void* m;  // mutex
		int index;  // 1-based
	};

	inline bool operator< (const MultiLockPair& m1, const MultiLockPair& m2)
	{
		return m1.m < m2.m;  // compare by address
	}



	// Base class for SyncMultiLock. It provides mutex member,
	// and lock/unlock facility. Its specialization for NullType
	// should be eliminated through empty-base-optimization by compiler.
	template<int BaseCounter, class Mutex, class LockPolicy>
	class MultiLockBase {
		protected:
			MultiLockBase(Mutex* m) : mutex(m)
			{ }

			void lock()
			{
				LockPolicy::lock(*mutex);
			}

			bool trylock()
			{
				return LockPolicy::trylock(*mutex);
			}

			void unlock()
			{
				LockPolicy::unlock(*mutex);
			}

		private:
			Mutex* mutex;
	};


	// Specialization for NullType, to disable all activity.
	template<int BaseCounter, class LockPolicy>
	class MultiLockBase<BaseCounter, NullType, LockPolicy> {
		protected:
			void lock()
			{ }

			bool trylock()
			{
				return true;  // no errors here
			}

			void unlock()
			{ }
	};

}




// The main class - SyncMultiLock<>.
// This is a scoped lock supporting multiple mutexes (up to 10),
// locking them always in the same order.
// Note: Mutexes don't have to have the same type.

template<
		class Mutex1,
		class Mutex2 = NullType,
		class Mutex3 = NullType,
		class Mutex4 = NullType,
		class Mutex5 = NullType,
		class Mutex6 = NullType,
		class Mutex7 = NullType,
		class Mutex8 = NullType,
		class Mutex9 = NullType,
		class Mutex10 = NullType,
		class LockPolicy1 = typename SyncGetPolicy<Mutex1>::type,
		class LockPolicy2 = typename SyncGetPolicy<Mutex2>::type,
		class LockPolicy3 = typename SyncGetPolicy<Mutex3>::type,
		class LockPolicy4 = typename SyncGetPolicy<Mutex4>::type,
		class LockPolicy5 = typename SyncGetPolicy<Mutex5>::type,
		class LockPolicy6 = typename SyncGetPolicy<Mutex6>::type,
		class LockPolicy7 = typename SyncGetPolicy<Mutex7>::type,
		class LockPolicy8 = typename SyncGetPolicy<Mutex8>::type,
		class LockPolicy9 = typename SyncGetPolicy<Mutex9>::type,
		class LockPolicy10 = typename SyncGetPolicy<Mutex10>::type >
class SyncMultiLock :
		public internal::MultiLockBase<1, Mutex1, LockPolicy1>,
		public internal::MultiLockBase<2, Mutex2, LockPolicy2>,
		public internal::MultiLockBase<3, Mutex3, LockPolicy3>,
		public internal::MultiLockBase<4, Mutex4, LockPolicy4>,
		public internal::MultiLockBase<5, Mutex5, LockPolicy5>,
		public internal::MultiLockBase<6, Mutex6, LockPolicy6>,
		public internal::MultiLockBase<7, Mutex7, LockPolicy7>,
		public internal::MultiLockBase<8, Mutex8, LockPolicy8>,
		public internal::MultiLockBase<9, Mutex9, LockPolicy9>,
		public internal::MultiLockBase<10, Mutex10, LockPolicy10>
{

	public:

		SyncMultiLock(Mutex1& m1, bool do_lock1 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				mutexes_(0), size_(0)
		{
			if (do_lock1) {
				mutexes_ = new internal::MultiLockPair[1];
				mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
				internal::MultiLockBase<1, Mutex1, LockPolicy1>::lock();
			}
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2,
				bool do_lock1 = true, bool do_lock2 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[2];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[3];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[4];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4, Mutex5& m5,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				internal::MultiLockBase<5, Mutex5, LockPolicy5>(&m5),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[5];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			if (do_lock5) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m5), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4, Mutex5& m5,
				Mutex6& m6,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				internal::MultiLockBase<5, Mutex5, LockPolicy5>(&m5),
				internal::MultiLockBase<6, Mutex6, LockPolicy6>(&m6),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[6];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			if (do_lock5) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m5), size_+1);
			if (do_lock6) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m6), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4, Mutex5& m5,
				Mutex6& m6, Mutex7& m7,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				internal::MultiLockBase<5, Mutex5, LockPolicy5>(&m5),
				internal::MultiLockBase<6, Mutex6, LockPolicy6>(&m6),
				internal::MultiLockBase<7, Mutex7, LockPolicy7>(&m7),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[7];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			if (do_lock5) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m5), size_+1);
			if (do_lock6) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m6), size_+1);
			if (do_lock7) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m7), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4, Mutex5& m5,
				Mutex6& m6, Mutex7& m7, Mutex8& m8,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				internal::MultiLockBase<5, Mutex5, LockPolicy5>(&m5),
				internal::MultiLockBase<6, Mutex6, LockPolicy6>(&m6),
				internal::MultiLockBase<7, Mutex7, LockPolicy7>(&m7),
				internal::MultiLockBase<8, Mutex8, LockPolicy8>(&m8),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[8];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			if (do_lock5) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m5), size_+1);
			if (do_lock6) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m6), size_+1);
			if (do_lock7) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m7), size_+1);
			if (do_lock8) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m8), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4, Mutex5& m5,
				Mutex6& m6, Mutex7& m7, Mutex8& m8, Mutex9& m9,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true, bool do_lock9 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				internal::MultiLockBase<5, Mutex5, LockPolicy5>(&m5),
				internal::MultiLockBase<6, Mutex6, LockPolicy6>(&m6),
				internal::MultiLockBase<7, Mutex7, LockPolicy7>(&m7),
				internal::MultiLockBase<8, Mutex8, LockPolicy8>(&m8),
				internal::MultiLockBase<9, Mutex9, LockPolicy9>(&m9),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[9];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			if (do_lock5) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m5), size_+1);
			if (do_lock6) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m6), size_+1);
			if (do_lock7) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m7), size_+1);
			if (do_lock8) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m8), size_+1);
			if (do_lock9) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m9), size_+1);
			this->sort_lock();
		}


		SyncMultiLock(Mutex1& m1, Mutex2& m2, Mutex3& m3, Mutex4& m4, Mutex5& m5,
				Mutex6& m6, Mutex7& m7, Mutex8& m8, Mutex9& m9, Mutex10& m10,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true, bool do_lock9 = true, bool do_lock10 = true) :
				internal::MultiLockBase<1, Mutex1, LockPolicy1>(&m1),
				internal::MultiLockBase<2, Mutex2, LockPolicy2>(&m2),
				internal::MultiLockBase<3, Mutex3, LockPolicy3>(&m3),
				internal::MultiLockBase<4, Mutex4, LockPolicy4>(&m4),
				internal::MultiLockBase<5, Mutex5, LockPolicy5>(&m5),
				internal::MultiLockBase<6, Mutex6, LockPolicy6>(&m6),
				internal::MultiLockBase<7, Mutex7, LockPolicy7>(&m7),
				internal::MultiLockBase<8, Mutex8, LockPolicy8>(&m8),
				internal::MultiLockBase<9, Mutex9, LockPolicy9>(&m9),
				internal::MultiLockBase<10, Mutex10, LockPolicy10>(&m10),
				mutexes_(0), size_(0)
		{
			mutexes_ = new internal::MultiLockPair[10];
			if (do_lock1) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m1), size_+1);
			if (do_lock2) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m2), size_+1);
			if (do_lock3) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m3), size_+1);
			if (do_lock4) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m4), size_+1);
			if (do_lock5) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m5), size_+1);
			if (do_lock6) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m6), size_+1);
			if (do_lock7) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m7), size_+1);
			if (do_lock8) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m8), size_+1);
			if (do_lock9) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m9), size_+1);
			if (do_lock10) mutexes_[size_++] = internal::MultiLockPair(static_cast<void*>(&m10), size_+1);
			this->sort_lock();
		}





		~SyncMultiLock()
		{
			for (std::size_t i = size_; i > 0; --i) {  // unlock reverse order
				switch (mutexes_[i - 1].index) {
					case 1: internal::MultiLockBase<1, Mutex1, LockPolicy1>::unlock(); break;
					case 2: internal::MultiLockBase<2, Mutex2, LockPolicy2>::unlock(); break;
					case 3: internal::MultiLockBase<3, Mutex3, LockPolicy3>::unlock(); break;
					case 4: internal::MultiLockBase<4, Mutex4, LockPolicy4>::unlock(); break;
					case 5: internal::MultiLockBase<5, Mutex5, LockPolicy5>::unlock(); break;
					case 6: internal::MultiLockBase<6, Mutex6, LockPolicy6>::unlock(); break;
					case 7: internal::MultiLockBase<7, Mutex7, LockPolicy7>::unlock(); break;
					case 8: internal::MultiLockBase<8, Mutex8, LockPolicy8>::unlock(); break;
					case 9: internal::MultiLockBase<9, Mutex9, LockPolicy9>::unlock(); break;
					case 10: internal::MultiLockBase<10, Mutex10, LockPolicy10>::unlock(); break;
				}
			}

			delete[] mutexes_;
		}


		void sort_lock()
		{
			shell_sort(mutexes_, mutexes_ + size_);  // sort by mutex addresses

			for (std::size_t i = 0; i < size_; ++i) {
				switch (mutexes_[i].index) {
					case 1: internal::MultiLockBase<1, Mutex1, LockPolicy1>::lock(); break;
					case 2: internal::MultiLockBase<2, Mutex2, LockPolicy2>::lock(); break;
					case 3: internal::MultiLockBase<3, Mutex3, LockPolicy3>::lock(); break;
					case 4: internal::MultiLockBase<4, Mutex4, LockPolicy4>::lock(); break;
					case 5: internal::MultiLockBase<5, Mutex5, LockPolicy5>::lock(); break;
					case 6: internal::MultiLockBase<6, Mutex6, LockPolicy6>::lock(); break;
					case 7: internal::MultiLockBase<7, Mutex7, LockPolicy7>::lock(); break;
					case 8: internal::MultiLockBase<8, Mutex8, LockPolicy8>::lock(); break;
					case 9: internal::MultiLockBase<9, Mutex9, LockPolicy9>::lock(); break;
					case 10: internal::MultiLockBase<10, Mutex10, LockPolicy10>::lock(); break;
				}
			}
		}


	private:

		internal::MultiLockPair* mutexes_;  // array of MultiLockPair* for sorting
		std::size_t size_;  // array size

		// forbid copying
		SyncMultiLock(const SyncMultiLock& from);
		SyncMultiLock& operator= (const SyncMultiLock& from);

};







// A scoped lock supporting multiple mutexes (up to 10 with positional
// parameters, unlimited with array-based constructors), locking them
// always in the same order. The mutexes must have the same type.

// Compared to a variant above, this one has a slightly less memory
// footprint, is slightly faster and handles unlimited number
// (for all practical purposes) of mutexes, all at the
// cost of requiring mutexes to be of the same type.

template<class Mutex, class LockPolicy = typename SyncGetPolicy<Mutex>::type>
class SyncMultiLockUniType {
	public:


		// -------------------------------- passed as an array


		template<std::size_t size>
		SyncMultiLockUniType(Mutex* (&mutexes)[size], bool do_lock = true)
				: mutexes_(0), size_(0)
		{
			if (size && do_lock) {
				size_ = size;
				mutexes_ = new Mutex*[size_];
				for (std::size_t i = 0; i < size_; ++i)
					mutexes_[i] = mutexes[i];
				this->sort_lock();
			}
		}


		// for gcc earlier than 4.2 this will be selected for STL containers.
		template<template<class ElemType> class Container>
		SyncMultiLockUniType(const Container<Mutex*>& mutexes, bool do_lock = true)
				: mutexes_(0), size_(0)
		{
			if (do_lock) {
				size_ = mutexes.size();
				mutexes_ = new Mutex*[size_];
				std::size_t i = 0;
				for (typename Container<Mutex*>::const_iterator iter = mutexes.begin(); iter != mutexes.end(); ++iter)
					mutexes_[i++] = *iter;
				this->sort_lock();
			}
		}


		// gcc versions earlier than 4.2 have an undocumented extension which
		// allowed templates with default parameters to be bound to template template
		// parameters with fewer parameters. This causes ambiguity errors with
		// this versions. Resolve that.
#if defined(__INTEL_COMPILER) || (!defined __GNUC__) || HZ_GCC_CHECK_VERSION(4, 2, 0)

		// STL containers mainly fall into this one.
		template<class CT2, template<class ElemType, class> class Container>
		SyncMultiLockUniType(const Container<Mutex*, CT2>& mutexes, bool do_lock = true)
				: mutexes_(0), size_(0)
		{
			if (do_lock) {
				size_ = mutexes.size();
				mutexes_ = new Mutex*[size_];
				std::size_t i = 0;
				for (typename Container<Mutex*, CT2>::const_iterator iter = mutexes.begin(); iter != mutexes.end(); ++iter)
					mutexes_[i++] = *iter;
				this->sort_lock();
			}
		}


		template<class CT2, class CT3, template<class ElemType, class, class> class Container>
		SyncMultiLockUniType(const Container<Mutex*, CT2, CT3>& mutexes, bool do_lock = true)
				: mutexes_(0), size_(0)
		{
			if (do_lock) {
				size_ = mutexes.size();
				mutexes_ = new Mutex*[size_];
				std::size_t i = 0;
				for (typename Container<Mutex*, CT2, CT3>::const_iterator iter = mutexes.begin(); iter != mutexes.end(); ++iter)
					mutexes_[i++] = *iter;
				this->sort_lock();
			}
		}
#endif


		// -------------------------------- passed directly


		SyncMultiLockUniType(Mutex& mutex1,
				bool do_lock1 = true) : mutexes_(0), size_(0)
		{
			if (do_lock1) {
				mutexes_ = new Mutex*[1];
				mutexes_[size_++] = &mutex1;  // for unlock to work
				LockPolicy::lock(mutex1);
			}
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2,
				bool do_lock1 = true, bool do_lock2 = true) : size_(0)
		{
			mutexes_ = new Mutex*[2];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true) : size_(0)
		{
			mutexes_ = new Mutex*[3];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true) : size_(0)
		{
			mutexes_ = new Mutex*[4];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true) : size_(0)
		{
			mutexes_ = new Mutex*[5];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			if (do_lock5) mutexes_[size_++] = &mutex5;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true) : size_(0)
		{
			mutexes_ = new Mutex*[6];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			if (do_lock5) mutexes_[size_++] = &mutex5;
			if (do_lock6) mutexes_[size_++] = &mutex6;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true) : size_(0)
		{
			mutexes_ = new Mutex*[7];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			if (do_lock5) mutexes_[size_++] = &mutex5;
			if (do_lock6) mutexes_[size_++] = &mutex6;
			if (do_lock7) mutexes_[size_++] = &mutex7;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7, Mutex& mutex8,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true) : size_(0)
		{
			mutexes_ = new Mutex*[8];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			if (do_lock5) mutexes_[size_++] = &mutex5;
			if (do_lock6) mutexes_[size_++] = &mutex6;
			if (do_lock7) mutexes_[size_++] = &mutex7;
			if (do_lock8) mutexes_[size_++] = &mutex8;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7, Mutex& mutex8, Mutex& mutex9,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true, bool do_lock9 = true) : size_(0)
		{
			mutexes_ = new Mutex*[9];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			if (do_lock5) mutexes_[size_++] = &mutex5;
			if (do_lock6) mutexes_[size_++] = &mutex6;
			if (do_lock7) mutexes_[size_++] = &mutex7;
			if (do_lock8) mutexes_[size_++] = &mutex8;
			if (do_lock9) mutexes_[size_++] = &mutex9;
			this->sort_lock();
		}

		SyncMultiLockUniType(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7, Mutex& mutex8, Mutex& mutex9, Mutex& mutex10,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true, bool do_lock9 = true, bool do_lock10 = true) : size_(0)
		{
			mutexes_ = new Mutex*[10];
			if (do_lock1) mutexes_[size_++] = &mutex1;
			if (do_lock2) mutexes_[size_++] = &mutex2;
			if (do_lock3) mutexes_[size_++] = &mutex3;
			if (do_lock4) mutexes_[size_++] = &mutex4;
			if (do_lock5) mutexes_[size_++] = &mutex5;
			if (do_lock6) mutexes_[size_++] = &mutex6;
			if (do_lock7) mutexes_[size_++] = &mutex7;
			if (do_lock8) mutexes_[size_++] = &mutex8;
			if (do_lock9) mutexes_[size_++] = &mutex9;
			if (do_lock10) mutexes_[size_++] = &mutex10;
			this->sort_lock();
		}


		~SyncMultiLockUniType()
		{
			for (std::size_t i = size_; i > 0; --i) {  // unlock in reverse order
				LockPolicy::unlock(*(mutexes_[i-1]));
			}
			delete[] mutexes_;
		}


		void sort_lock()
		{
			shell_sort(mutexes_, mutexes_ + size_);
			for (std::size_t i = 0; i < size_; ++i) {
				LockPolicy::lock(*(mutexes_[i]));
			}
		}


	private:
		Mutex** mutexes_;  // array of Mutex*
		std::size_t size_;

		// resolve ambiguity with earlier gcc versions
#if defined(__INTEL_COMPILER) || (!defined __GNUC__) || HZ_GCC_CHECK_VERSION(4, 2, 0)
		SyncMultiLockUniType(const SyncMultiLockUniType& from);
#endif
		SyncMultiLockUniType& operator= (const SyncMultiLockUniType& from);
};





// This one does nothing. Useful for None policy.

template<class Mutex>
class SyncMultiLockEmpty {
	public:

		// -------------------------------- passed as an array

		template<std::size_t size>
		SyncMultiLockEmpty(Mutex* (&mutexes)[size], bool do_lock = true)
		{ }

		// constructors for various containers
		template<template<class ElemType> class Container>
		SyncMultiLockEmpty(const Container<Mutex*>& mutexes, bool do_lock = true)
		{ }

		// STL containers mainly fall into this one
		template<class CT2, template<class ElemType, class> class Container>
		SyncMultiLockEmpty(const Container<Mutex*, CT2>& mutexes, bool do_lock = true)
		{ }

		template<class CT2, class CT3, template<class ElemType, class, class> class Container>
		SyncMultiLockEmpty(const Container<Mutex*, CT2, CT3>& mutexes, bool do_lock = true)
		{ }


		// -------------------------------- passed directly

		SyncMultiLockEmpty(Mutex& mutex1,
				bool do_lock1 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2,
				bool do_lock1 = true, bool do_lock2 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7, Mutex& mutex8,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7, Mutex& mutex8, Mutex& mutex9,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true, bool do_lock9 = true)
		{ }

		SyncMultiLockEmpty(Mutex& mutex1, Mutex& mutex2, Mutex& mutex3, Mutex& mutex4, Mutex& mutex5,
				Mutex& mutex6, Mutex& mutex7, Mutex& mutex8, Mutex& mutex9, Mutex& mutex10,
				bool do_lock1 = true, bool do_lock2 = true, bool do_lock3 = true, bool do_lock4 = true, bool do_lock5 = true,
				bool do_lock6 = true, bool do_lock7 = true, bool do_lock8 = true, bool do_lock9 = true, bool do_lock10 = true)
		{ }

	private:
		SyncMultiLockEmpty(const SyncMultiLockEmpty& from);
		SyncMultiLockEmpty& operator= (const SyncMultiLockEmpty& from);
};






}  // ns





#endif
