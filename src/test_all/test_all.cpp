/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup test_all
/// \weakgroup test_all
/// @{


#include "catch2/catch_session.hpp"
#include "libdebug/libdebug.h"



int main(int argc, char* argv[])
{
	debug_register_domain("gtk");
	debug_register_domain("app");
	debug_register_domain("hz");
	debug_register_domain("rmn");
	debug_register_domain("rconfig");

	int result = Catch::Session().run( argc, argv );
	return result;
}



/// @}
