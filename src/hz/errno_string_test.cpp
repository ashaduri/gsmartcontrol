/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: Public Domain
***************************************************************************/

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
#include <errno.h>  // E*


#define DECLARE_EVALUE(a) \
	values[a] = #a


int main()
{
	std::map<int, std::string> values;

	DECLARE_EVALUE(EACCES);
	DECLARE_EVALUE(EAGAIN);
	DECLARE_EVALUE(EBUSY);
	DECLARE_EVALUE(ENOENT);
	DECLARE_EVALUE(EEXIST);
// 	DECLARE_EVALUE(ELOOP);  // not on win32

	for(std::map<int, std::string>::const_iterator iter = values.begin(); iter != values.end(); ++iter) {
		std::cout << iter->second << ": " << hz::errno_string(iter->first) << "\n";
	}

	return 0;
}




