/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

#ifndef HZ_TLS_BOOST_H
#define HZ_TLS_BOOST_H

#include "hz_config.h"  // feature macros

#include <boost/thread/tss.hpp>


// Boost-based TLS.


namespace hz {


// TlsPolicyBoost is declared in tls.h.


// Our class mimics boost::thread_specific_ptr<> in its API,
// so we may just wrap it without any changes.

template<typename T, void cleanup_func(T*)>
class thread_local_ptr<T, TlsPolicyBoost, cleanup_func>
	: public boost::thread_specific_ptr<T> {

	public:
		thread_local_ptr() : boost::thread_specific_ptr<T>(cleanup_func)
		{ }

		static const bool cleanup_supported = true;

};


// boost doesn't support void*, but we still need to specialize to avoid
// ambigous situations.
template<void cleanup_func(void*)>
class thread_local_ptr<void, TlsPolicyBoost, cleanup_func> {

	public:
		thread_local_ptr() { }

		// these should trigger the errors
		void get() { }
		void reset() { }
};




// These two will work if the compiler supports passing C functions
// to C++-linkage template arguments

template<typename T, tls_cleanup_c_func_t cleanup_func>
class thread_local_c_ptr<T, TlsPolicyBoost, cleanup_func>
	: public boost::thread_specific_ptr<T> {

	public:
		thread_local_c_ptr() : boost::thread_specific_ptr<T>(cleanup_func)
		{ }

		static const bool cleanup_supported = true;

};


// boost doesn't support void*, but we still need to specialize to avoid
// ambigous situations.
template<tls_cleanup_c_func_t cleanup_func>
class thread_local_c_ptr<void, TlsPolicyBoost, cleanup_func> {

	public:
		thread_local_c_ptr() { }

		// these should trigger the errors
		void get() { }
		void reset() { }
};





}  // ns




#endif
