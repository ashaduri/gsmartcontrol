/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "fs_dir.h"

#include <iostream>
#include <vector>



int main()
{
	using namespace hz;

	{
		Dir dir(".");  // open the directory. closed in destructor.
		while (dir.entry_next()) {
			if (!dir.bad())  // one-entry failure
				std::cerr << dir.entry_get_name() << "\n";
		}

		if (dir.bad()) {  // general failure
			std::cerr << "Directory error: " << dir.get_error_locale() << "\n";
		} else {
			std::cerr << "All OK.\n";
		}
	}


	// iterator interface
	{
		std::cerr << "\nListing through iterator interface:\n";

		Dir dir("..");  // open the directory. closed in destructor.

		for (Dir::iterator iter = dir.begin(); iter != dir.end(); ++iter) {
			if (!dir.bad())  // one-entry failure
				std::cerr << *iter << "\n";
		}

		if (dir.bad()) {  // general failure
			std::cerr << "Directory error: " << dir.get_error_locale() << "\n";
		} else {
			std::cerr << "All OK.\n";
		}
	}


	// listing entries
	{
		std::cerr << "\nListing through list():\n";

		hz::Dir dir("..");  // open the directory. closed in destructor.

		{
			std::cerr << "\nSorted by name (dirs first):\n";
			std::vector<std::string> v;
			dir.list(v, false);

			for (unsigned int i = 0; i < v.size(); ++i) {
				std::cerr << v[i] << "\n";
			}
		}

		{
			std::cerr << "\nSorted by timestamp (mixed):\n";
			std::vector<std::string> v;
			dir.list(v, true, hz::DirSortMTime(DIR_SORT_MIXED));

			for (unsigned int i = 0; i < v.size(); ++i) {
				std::cerr << v[i] << "\n";
			}
		}

		{
			std::cerr << "\nSorted by name (dirs first), filtered by wildcard:\n";
			std::vector<std::string> v;
			dir.list_filtered(v, false, hz::DirFilterWc("*.o"));

			for (unsigned int i = 0; i < v.size(); ++i) {
				std::cerr << v[i] << "\n";
			}
		}
	}


	// error handling
	{
		hz::Dir dir("/nonexistent/directory");

		if (!dir.open()) {
			std::cerr << dir.get_error_locale() << "\n";
		} else {
			std::cerr << "Directory \"" << dir.get_path() << "\" opened successfully.\n";
		}
	}


	return 0;
}




