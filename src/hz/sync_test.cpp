/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_whatever.txt
***************************************************************************/

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
// #define HZ_SYNC_DEFAULT_POLICY_PTHREAD
// #define HZ_SYNC_DEFAULT_POLICY_WIN32
*/

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "sync.h"

#include <iostream>



int main()
{
	using namespace hz;


	SyncPolicyMtDefault::init();


	{
		SyncPolicyMtDefault::Mutex m;

		SyncPolicyMtDefault::lock(m);
		SyncPolicyMtDefault::unlock(m);
		SyncPolicyMtDefault::trylock(m);
		SyncPolicyMtDefault::unlock(m);

		SyncPolicyMtDefault::ScopedLock lock(m);
	}

	{
		SyncPolicyMtDefault::RecMutex m;

		SyncPolicyMtDefault::lock(m);
		SyncPolicyMtDefault::trylock(m);
		SyncPolicyMtDefault::unlock(m);
		SyncPolicyMtDefault::unlock(m);

		SyncPolicyMtDefault::ScopedRecLock lock(m);
	}

	{
		SyncPolicyMtDefault::RWMutex m;

		SyncPolicyMtDefault::lock(m);
		SyncPolicyMtDefault::unlock(m);
		SyncPolicyMtDefault::trylock(m);
		SyncPolicyMtDefault::unlock(m);

		SyncPolicyMtDefault::lock(m, true);
		SyncPolicyMtDefault::unlock(m, true);
		SyncPolicyMtDefault::trylock(m, true);
		SyncPolicyMtDefault::unlock(m, true);

		{
			SyncPolicyMtDefault::ScopedRWLock lock(m);
		}
		{
			SyncPolicyMtDefault::ScopedRWLock lock(m, true);
		}
	}


	std::cerr << "All OK\n";


	return 0;
}





