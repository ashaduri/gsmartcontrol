/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

#include <iostream>
#include <cstdlib>

#include "hz/debug.h"
#include "hz/fs_file.h"
#include "storage_property.h"
#include "smartctl_parser.h"



int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <file_to_parse>\n";
		return EXIT_FAILURE;
	}

	std::string file_str = argv[1];
	hz::File file(file_str);

	std::string contents;
	if (!file.get_contents(contents)) {
		debug_out_error("app", file.get_error_locale() << "\n");
		return EXIT_FAILURE;
	}


	SmartctlParser sp;

	if (!sp.parse_full(contents)) {
		debug_out_error("app", "Cannot parse file contents: " << sp.get_error_msg() << "\n");
		return EXIT_FAILURE;
	}


	const SmartctlParser::prop_list_t& props = sp.get_properties();

	for(unsigned int i = 0; i < props.size(); ++i) {
		debug_out_dump("app", props[i] << "\n");
	}


	return EXIT_SUCCESS;
}



/*
#include "app_pcrecpp.h"

int main()
{
	pcrecpp::RE re("^ab\\/c?de$");

	std::cerr << re.PartialMatch("ab/de") << "\n";


	std::cerr << app_pcre_match("^abcd", "abcde") << "\n";
	std::cerr << app_pcre_match("/^abcd/", "abcde") << "\n";
	std::cerr << app_pcre_match("/^abcd/i", "Abcde") << "\n";
	std::cerr << app_pcre_match("/^ab.*de$/mis", "abc\\nDe") << "\n";

}
*/




