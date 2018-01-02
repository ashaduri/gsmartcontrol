/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz_examples
/// \weakgroup hz_examples
/// @{

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "errno_string.h"

#include <iostream>
#include <map>
#include <cerrno>  // for errno.h
#include <errno.h>  // E*


/// Helper macro for the test - declares E* value (e.g. EACCES).
#define TEST_DECLARE_EVALUE(a) \
	values[a] = #a



/// Main function for the test
int main()
{
	std::map<int, std::string> values;

	TEST_DECLARE_EVALUE(EACCES);
	TEST_DECLARE_EVALUE(EAGAIN);
	TEST_DECLARE_EVALUE(EBUSY);
	TEST_DECLARE_EVALUE(ENOENT);
	TEST_DECLARE_EVALUE(EEXIST);
// 	TEST_DECLARE_EVALUE(ELOOP);  // not on win32

	for(std::map<int, std::string>::const_iterator iter = values.begin(); iter != values.end(); ++iter) {
		std::cout << iter->second << ": " << hz::errno_string(iter->first) << "\n";
	}

	return 0;
}





/// @}
