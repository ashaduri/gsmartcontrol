/******************************************************************************
License: BSD Zero Clause License
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_examples
/// \weakgroup applib_examples
/// @{

// #include "applib/smartctl_executor_gui.h"
#include "applib/smartctl_executor.h"

#include "local_glibmm.h"
#include <gtkmm.h>
#include <iostream>

#include "hz/main_tools.h"



/// Main function of the test
int main(int argc, char** argv)
{
	return hz::main_exception_wrapper([&argc, &argv]()
	{
		// Required by executor
		auto app = Gtk::Application::create("org.gsmartcontrol.examples.smartctl_executor");

		// NOTE: Don't use long options (e.g. --info). Use short ones (e.g. -i),
		// because long options may be unsupported on some platforms.

	//  SmartctlExecutor ex("smartctl", "-i /dev/sda");
	// 	SmartctlExecutorGui ex("ls", "-l --color=no -R /dev");
	// 	SmartctlExecutorGui ex("lsa", "-1 --color=no /sys/block");
	// 	SmartctlExecutorGui ex("../../../0test_binary.sh", "");
		SmartctlExecutor ex("../../../0test_binary.sh", "");

		ex.execute();

		std::string out_str = ex.get_stdout_str();
	// 	std::cout << "OUT:\n" << out_str << "\n\n";
		std::cerr << "OUT SIZE: " << out_str.size() << "\n";

		std::cerr << "STDERR:\n" << ex.get_stderr_str() << "\n";

		std::cerr << "ERROR MSG:\n";
		std::cerr << ex.get_error_msg();

		// execute it second time
		ex.execute();

	// 	m.run();
		return EXIT_SUCCESS;
	});
}




/// @}
