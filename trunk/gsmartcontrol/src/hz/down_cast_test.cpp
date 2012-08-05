/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz_tests
/// \weakgroup hz_tests
/// @{

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1



// #define DISABLE_RTTI


// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "down_cast.h"

// #include <iostream>



struct TestBase { };

struct TestChild : public TestBase { };

struct TestPoly : public TestBase
{
	virtual ~TestPoly() { }
};

struct TestPoly2 : public TestPoly
{
	virtual ~TestPoly2() { }
};




/// Main function for the test
int main()
{
// 	std::cerr << "TestBase: " << hz::type_is_polymorphic<TestBase>::value << "\n";
// 	std::cerr << "TestChild: " << hz::type_is_polymorphic<TestChild>::value << "\n";
// 	std::cerr << "TestPoly: " << hz::type_is_polymorphic<TestPoly>::value << "\n";


	{
		TestChild c;
		TestBase* b = &c;

		hz::down_cast<TestChild*>(b);  // should use static_cast (both are non-polymorphic)
// 		static_cast<TestChild*>(b);
	}

	{
		TestPoly p;
		TestBase* b = &p;

		hz::down_cast<TestPoly*>(b);  // should use static_cast (base is non-polymorphic)
// 		static_cast<TestChild*>(b);
	}

	{
		TestPoly2 p;
		TestPoly* b = &p;

		hz::down_cast<TestPoly2*>(b);  // should use dynamic_cast (both are polymorphic)
// 		dynamic_cast<TestPoly2*>(b);
// 		static_cast<TestPoly2*>(b);
	}

/*
	{
		TestPoly p;
		TestBase& b = p;

		hz::down_cast<TestPoly&>(b);  // should throw an error, only pointers are supported.
	}
*/

	return 0;
}







/// @}
