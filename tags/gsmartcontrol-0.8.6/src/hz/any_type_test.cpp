/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1


// define this to disable any_convert.h dependency
// #define DISABLE_ANY_CONVERT

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "any_type.h"

#include <iostream>
#include <list>
#include <string>


using namespace hz;


struct A {

	typedef std::list<any_type> any_list;


	void append_int(any_list& values, int value)
	{
		any_type to_append = value;
		values.push_back(to_append);
	}


	void append_string(any_list& values, const std::string& value)
	{
		values.push_back(value);
	}


	void append_char_ptr(any_list& values, const char* value)
	{
		values.push_back(value);
	}


	void append_any(any_list& values, const any_type& value)
	{
		values.push_back(value);
	}


	void append_nothing(any_list& values)
	{
		values.push_back(any_type());
	}


	std::string get_string(any_list& values)
	{
		any_type a = values.back();

		any_cast<std::string>(a);

		std::string b;
		a.get(b);

		a.get<std::string>();

		// is_type<> is available only with RTTI
#if !(defined DISABLE_RTTI && DISABLE_RTTI)
		if (a.is_type<std::string>())
			return any_cast<std::string>(a);
		return std::string();

#else  // can't do any checking here
		return any_cast<std::string>(a);
#endif
	}


};




// enable printing A.
// This needs operator<< and and ANY_TYPE_SET_PRINTABLE specialization.

inline std::ostream& operator<< (std::ostream& os, A a)
{
	return (os << "A\n");
}

ANY_TYPE_SET_PRINTABLE(A, true);




int main()
{

	any_type a1;
	a1 = 4;
	std::cerr << a1.to_stream() << "\n";


	A b;
	any_type b1 = b;
// 	std::cerr << b;
	std::cerr << b1.to_stream() << "\n";



#if !(defined DISABLE_ANY_CONVERT && DISABLE_ANY_CONVERT)

	any_type a2 = std::string("5.444");
	double a2val = 0;
	std::cerr << "conversion " << (a2.convert(a2val) ? "succeeded" : "failed") << ", ";
	std::cerr << "value: " << a2val << "\n";

	any_type a3 = 6.55;
	std::string a3val;
	std::cerr << "conversion " << (a3.convert(a3val) ? "succeeded" : "failed") << ", ";
	std::cerr << "value: " << a3val << "\n";

	any_type a4 = std::string("7");
	int a4val = 0;
	std::cerr << "conversion " << (a4.convert(a4val) ? "succeeded" : "failed") << ", ";
	std::cerr << "value: " << a4val << "\n";

	any_type a5 = 'a';
	double a5val = 0;
	std::cerr << "conversion " << (a5.convert(a5val) ? "succeeded" : "failed") << ", ";
	std::cerr << "value: " << a5val << "\n";


	char b1val = 0;
	std::cerr << "conversion " << (b1.convert(b1val) ? "succeeded" : "failed") << ", ";
	std::cerr << "value: " << int(b1val) << "\n";

#endif


	return 0;
}





