/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
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
#include "hz/string_algo.h"

#include <vector>
#include <iostream>

#include "hz/main_tools.h"



/// Main function for the test
int main()
{
	return hz::main_exception_wrapper([]()
	{
		using namespace hz;

		{
			std::string s = "/aa/bbb/ccccc//dsada//";

			std::vector<std::string> v;

			string_split(s, '/', v, false);

			for (const auto& str : v) {
				std::cerr << (str.empty() ? "[empty]" : str) << "\n";
			}
		}


		{
			std::string s = "//aa////bbb/ccccc//dsada////";

			std::vector<std::string> v;

			string_split(s, "//", v, false);

			for (const auto& str : v) {
				std::cerr << (str.empty() ? "[empty]" : str) << "\n";
			}
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

			from.emplace_back("12");
			from.emplace_back("abc");

			to.emplace_back("345");
			to.emplace_back("de");

			std::string s = "12345678abcdefg abc ab";
			std::cerr << s << "\n";
			string_replace_array(s, from, to);
			std::cerr << s << "\n";
		}

		{
			std::vector<std::string> from, to;

			from.emplace_back("12");
			from.emplace_back("abc");

			std::string s = "12345678abcdefg abc ab";
			std::cerr << s << "\n";
			string_replace_array(s, from, ":");
			std::cerr << s << "\n";
		}

		return EXIT_SUCCESS;
	});
}






/// @}
