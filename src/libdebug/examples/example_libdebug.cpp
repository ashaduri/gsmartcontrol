/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup libdebug_examples
/// \weakgroup libdebug_examples
/// @{

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "libdebug/libdebug.h"

#include <iostream>


/// libdebug namespace for tests
namespace libdebug_example {

	/// Libdebug test class
	struct TestClassA {
		bool func([[maybe_unused]] int a = 2)
		{
			// this prints "func"
			debug_out_info("default", DBG_FUNC_NAME << "\n");

			// this prints "bool<unnamed>::TestClassA::func()" if in anonymous namespace,
			// else "bool test::TestClass::func()".
			debug_out_info("default", DBG_FUNC_PRNAME << "\n");

			// prints "test::TestClassA::func(): function called."
			debug_out_info("default", DBG_FUNC_MSG << "function called.\n");

			return true;
		}
	};

}


namespace {

	/// Libdebug test struct
	template<typename U>
	struct TestClassB {
		template<typename V>
		U func2([[maybe_unused]] V v, [[maybe_unused]] int i)
		{
			debug_out_info("default", DBG_FUNC_PRNAME << "\n");
			debug_out_info("default", DBG_FUNC_MSG << "function called.\n");
			return 0;
		}
	};

	/// Libdebug test struct
	template<typename T>
	struct TestClassC { };

}



/// Main function for the test
int main()
{
	debug_register_domain("dom");

	debug_set_enabled("dom", debug_level::dump, false);
	debug_set_format("dom", debug_level::info,
			(debug_get_formats("dom")[debug_level::info]
			.reset(debug_format::color)
			.set(debug_format::datetime)
	));


	std::string something = "some thing";
	const char* obj = "obj";
	int op = 5;


	debug_print_dump("dom", "Dumping something: %s\n", something.c_str());
	debug_print_info("dom", "Doing something: %s\n", something.c_str());
	debug_print_error("dom", "Error while doing something\n");

	debug_out_info("dom", "Doing something with " << obj << " object\n");
	debug_out_fatal("dom", "Fatal error while performing operation " << op << "\n");


	DBG_ASSERT_MSG(1 == 0, "One does not equal 0");
	DBG_ASSERT(1 == 0);


//	debug::out(debug::dump) << debug::libdebug_info;
// or
//	printd(debug::dump, "%s", debug::libdebug_info_str().c_str());

// 	debug::out() << "info1\n";  // info level, default domain
//	or
//	debug::print("info1\n");

// 	debug::out(debug::error) << "error1\n";
// or
//	debug::out_error() << "error1\n";
// or
//	debug::print(debug::error, "error1\n");
// or
//	debug::print_error("error1\n");


// 	debug::out(debug::warn, "dom") << debug::indent << "\nwarning1\nwarning2" << debug::unindent << "\n";
// or
//	debug::indent++;
//	debug::printd_warn("dom", "\nwarning1\nwarning2");
//	debug::indent--;

// reseting indentation is done by
//	debug::out("dom", debug::prnone) << debug::indent(0);
// or
//	indent_reset();

// 	debug::out("dom", debug::prnone) << "info2\n";
// or
//	debug::out(debug::info, "dom") << "info2\n";
// or
//	debug::printd("dom", "info2\n");


// 	debug::out(debug::error, "dom") << "error2\n";
// or
//	debug::printd_error("dom", "error2\n");

// 	debug::out(debug::dump, "default", debug::prnone) << "dump1, no prefixes here\n";

	// print out current function name
// 	debug::out(debug::dump) << "dump2, " << DBG_POS(debug::posfunc) << "\n";
// or
//	debug::print_dump("%s", DBG_POS(debug::posfunc).get_text().c_str());


// 	debug::out(debug::dump, "default", debug::prdate) << "date prefix here\n";


	debug_out_dump("default", DBG_POS << "\n");
	debug_out_dump("default", DBG_POS.func << "\n");
// or
	debug_print_info("default", "%s\n", DBG_POS.str().c_str());


	DBG_TRACE_POINT_MSG(1);
	DBG_TRACE_POINT_MSG(666 a);

	DBG_TRACE_POINT_AUTO;
	DBG_TRACE_POINT_AUTO;

// 	Str s;
// 	s.somefunc(1, 1);



	// these begin()/end() turn off prefix printing
	debug_begin();  // don't use different levels inside, or they might get merged (the order will be different).
		debug_out_info("default", "The following lines should have no prefixes\n");
		debug_out_info("default", "1st line\n" << "2nd line\n");
		debug_out_error("default", "3rd line, error, prefixed\n");
		debug_out_info("default", debug_indent << "4th line, not prefixed\n");
		debug_out_warn("default", "5th line, warning, prefixed\n");
		debug_out_warn("default", "6th line, warning, not prefixed\n");
		debug_indent_dec();  // or use << debug_unindent
	debug_end();

	debug_out_info("default", "prefixed\n");



	std::ostream& os = debug_out_dump("default", "");  // get the ostream
	os << "";


	libdebug_example::TestClassA().func();

	TestClassB<unsigned int>().func2(TestClassC<char*>(), 0);

// 	debug_out_warn("default", DBG_FUNC_MSG << "Doing something.\n");
// 	debug_out_warn("default", DBG_FUNC_MSG << "Doing something.\n");


	return EXIT_SUCCESS;

}






/// @}
