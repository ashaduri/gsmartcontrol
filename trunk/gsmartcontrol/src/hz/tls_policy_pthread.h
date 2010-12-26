/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_TLS_POLICY_PTHREAD_H
#define HZ_TLS_POLICY_PTHREAD_H

#include "hz_config.h"  // feature macros

#include <pthread.h>

// Don't use DBG_ASSERT() here, it's using this library and therefore
// too error-prone to use in this context.
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif


// POSIX Threads-based TLS.
// Note that cleanup-function calling policy emulates the Glib one.


namespace hz {


extern "C" {
	typedef void (*tls_policy_pthread_cleanup_func_t)(void*);
}

// suncc needs this
#if defined HAVE_CXX_EXTERN_C_OVERLOAD && HAVE_CXX_EXTERN_C_OVERLOAD
	typedef void (*tls_policy_pthread_cleanup_cpp_func_t)(void*);
#endif



class TlsPolicyPthread {
	public:

		TlsPolicyPthread(tls_policy_pthread_cleanup_func_t native_cleanup)
				: native_cleanup_(native_cleanup), inited_(false)
		{
			int res = pthread_key_create(&key_, native_cleanup);
			ASSERT(res == 0);
		}

#if defined HAVE_CXX_EXTERN_C_OVERLOAD && HAVE_CXX_EXTERN_C_OVERLOAD
		TlsPolicyPthread(tls_policy_pthread_cleanup_cpp_func_t native_cleanup) : native_cleanup_(native_cleanup)
		{
			int res = pthread_key_create(&key_, native_cleanup);
			ASSERT(res == 0);
		}
#endif

		~TlsPolicyPthread()
		{
			// if this object is not static, then it will die before the thread.
			// pthread_key_delete() won't call the destructor function, so call it manually.
			if (inited_ && native_cleanup_)
				native_cleanup_(get());

			int res = pthread_key_delete(key_);
			ASSERT(res == 0);
		}

		void* get() const
		{
			return pthread_getspecific(key_);
		}

		void reset(void* p)
		{
			// call the cleanup function for the previous pointer.
			if (inited_ && native_cleanup_)
				native_cleanup_(get());

			if (p)
				inited_ = true;

			int res = pthread_setspecific(key_, p);
			ASSERT(res == 0);
		}


		static const bool cleanup_supported = true;

	private:

		pthread_key_t key_;
		tls_policy_pthread_cleanup_func_t native_cleanup_;  // may be NULL
		bool inited_;  // if the key has been associated with the non-NULL value at least once.

		TlsPolicyPthread(const TlsPolicyPthread&);
		TlsPolicyPthread& operator= (const TlsPolicyPthread& from);
};








}  // ns




#endif
