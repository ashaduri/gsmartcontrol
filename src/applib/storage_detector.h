/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_DETECTOR_H
#define STORAGE_DETECTOR_H

#include <vector>
#include <string>

#include "storage_device.h"
#include "command_executor.h"
#include "command_executor_factory.h"



/// Storage detector - detects available drives in the system.
class StorageDetector {
	public:

		/// Detects a list of drives. Returns detection error message if error occurs.
		std::string detect(std::vector<StorageDevicePtr>& drives,
				const ExecutorFactoryPtr& ex_factory);


		/// For each drive, fetch basic data and parse it.
		/// If \c return_first_error is true, the function returns on the first error.
		/// \return An empty string. Or, if return_first_error is true, the first error that occurs.
		std::string fetch_basic_data(std::vector<StorageDevicePtr>& drives,
				const ExecutorFactoryPtr& ex_factory, bool return_first_error = false);


		/// Run detect() and fetch_basic_data().
		/// \return An error if such occurs.
		std::string detect_and_fetch_basic_data(std::vector<StorageDevicePtr>& put_drives_here,
				const ExecutorFactoryPtr& ex_factory);


// 		void add_match_patterns(std::vector<std::string>& patterns)
// 		{
// 			match_patterns_.insert(match_patterns_.end(), patterns.begin(), patterns.end());
// 		}


		/// Add device patterns to drive detection blacklist
		void add_blacklist_patterns(const std::vector<std::string>& patterns)
		{
			blacklist_patterns_.insert(blacklist_patterns_.end(), patterns.begin(), patterns.end());
		}


		/// Get all errors produced by fetch_basic_data().
		[[nodiscard]] const std::vector<std::string>& get_fetch_data_errors() const
		{
			return fetch_data_errors_;
		}


		/// Get command output for each error in get_fetch_data_errors().
		[[nodiscard]] const std::vector<std::string>& get_fetch_data_error_outputs() const
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
