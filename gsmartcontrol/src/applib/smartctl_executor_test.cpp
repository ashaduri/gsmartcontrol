/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_tests
/// \weakgroup applib_tests
/// @{

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // NOT NEEDED
#undef throw

#include <iostream>
#include <gtkmm.h>

// #include "smartctl_executor_gui.h"
#include "smartctl_executor.h"



/// Main function of the test
int main(int argc, char** argv)
{
	Gtk::Main m(argc, argv);

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

	return 0;
}




/// @}
