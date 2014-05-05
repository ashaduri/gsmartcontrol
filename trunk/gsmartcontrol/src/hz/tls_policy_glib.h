/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_TLS_POLICY_GLIB_H
#define HZ_TLS_POLICY_GLIB_H

#include "hz_config.h"  // feature macros

#include <glib.h>


/**
\file
Glib-based thread-local storage.
*/


namespace hz {


extern "C" {
	typedef void (*tls_policy_glib_cleanup_func_t)(void*);
}

// suncc needs this
#if defined HAVE_CXX_EXTERN_C_OVERLOAD && HAVE_CXX_EXTERN_C_OVERLOAD
	typedef void (*tls_policy_glib_cleanup_func_cpp_t)(void*);
#endif



#if GLIB_CHECK_VERSION(2, 32, 0)


/// Glib-based TLS policy
class TlsPolicyGlib {
	public:

		TlsPolicyGlib(tls_policy_glib_cleanup_func_t native_cleanup) : native_cleanup_(native_cleanup)
		{
			key_ = G_PRIVATE_INIT(native_cleanup_);
		}

#if defined HAVE_CXX_EXTERN_C_OVERLOAD && HAVE_CXX_EXTERN_C_OVERLOAD
		TlsPolicyGlib(tls_policy_glib_cleanup_func_cpp_t native_cleanup) : native_cleanup_(native_cleanup)
		{
			key_ = G_PRIVATE_INIT(native_cleanup_);
		}
#endif

		~TlsPolicyGlib()
		{
			// Nothing
		}

		void* get() const
		{
			return g_private_get(&key_);
		}

		void reset(void* p)
		{
			// this will call the cleanup function for the previous pointer
			g_private_replace(&key_, static_cast<gpointer>(p));
		}


		static const bool cleanup_supported = true;

	private:

		mutable GPrivate key_;
		tls_policy_glib_cleanup_func_t native_cleanup_;  ///< may be NULL

		TlsPolicyGlib(const TlsPolicyGlib&);
		TlsPolicyGlib& operator= (const TlsPolicyGlib& from);
};



#else  // old glib



/// Glib-based TLS policy
class TlsPolicyGlib {
	public:

		TlsPolicyGlib(tls_policy_glib_cleanup_func_t native_cleanup) : native_cleanup_(native_cleanup)
		{
			g_static_private_init(&key_);
		}

#if defined HAVE_CXX_EXTERN_C_OVERLOAD && HAVE_CXX_EXTERN_C_OVERLOAD
		TlsPolicyGlib(tls_policy_glib_cleanup_func_cpp_t native_cleanup) : native_cleanup_(native_cleanup)
		{
			g_static_private_init(&key_);
		}
#endif

		~TlsPolicyGlib()
		{
			// if this object is not static, then it will die before the thread.
			// g_static_private_free() will call the destructor function if the key was associated
			// with a non-NULL value at least once.
			g_static_private_free(&key_);
		}

		void* get() const
		{
			return g_static_private_get(&key_);
		}

		void reset(void* p)
		{
			// this will call the cleanup function for the previous pointer
			g_static_private_set(&key_, static_cast<gpointer>(p), native_cleanup_);
		}


		static const bool cleanup_supported = true;

	private:

		mutable GStaticPrivate key_;
		tls_policy_glib_cleanup_func_t native_cleanup_;  ///< may be NULL

		TlsPolicyGlib(const TlsPolicyGlib&);
		TlsPolicyGlib& operator= (const TlsPolicyGlib& from);
};



#endif




}  // ns




#endif

/// @}
