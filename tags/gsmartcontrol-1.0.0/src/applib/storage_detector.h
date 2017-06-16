/**************************************************************************
 Copyright:
      (C) 2008 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_DETECTOR_H
#define STORAGE_DETECTOR_H

#include <vector>
#include <string>

#include "hz/intrusive_ptr.h"

#include "storage_device.h"
#include "cmdex_sync.h"
#include "executor_factory.h"



/// Storage detector - detects available drives in the system.
class StorageDetector {
	public:

		/// Constructor
		StorageDetector()
		{
// 			match_patterns_.push_back("/.+/");  // accept anything, the scanner picks up disks only anyway.
		}


		/// Detects a list of drives. Returns detection error message if error occurs.
		std::string detect(std::vector<StorageDeviceRefPtr>& put_drives_here,
				ExecutorFactoryRefPtr ex_factory);


		/// For each drive, fetch basic data and parse it.
		/// If \c return_first_error is true, the function returns on the first error.
		/// \return An empty string. Or, if return_first_error is true, the first error that occurs.
		std::string fetch_basic_data(std::vector<StorageDeviceRefPtr>& drives,
				ExecutorFactoryRefPtr ex_factory, bool return_first_error = false);


		/// Run detect() and fetch_basic_data().
		/// \return An error if such occurs.
		std::string detect_and_fetch_basic_data(std::vector<StorageDeviceRefPtr>& put_drives_here,
				ExecutorFactoryRefPtr ex_factory);


// 		void add_match_patterns(std::vector<std::string>& patterns)
// 		{
// 			match_patterns_.insert(match_patterns_.end(), patterns.begin(), patterns.end());
// 		}


		/// Add device patterns to drive detection blacklist
		void add_blacklist_patterns(std::vector<std::string>& patterns)
		{
			blacklist_patterns_.insert(blacklist_patterns_.end(), patterns.begin(), patterns.end());
		}


		/// Get all errors produced by fetch_basic_data().
		const std::vector<std::string>& get_fetch_data_errors() const
		{
			return fetch_data_errors_;
		}


		/// Get command output for each error in get_fetch_data_errors().
		const std::vector<std::string>& get_fetch_data_error_outputs() const
		{
			return fetch_data_error_outputs_;
		}


	private:

// 		std::vector<std::string> match_patterns_;  ///< First each file is matched against these
		std::vector<std::string> blacklist_patterns_;  ///< If a device matches these, it's ignored.

		std::vector<std::string> fetch_data_errors_;  ///< Errors that have occurred
		std::vector<std::string> fetch_data_error_outputs_;  ///< Corresponding command outputs to fetch_data_errors_

};






#endif

/// @}
