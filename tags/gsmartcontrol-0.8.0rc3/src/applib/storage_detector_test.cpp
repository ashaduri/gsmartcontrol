/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: Public Domain
***************************************************************************/

#include <iostream>

#include "storage_device.h"
#include "storage_detector.h"



int main()
{
	std::vector<StorageDeviceRefPtr> drives;
// 	std::vector<std::string> match_patterns;
	std::vector<std::string> blacklist_patterns;  // additional parameters

	StorageDetector sd;
// 	sd.add_match_patterns(match_patterns);
	sd.add_blacklist_patterns(blacklist_patterns);

	std::string error_msg = sd.detect_and_fetch_basic_data(drives);
	if (!error_msg.empty()) {
		std::cerr << error_msg << "\n";

	} else {
		for (unsigned int i = 0; i < drives.size(); ++i) {
			std::cerr << drives[i]->get_device() << " (" << StorageDevice::get_type_name(drives[i]->get_type()) << ")\n";
		}
	}

	return 0;
}



