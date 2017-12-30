/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz_tests
/// \weakgroup hz_tests
/// @{

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1


// If none are defined and there are no undefs below, the default
// policy is used (see global_macros.h).

/*
#undef HZ_SYNC_DEFAULT_POLICY_GLIBMM
#undef HZ_SYNC_DEFAULT_POLICY_GLIB
#undef HZ_SYNC_DEFAULT_POLICY_BOOST
#undef HZ_SYNC_DEFAULT_POLICY_POCO
#undef HZ_SYNC_DEFAULT_POLICY_PTHREAD
#undef HZ_SYNC_DEFAULT_POLICY_WIN32

// #define HZ_SYNC_DEFAULT_POLICY_GLIBMM
// #define HZ_SYNC_DEFAULT_POLICY_GLIB
// #define HZ_SYNC_DEFAULT_POLICY_BOOST
// #define HZ_SYNC_DEFAULT_POLICY_POCO
#define HZ_SYNC_DEFAULT_POLICY_PTHREAD
// #define HZ_SYNC_DEFAULT_POLICY_WIN32
*/

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "sync_multilock.h"

#include "sync.h"  // policies need this
// #include "sync_policy_glib.h"
// #include "sync_policy_glibmm.h"
// #include "sync_policy_pthread.h"

#include <iostream>
#include <vector>



/// Main function for the test
int main()
{
	using namespace hz;


// 	g_thread_init(NULL);


	{
		SyncPolicyMtDefault::Mutex m1, m2, m3, m4;

		{
			SyncMultiLockUniType<SyncPolicyMtDefault::Mutex> lock(m1, m2, m3, m4, true, true, false, true);
		}
		{
			SyncPolicyMtDefault::Mutex* ms[] = {&m1, &m2, &m3, &m4};
			SyncMultiLockUniType<SyncPolicyMtDefault::Mutex> lock(ms);
		}
		{
			std::vector<SyncPolicyMtDefault::Mutex*> ms;
			ms.push_back(&m1);
			ms.push_back(&m2);

			SyncMultiLockUniType<SyncPolicyMtDefault::Mutex> lock(ms);
		}
	}

/*
	{
		SyncPolicyGlib::Mutex m1;
		SyncPolicyGlibmm::RecMutex m2;
// 		SyncPolicyPthread::Mutex m3;
		SyncPolicyGlib::Mutex m3;  // allow compilation of tests on non-pthread platforms for now.

		{
			SyncMultiLock<SyncPolicyGlib::Mutex, SyncPolicyGlibmm::RecMutex, SyncPolicyGlib::Mutex> lock(m1, m2, m3, true, true, false);
// 			SyncMultiLock<SyncPolicyGlib::Mutex, SyncPolicyGlibmm::RecMutex> lock(m1, m2, m3, true, true, false);  // error
		}
	}
*/


	std::cerr << "All OK\n";


	return 0;
}






/// @}
