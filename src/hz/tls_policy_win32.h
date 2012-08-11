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

#ifndef HZ_TLS_POLICY_WIN32_H
#define HZ_TLS_POLICY_WIN32_H

#include "hz_config.h"  // feature macros

#ifndef _WIN32
	#error Cannot compile a win32-only file under non-win32 system
#endif

#include <windows.h>

// Don't use DBG_ASSERT() here, it's using this library and therefore
// too error-prone to use in this context.
#include <cassert>
#ifndef ASSERT  // assert() is undefined if NDEBUG is defined.
	#define ASSERT(a) assert(a)
#endif


/**
\file
Win32-threads-based thread-local storage.

NOTE On-thread-exit cleanup functions ARE NOT implemented.
They are called on object destruction and reset(), however.
Implementing on-thread-exit callbacks requires some really ugly
hacks, introducing some really ugly problems and limitations.
See http://lists.boost.org/Archives/boost/2003/02/44905.php
*/


namespace hz {


/// Win32-threads-based TLS policy
class TlsPolicyWin32 {
	public:

		typedef void (*native_cleanup_func_t)(void*);

		TlsPolicyWin32(native_cleanup_func_t native_cleanup)
				: native_cleanup_(native_cleanup), inited_(false)
		{
			key_ = TlsAlloc();  // sets data to 0.
			ASSERT(key_ != TLS_OUT_OF_INDEXES);
		}

		~TlsPolicyWin32()
		{
			// call cleanup manually
			if (inited_ && native_cleanup_)
				native_cleanup_(get());

			if (key_ != TLS_OUT_OF_INDEXES) {
				BOOL res = TlsFree(key_);
				ASSERT(res != 0);
			}
		}

		void* get() const
		{
			void* p = TlsGetValue(key_);
			ASSERT(p || GetLastError() == ERROR_SUCCESS);  // if p is 0, check last error
			return p;
		}

		void reset(void* p)
		{
			// call the cleanup function for the previous pointer.
			if (inited_ && native_cleanup_)
				native_cleanup_(get());

			if (p)
				inited_ = true;

			BOOL res = TlsSetValue(key_, p);
			ASSERT(res != 0);
		}


		static const bool cleanup_supported = false;

	private:

		DWORD key_;
		native_cleanup_func_t native_cleanup_;  ///< may be NULL
		bool inited_;  ///< True if the key has been associated with the non-NULL value at least once.

		TlsPolicyWin32(const TlsPolicyWin32&);
		TlsPolicyWin32& operator= (const TlsPolicyWin32& from);
};





}  // ns




#endif

/// @}
