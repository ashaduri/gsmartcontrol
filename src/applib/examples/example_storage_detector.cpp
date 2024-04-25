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

#include <iostream>

#include "applib/storage_device.h"
#include "applib/storage_detector.h"
#include "applib/gsc_settings.h"

#include "hz/main_tools.h"



/// Main function of the test
int main()
{
	return hz::main_exception_wrapper([]()
	{
		// These settings contain device search paths, smartctl binary, etc.
		init_default_settings();

		std::vector<StorageDevicePtr> drives;
	// 	std::vector<std::string> match_patterns;
		std::vector<std::string> blacklist_patterns;  // additional parameters

		StorageDetector sd;
	// 	sd.add_match_patterns(match_patterns);
		sd.add_blacklist_patterns(blacklist_patterns);

		auto ex_factory = std::make_shared<CommandExecutorFactory>(false);
		auto fetch_error = sd.detect_and_fetch_basic_data(drives, ex_factory);
		if (!fetch_error) {
			std::cerr << fetch_error.error().message() << "\n";

		} else {
			for (const auto& drive : drives) {
				std::cerr << drive->get_device_with_type() <<
						" (" << StorageDevice::get_type_storable_name(drive->get_detected_type()) << ")\n";
			}
		}
		return EXIT_SUCCESS;
	});
}




/// @}
