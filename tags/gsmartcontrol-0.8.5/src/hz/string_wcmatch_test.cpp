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
#include "string_wcmatch.h"

#include <iostream>
#include <ios>



int main()
{
	std::cerr << std::boolalpha << hz::string_wcmatch("*aa", "21345") << "\n";  // false
	std::cerr << std::boolalpha << hz::string_wcmatch("*aa", "21345aaa") << "\n";  // true
	std::cerr << std::boolalpha << hz::string_wcmatch("2??45", "21345") << "\n";  // true
	std::cerr << std::boolalpha << hz::string_wcmatch("*a*", "abcd") << "\n";  // true
	std::cerr << std::boolalpha << hz::string_wcmatch("[123]45", "245") << "\n";  // true
	std::cerr << std::boolalpha << hz::string_wcmatch("\\*aa", "aaa") << "\n";  // false

	return 0;
}




