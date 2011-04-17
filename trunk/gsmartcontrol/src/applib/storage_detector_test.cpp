/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

#include <iostream>

#include "storage_device.h"
#include "storage_detector.h"
#include "gsc_settings.h"  // in src directory


int main()
{
	// These settings contain device search paths, smartctl binary, etc...
	init_default_settings();

	std::vector<StorageDeviceRefPtr> drives;
// 	std::vector<std::string> match_patterns;
	std::vector<std::string> blacklist_patterns;  // additional parameters

	StorageDetector sd;
// 	sd.add_match_patterns(match_patterns);
	sd.add_blacklist_patterns(blacklist_patterns);

	ExecutorFactoryRefPtr ex_factory(new ExecutorFactory(false));
	std::string error_msg = sd.detect_and_fetch_basic_data(drives, ex_factory);
	if (!error_msg.empty()) {
		std::cerr << error_msg << "\n";

	} else {
		for (unsigned int i = 0; i < drives.size(); ++i) {
			std::cerr << drives[i]->get_device() <<
					" (" << StorageDevice::get_type_readable_name(drives[i]->get_detected_type()) << ")\n";
		}
	}

	return 0;
}



