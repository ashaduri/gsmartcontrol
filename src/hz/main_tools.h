/******************************************************************************
License: Zlib
Copyright:
	(C) 2013 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup hz
/// \weakgroup hz
/// @{

#ifndef HZ_MAIN_TOOLS_H
#define HZ_MAIN_TOOLS_H

#include <cstdlib>  // EXIT_*
#include <iostream>  // std::cerr
#include <exception>  // std::exception, std::set_terminate()
#include <string>

#include "system_specific.h"  // type_name_demangle


namespace hz {


/// This function calls `main_impl()` but wraps it in verbose exception handlers.
/// \tparam MainImplFunc function with signature `int main_impl()`, returning exit status.
template <typename MainImplFunc>
int main_exception_wrapper(MainImplFunc main_impl) noexcept
{
	// We don't use __gnu_cxx::__verbose_terminate_handler(), we can do the same in noexcept way
	// with C++ABI library (without clang-tidy warnings about not handling exceptions in main()).
	try {
		return main_impl();
	}
	catch(std::exception& e) {
		// don't use anything other than cerr here, it's the most safe option.
		std::cerr << "main(): Unhandled exception: " << e.what() << std::endl;
		if (const auto* ex_type = get_current_exception_type()) {
			std::cerr << "Type of exception: " << type_name_demangle(ex_type->name()) << std::endl;
		}
	}
	catch(...) {  // this guarantees proper unwinding in case of unhandled exceptions (win32 I think)
		std::cerr << "main(): Unhandled unknown exception." << std::endl;
		if (const auto* ex_type = get_current_exception_type()) {
			std::cerr << "Type of exception: " << type_name_demangle(ex_type->name()) << std::endl;
		}
	}
	return EXIT_FAILURE;
}



}


#endif

/// @}
