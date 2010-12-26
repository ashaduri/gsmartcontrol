/**************************************************************************
 Copyright:
      (C) 2008 - 2010  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include "bin2ascii_encoder.h"

#include <iostream>



int main()
{

	std::string s = "adbeq 2dd +-23\nqqq#4 $";


	hz::Bin2AsciiEncoder enc;

	std::string e = enc.encode(s);
	std::string d = enc.decode(e);

	std::cerr << "Original: \"" << s << "\"\n";
	std::cerr << "Encoded:  \"" << e << "\"\n";
	std::cerr << "Decoded:  \"" << d << "\"\n";




	return 0;
}





