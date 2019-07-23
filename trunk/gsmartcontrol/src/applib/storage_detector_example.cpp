/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib_examples
/// \weakgroup applib_examples
/// @{

#include <iostream>

#include "storage_device.h"
#include "storage_detector.h"
#include "gsc_settings.h"  // in src directory



/// Main function of the test
int main()
{
	// These settings contain device search paths, smartctl binary, etc...
	init_default_settings();

	std::vector<StorageDevicePtr> drives;
// 	std::vector<std::string> match_patterns;
	std::vector<std::string> blacklist_patterns;  // additional parameters

	StorageDetector sd;
// 	sd.add_match_patterns(match_patterns);
	sd.add_blacklist_patterns(blacklist_patterns);

	auto ex_factory = std::make_shared<ExecutorFactory>(false);
	std::string error_msg = sd.detect_and_fetch_basic_data(drives, ex_factory);
	if (!error_msg.empty()) {
		std::cerr << error_msg << "\n";

	} else {
		for (const auto& drive : drives) {
			std::cerr << drive->get_device_with_type() <<
					" (" << StorageDevice::get_type_storable_name(drive->get_detected_type()) << ")\n";
		}
	}

	return 0;
}




/// @}
