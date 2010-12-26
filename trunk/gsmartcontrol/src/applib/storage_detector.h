/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_DETECTOR_H
#define STORAGE_DETECTOR_H

#include <vector>
#include <string>

#include "hz/intrusive_ptr.h"

#include "storage_device.h"
#include "cmdex_sync.h"



class StorageDetector {

	public:

		StorageDetector()
		{
			match_patterns_.push_back("/.+/");  // accept anything, the scanner picks up disks only anyway.
		}


		// detects a list of drives. returns detection error if error occurs.
		std::string detect(std::vector<StorageDeviceRefPtr>& put_drives_here);


		// fetch basic data of "drives" elements
		std::string fetch_basic_data(std::vector<StorageDeviceRefPtr>& drives,
				hz::intrusive_ptr<CmdexSync> smartctl_ex = 0, bool return_first_error = false);


		// do both of the above, return detection error.
		std::string detect_and_fetch_basic_data(std::vector<StorageDeviceRefPtr>& put_drives_here,
				hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


		void add_match_patterns(std::vector<std::string>& patterns)
		{
			match_patterns_.insert(match_patterns_.end(), patterns.begin(), patterns.end());
		}


		void add_blacklist_patterns(std::vector<std::string>& patterns)
		{
			blacklist_patterns_.insert(blacklist_patterns_.end(), patterns.begin(), patterns.end());
		}


		const std::vector<std::string>& get_fetch_data_errors() const
		{
			return fetch_data_errors_;
		}

		const std::vector<std::string>& get_fetch_data_error_outputs() const
		{
			return fetch_data_error_outputs_;
		}


	private:

		std::vector<std::string> match_patterns_;  // first each file is matched against these
		std::vector<std::string> blacklist_patterns_;  // and then these

		std::vector<std::string> fetch_data_errors_;  // errors not returned via functions
		std::vector<std::string> fetch_data_error_outputs_;  // corresponding outputs

};






#endif
