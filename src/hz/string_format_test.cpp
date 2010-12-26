/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_whatever.txt
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "string_format.h"

#include <iostream>



struct A { };

inline std::ostream& operator<< (std::ostream& os, const A& a)
{
	return (os << "<A>");
}



int main()
{

	{
		std::string s;
		hz::string_format(s, "abc %s efg %d ijk %Lf lmn %s\n")("hello")(5)(8.99)(std::string("werld"));
		std::cerr << s;
	}


	{
		std::string s;
		hz::string_format(s, "A: %s.\n")(A());
		std::cerr << s;
	}


	return 0;
}








