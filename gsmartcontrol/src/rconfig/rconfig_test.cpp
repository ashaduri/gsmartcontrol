/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "rconfig.h"
// #include "rconfig_mini.h"

#include <iostream>

#include "hz/hz_config.h"  // ENABLE_GLIB

#if defined ENABLE_GLIB && ENABLE_GLIB
	#include <glib.h>
#endif



int main()
{
	rconfig::load_from_file("test.config");


	// populate /default:
	rconfig::set_default_data("app/use_stuff", true);
	rconfig::set_default_data("app/some_string1", std::string("some_string1_data"));
	rconfig::set_default_data("app/some_string2", "some_string2_data");  // this will store it as std::string!
	rconfig::set_default_data("app/int_var", uint32_t(5));
	rconfig::set_default_data("app/int_var2", int64_t(10));
	rconfig::set_default_data("app/huh", 6.7f);  // float


	rconfig::set_data("app/int_var2", int64_t(11));  // override default.
// 	rconfig::set_data("app/int_var2", uint16_t(12));  // override default. this will error out because it has different type

	rconfig::set_data("/this/is/absolute", 2);  // This will go to root ("/"), not /config or /default.



	// strict typing:

	int64_t int_var = 0;
	rconfig::get_data("app/int_var", int_var);  // this should leave it as 0, because of different types.
	std::cerr << "app/int_var: " << int_var << "\n";

	// we inserted const char*, but it was converted to std::string for storage, so this will work.
	std::cerr << "app/some_string2: " << rconfig::get_data<std::string>("app/some_string2") << "\n";



	// loose typing:

	int int_var2 = 0;
	rconfig::convert_data("app/int_var2", int_var2);  // this will do the type conversion
	std::cerr << "app/int_var2: " << int_var2 << "\n";  // this will get out 11, not 10 (default)


	std::cerr << "app/some_string1: " << rconfig::convert_data<std::string>("app/some_string1") << "\n";


	std::string huh;
	rconfig::convert_data("app/huh", huh);  // float -> string conversion, should work.
	std::cerr << "app/huh: " << huh << "\n";


	std::cerr << "\"app/empty\" exists: " << std::boolalpha << rconfig::data_is_empty("app/empty") << "\n";


	rconfig::dump_tree();

	rconfig::save_to_file("test.config");


#if defined ENABLE_GLIB && ENABLE_GLIB
	rconfig::autosave_set_config_file("test2.config");
	rconfig::autosave_start(2);  // every 2 seconds
	while(true) {
		// without this the timeout function won't be called.
		g_main_context_iteration(NULL, false);
	}
#endif


	return 0;
}









