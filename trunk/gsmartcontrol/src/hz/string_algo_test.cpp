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

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "string_algo.h"

#include <vector>
#include <iostream>


int main()
{
	using namespace hz;


	{
		std::string s = "/aa/bbb/ccccc//dsada//";

		std::vector<std::string> v;

		string_split(s, '/', v, false);

		for (unsigned int i = 0; i < v.size(); i++)
			std::cerr << (v[i] == "" ? "[empty]" : v[i]) << "\n";
	}


	{
		std::string s = "//aa////bbb/ccccc//dsada////";

		std::vector<std::string> v;

		string_split(s, "//", v, false);

		for (unsigned int i = 0; i < v.size(); i++)
			std::cerr << (v[i] == "" ? "[empty]" : v[i]) << "\n";
	}


	{
		std::string s = "  a b bb  c     d   ";
		std::cerr << string_remove_adjacent_duplicates_copy(s, ' ') << "\n";
		std::cerr << string_remove_adjacent_duplicates_copy(s, ' ', 2) << "\n";
	}



	{
		std::string s = "/a/b/c/dd//e/";
		string_replace(s, '/', ':');
		std::cerr << s << "\n";
	}


	{
		std::string s = "112/2123412";
		string_replace(s, "12", "AB");
		std::cerr << s << "\n";
	}


	{
		std::vector<std::string> from, to;

		from.push_back("12");
		from.push_back("abc");

		to.push_back("345");
		to.push_back("de");

		std::string s = "12345678abcdefg abc ab";
		std::cerr << s << "\n";
		string_replace_array(s, from, to);
		std::cerr << s << "\n";
	}

	{
		std::vector<std::string> from, to;

		from.push_back("12");
		from.push_back("abc");

		std::string s = "12345678abcdefg abc ab";
		std::cerr << s << "\n";
		string_replace_array(s, from, ":");
		std::cerr << s << "\n";
	}


	return 0;
}





