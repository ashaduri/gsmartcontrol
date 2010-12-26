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
#undef HZ_TLS_DEFAULT_POLICY_GLIB
#undef HZ_TLS_DEFAULT_POLICY_BOOST
#undef HZ_TLS_DEFAULT_POLICY_PTHREAD
#undef HZ_TLS_DEFAULT_POLICY_WIN32

// #define HZ_TLS_DEFAULT_POLICY_GLIB
// #define HZ_TLS_DEFAULT_POLICY_BOOST
#define HZ_TLS_DEFAULT_POLICY_PTHREAD
// #define HZ_TLS_DEFAULT_POLICY_WIN32
*/

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "tls.h"

#include <iostream>




// For those compilers who don't support mixing C- and C++-style
// linkage functions in function pointers, you should use a special
// thread_local_c_ptr<> variant.

// C++-style, for thread_local_ptr<>
inline void custom_cleanup(int* p)
{
	std::cerr << "Calling custom_cleanup with p=" << p << "\n";
	delete p;
}

// C-style, for thread_local_c_ptr<>
inline void custom_cleanup_c(void* p)
{
	std::cerr << "Calling custom_cleanup_c with p=" << p << "\n";
	free(p);
}




int main()
{
	using namespace hz;

// 	g_thread_init(NULL);


	{
		thread_local_ptr<int, TlsPolicyMtDefault> p;

		p.reset(new int(5));
		std::cerr << *p << "\n";

		p.reset(new int(6));
		std::cerr << *p << "\n";
	}


	{
		thread_local_ptr<int, TlsPolicyMtDefault, custom_cleanup> p;
// 		static thread_local_ptr<int, TlsPolicyMtDefault, custom_cleanup> p;

		// void* pointers
// 		thread_local_ptr<void, TlsPolicyMtDefault, tls_cleanup_c_free> p;  // g++ yes, intel yes, sun no.
// 		thread_local_c_ptr<void, TlsPolicyMtDefault, tls_cleanup_c_free> p;  // all yes.

		p.reset(new int(7));
		// should be NO cleanup here
// 		std::cerr << *p << "\n";

		p.reset();
		// should call cleanup here
		std::cerr << p.get() << "\n";
		// should call cleanup here if NOT static
	}

	std::cerr << "All OK\n";

	// should call cleanup here if _static_

	return 0;
}





