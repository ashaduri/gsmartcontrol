/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_TLS_H
#define HZ_TLS_H

#include "hz_config.h"  // feature macros

#include <cstdlib>  // std::free()

// Don't use DBG_ASSERT() here, it's using this library and therefore
// too error-prone to use in this context.
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif



// Thread-local storage



// Notes on various policies:

// Poco-based policy cannot be implemented because Poco's ThreadLocal
// lacks some vital methods, e.g. setting it to an existing pointer (it creates
// the object itself), or specifying a destroy function (it just deletes the pointer).

// Glibmm-based policy has been implemented but it had problems
// with calling the destructor function (not calling it at all), plus it's completely
// redundant with availability of Glib policy.

// Win32-based policy doesn't have on-thread-exit cleanup function support
// due to technical limitations of MS design. Cleanup functions are called
// on object destruction and reset(), however. If this is a major problem,
// use Glib, pthread-win32 or Boost policies instead (they have some
// dll-specific workarounds implemented).
// See win32 policy file for details.



namespace hz {


	// suitable for single-threaded model.
	struct TlsPolicyNone {
		typedef void (*native_cleanup_func_t)(void*);

		TlsPolicyNone(native_cleanup_func_t native_cleanup)
			: native_cleanup_(native_cleanup), inited_(false), p_(0)
		{ }

		~TlsPolicyNone()
		{
			if (inited_ && native_cleanup_)
				native_cleanup_(get());
		}

		void* get() const
		{
			return p_;
		}

		void reset(void* p)
		{
			// call the cleanup function for the previous pointer.
			if (inited_ && native_cleanup_)
				native_cleanup_(p_);
			if (p)
				inited_ = true;
			p_ = p;
		}


		private:
			native_cleanup_func_t native_cleanup_;  // may be NULL
			bool inited_;
			void* p_;

			TlsPolicyNone(const TlsPolicyNone&);
			TlsPolicyNone& operator= (const TlsPolicyNone& from);
	};


}



#if defined HZ_TLS_DEFAULT_POLICY_GLIB
	#include "tls_policy_glib.h"
	namespace hz {
		typedef TlsPolicyGlib TlsPolicyMtDefault;
	}

#elif defined HZ_TLS_DEFAULT_POLICY_BOOST
	// No header here. The header contains thread_local_ptr specialization,
	// therefore it must be after thread_local_ptr (see below).
	namespace hz {
		// Note: TlsPolicyMtDefault should be before thread_local_ptr - it's
		// used as its default template argument.
		struct TlsPolicyBoost { };  // dummy, only to help specialization

		typedef TlsPolicyBoost TlsPolicyMtDefault;
	}

#elif defined HZ_TLS_DEFAULT_POLICY_PTHREAD
	#include "tls_policy_pthread.h"
	namespace hz {
		typedef TlsPolicyPthread TlsPolicyMtDefault;
	}


#elif defined HZ_TLS_DEFAULT_POLICY_WIN32
	#include "tls_policy_win32.h"
	namespace hz {
		typedef TlsPolicyWin32 TlsPolicyMtDefault;
	}

#else  // default: NONE
// #elif defined HZ_TLS_DEFAULT_POLICY_NONE  // default:
	namespace hz {
		typedef TlsPolicyNone TlsPolicyMtDefault;
	}

#endif





namespace hz {



namespace internal {

	// base class, needed to distinguish between actual and void* specialization
	template<typename T, class TlsPolicy>
	class thread_local_ptr_base {

		protected:

			template<typename PolicyArg>
			thread_local_ptr_base(PolicyArg cleanup) : policy_(cleanup)
			{ }

		public:

			~thread_local_ptr_base()
			{
				reset();
			}

			T* get() const
			{
				return static_cast<T*>(policy_.get());
			}

			T* release()
			{
				T* tmp = get();
				reset();
				return tmp;
			}

			void reset(T* p = NULL)
			{
				if (get() == p)
					return;
				// this will call the cleanup function for the previous pointer
				policy_.reset(static_cast<void*>(p));
			}

			// policy capabilities
			bool cleanup_supported()
			{
				return TlsPolicy::cleanup_supported;
			}

		private:

			TlsPolicy policy_;
	};

}  // ns




// -------------------------------- C++-style cleanup function linkage



// Note: These functions have C++ linkage. It is impossible to make
// them extern "C", because they are templates. Moving to void* arguments
// is also invalid, because C++ operator delete cannot reliably delete a
// void* pointer (destructor may not get called, etc...).

// C linkage _may_ be needed if:
// 1. The compiler doesn't support calling C++ functions by pointers from
// 	C code, AND
// 2. The underlying TLS implementation is in C (e.g. GLib, pthread, etc...).
// Note that this may not affect implementations which do implement their
// own callback system (e.g. Boost).

// If you desperately need C linkage, you may define your own C-linked callback.
// However, note that you MUST NOT delete the resulting pointer with delete
// operator if it's void*.
// Note: There may be problems when passing extern "C" function as a template
// parameter to non-extern "C" class template (sun compiler).

template<typename T>
struct tls_functions {
	// this will be called by default if no cleanup function has been set.
	static void cleanup_delete(T* p) { delete p; }

