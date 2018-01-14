/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef SELFTEST_H
#define SELFTEST_H

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>
#undef throw

#include <string>
#include <cstdint>
#include <chrono>

#include "hz/intrusive_ptr.h"

#include "storage_device.h"
#include "cmdex_sync.h"



/// SMART self-test runner.
class SelfTest : public hz::intrusive_ptr_referenced {
	public:

		/// Test type
		enum test_t {
			type_ioffline,  ///< Immediate offline, not supported
			type_short,  ///< Short self-test
			type_long,  ///< Extended (a.k.a. long) self-test
			type_conveyance  ///< Conveyance self-test
		};


		/// Get displayable name for a test type
		static std::string get_test_name(test_t t)
		{
			switch (t) {
				case type_ioffline: return "Immediate Offline Test";
				case type_short: return "Short Self-test";
				case type_long: return "Extended Self-test";
				case type_conveyance: return "Conveyance Self-test";
			};
			return "[error]";
		}


		/// Constructor. \c drive must have the capabilities present in its properties.
		SelfTest(StorageDeviceRefPtr drive, test_t type)
			: drive_(drive), type_(type)
		{
			clear();
		}


		/// Clear results of previous test
		void clear()
		{
			status_ = StorageSelftestEntry::status_unknown;
			remaining_percent_ = -1;
			last_seen_percent_ = -1;
			total_duration_ = std::chrono::seconds(-1);
			poll_in_seconds_ = std::chrono::seconds(-1);
		}


		/// Check if the test is currently active
		bool is_active() const
		{
			return (status_ == StorageSelftestEntry::status_in_progress);
		}


		/// Get remaining time percent until the test completion.
		/// \return -1 if N/A or unknown.
		int8_t get_remaining_percent() const
		{
			return remaining_percent_;
		}


		/// Get estimated time of completion for the test.
		/// \return -1 if N/A or unknown. Note that 0 is a valid value.
		std::chrono::seconds get_remaining_seconds() const;


		/// Get test type
		test_t get_test_type() const
		{
			return type_;
		}


		/// Get test status
		StorageSelftestEntry::status_t get_status() const
		{
			return status_;
		}


		/// Get the number of seconds after which the caller should call update().
		std::chrono::seconds get_poll_in_seconds() const
		{
			return poll_in_seconds_;
		}


		/// Get a constant "test duration during idle" capability drive's stored capabilities. -1 if N/A.
		std::chrono::seconds get_min_duration_seconds() const;


		/// Gets the current test type support status from drive's stored capabilities.
		bool is_supported() const;


		/// Start the test.
		/// \return error message on error, empty string on success.
		std::string start(hz::intrusive_ptr<CmdexSync> smartctl_ex = nullptr);


		/// Abort the running test.
		/// \return error message on error, empty string on success.
		std::string force_stop(hz::intrusive_ptr<CmdexSync> smartctl_ex = nullptr);


		/// Update status variables. The user should call this every get_poll_in_seconds() seconds.
		/// \return error message on error, empty string on success.
		std::string update(hz::intrusive_ptr<CmdexSync> smartctl_ex = nullptr);


	private:

		StorageDeviceRefPtr drive_;  ///< Drive to run the tests on
		test_t type_;  ///< Test type

		// status variables:
		StorageSelftestEntry::status_t status_;  ///< Current status of the test as reported by the drive
		int8_t remaining_percent_;  ///< Remaining %. 0 means unknown, -1 means N/A. This is set to 100 on start.
		int8_t last_seen_percent_;  ///< Last reported %, to detect changes in percentage (needed for timer update).
		mutable std::chrono::seconds total_duration_;  ///< Total duration needed for the test, as reported by the drive. Constant. This variable acts as a cache.
		std::chrono::seconds poll_in_seconds_;  ///< The user is asked to poll after this much seconds have passed.

		Glib::Timer timer_;  ///< Counts time since the last percent change

};



/// A reference-counting pointer to SelfTest
using SelfTestPtr = hz::intrusive_ptr<SelfTest>;




#endif

/// @}
