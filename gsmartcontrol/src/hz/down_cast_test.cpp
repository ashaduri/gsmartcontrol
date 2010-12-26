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



// #define DISABLE_RTTI


// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "down_cast.h"

// #include <iostream>



struct Base { };

struct Child : public Base { };

struct Poly : public Base
{
	virtual ~Poly() { }
};

struct Poly2 : public Poly
{
	virtual ~Poly2() { }
};





int main()
{
// 	std::cerr << "Base: " << hz::type_is_polymorphic<Base>::value << "\n";
// 	std::cerr << "Child: " << hz::type_is_polymorphic<Child>::value << "\n";
// 	std::cerr << "Poly: " << hz::type_is_polymorphic<Poly>::value << "\n";


	{
		Child c;
		Base* b = &c;

		hz::down_cast<Child*>(b);  // should use static_cast (both are non-polymorphic)
// 		static_cast<Child*>(b);
	}

	{
		Poly p;
		Base* b = &p;

		hz::down_cast<Poly*>(b);  // should use static_cast (base is non-polymorphic)
// 		static_cast<Child*>(b);
	}

	{
		Poly2 p;
		Poly* b = &p;

		hz::down_cast<Poly2*>(b);  // should use dynamic_cast (both are polymorphic)
// 		dynamic_cast<Poly2*>(b);
// 		static_cast<Poly2*>(b);
	}

/*
	{
		Poly p;
		Base& b = p;

		hz::down_cast<Poly&>(b);  // should throw an error, only pointers are supported.
	}
*/

	return 0;
}