	// use this for delete[]
	static void cleanup_delete_array(T* p) { delete[] p; }

	// use this for C's free()
	static void cleanup_free(T* p) { std::free(p); }

	// use this for no cleanup
	static void nothing(T* p) { }  // Note: This will work with C linkage too, because it's not actually called.
};



// The actual public class.
template<typename T, class TlsPolicy = TlsPolicyMtDefault,
		void cleanup_func(T*) = tls_functions<T>::cleanup_delete>
class thread_local_ptr : public internal::thread_local_ptr_base<T, TlsPolicy> {

		// This class cannot support DefaultType, because it may be specialized by TlsPolicy parameter.
// 		typedef typename type_auto_select<TlsPolicy_, TlsPolicyMtDefault>::type TlsPolicy;  // support DefaultType

	typedef internal::thread_local_ptr_base<T, TlsPolicy> base;
	typedef thread_local_ptr<T, TlsPolicy, cleanup_func> self_type;

	public:
		using base::get;
		using base::release;
		using base::reset;
		using base::cleanup_supported;

		thread_local_ptr()
			: base(cleanup_func == tls_functions<T>::nothing ? NULL : &cleanup_proxy)
		{ }

		T* operator->() const { return get(); }

		T& operator*() const
		{
			T* cur = get();
			ASSERT(cur);
			return *cur;
		}

	private:
		thread_local_ptr(const self_type&);
		thread_local_ptr& operator= (const self_type& from);

		// Internal proxy function. This is called by the underlying implementation.
		// Note: This should have been a C linkage function, because its pointer may
		// be passed to C functions. However, we can't do that because it's a template.
		// So we rely on compiler to support passing C++ function pointers to C functions.
		static void cleanup_proxy(void* p)
		{
			cleanup_func(static_cast<T*>(p));
		}

};



// void* specialization. disallow operator*.
template<class TlsPolicy, void cleanup_func(void*)>
class thread_local_ptr<void, TlsPolicy, cleanup_func>
		: public internal::thread_local_ptr_base<void, TlsPolicy> {

	typedef internal::thread_local_ptr_base<void, TlsPolicy> base;
	typedef thread_local_ptr<void, TlsPolicy, cleanup_func> self_type;

	public:
		using base::get;
		using base::release;
		using base::reset;
		using base::cleanup_supported;

		thread_local_ptr() : base(cleanup_func == tls_functions<void>::nothing ? NULL : cleanup_func)
		{ }

		void* operator->() const { return get(); }

	private:
		thread_local_ptr(const self_type&);
		thread_local_ptr& operator= (const self_type& from);
};






// -------------------------------- C-style cleanup function linkage



// C function for T=void. Don't use C linkage -
extern "C" {

	typedef void (tls_cleanup_c_func_t)(void*);

	inline void tls_cleanup_c_free(void* p)
	{
		std::free(p);
	}

	inline void tls_cleanup_c_nothing(void* p)
	{ }

}




// The actual public class.
template<typename T, class TlsPolicy = TlsPolicyMtDefault,
		tls_cleanup_c_func_t cleanup_func = tls_cleanup_c_free>
class thread_local_c_ptr : public internal::thread_local_ptr_base<T, TlsPolicy> {

	typedef internal::thread_local_ptr_base<T, TlsPolicy> base;
	typedef thread_local_ptr<T, TlsPolicy, cleanup_func> self_type;

	public:
		using base::get;
		using base::release;
		using base::reset;
		using base::cleanup_supported;

		thread_local_c_ptr() : base(cleanup_func == tls_cleanup_c_nothing ? NULL : cleanup_func)
		{ }

		T* operator->() const { return get(); }

		T& operator*() const
		{
			T* cur = get();
			ASSERT(cur);
			return *cur;
		}

	private:
		thread_local_c_ptr(const self_type&);
		thread_local_c_ptr& operator= (const self_type& from);
};



// void* specialization. disallow operator*.
template<class TlsPolicy, tls_cleanup_c_func_t cleanup_func>
class thread_local_c_ptr<void, TlsPolicy, cleanup_func>
		: public internal::thread_local_ptr_base<void, TlsPolicy> {

	typedef internal::thread_local_ptr_base<void, TlsPolicy> base;
	typedef thread_local_ptr<void, TlsPolicy, cleanup_func> self_type;

	public:
		using base::get;
		using base::release;
		using base::reset;
		using base::cleanup_supported;

		thread_local_c_ptr() : base(cleanup_func == tls_cleanup_c_nothing ? NULL : cleanup_func)
		{ }

		void* operator->() const { return get(); }

	private:
		thread_local_c_ptr(const self_type&);
		thread_local_c_ptr& operator= (const self_type& from);
};






}  // ns



// must be after thread_local_ptr
#if defined HZ_TLS_DEFAULT_POLICY_BOOST
	#include "tls_policy_boost.h"
#endif





#endif
