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

#ifndef SELFTEST_H
#define SELFTEST_H

#include "local_glibmm.h"
#include <string>
#include <cstdint>
#include <chrono>
#include <unordered_map>

#include "storage_device.h"
#include "command_executor.h"



/// SMART self-test runner.
class SelfTest {
	public:

		/// Test type
		enum class TestType {
			immediate_offline,  ///< Immediate offline, not supported
			short_test,  ///< Short self-test
			long_test,  ///< Extended (a.k.a. long) self-test
			conveyance  ///< Conveyance self-test
		};


		/// Get displayable name for a test type
		static std::string get_test_displayable_name(TestType type);


		/// Constructor. \c drive must have the capabilities present in its properties.
		SelfTest(StorageDevicePtr drive, TestType type)
			: drive_(std::move(drive)), type_(type)
		{ }


		/// Check if the test is currently active
		bool is_active() const
		{
			return (status_ == StorageSelftestEntry::Status::in_progress);
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
		TestType get_test_type() const
		{
			return type_;
		}


		/// Get test status
		StorageSelftestEntry::Status get_status() const
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


		/// Start the test. Note that this object is not reusable, start() must be called
		/// only on newly constructed objects.
		/// \return error message on error, empty string on success.
		std::string start(const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);


		/// Abort the running test.
		/// \return error message on error, empty string on success.
		std::string force_stop(const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);


		/// Update status variables. The user should call this every get_poll_in_seconds() seconds.
		/// \return error message on error, empty string on success.
		std::string update(const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);


	private:

		StorageDevicePtr drive_;  ///< Drive to run the tests on
		TestType type_ = TestType::short_test;  ///< Test type

		// status variables:
		StorageSelftestEntry::Status status_ = StorageSelftestEntry::Status::unknown;  ///< Current status of the test as reported by the drive
		int8_t remaining_percent_ = -1;  ///< Remaining %. 0 means unknown, -1 means N/A. This is set to 100 on start.
		int8_t last_seen_percent_ = -1;  ///< Last reported %, to detect changes in percentage (needed for timer update).
		mutable std::chrono::seconds total_duration_ = std::chrono::seconds(-1);  ///< Total duration needed for the test, as reported by the drive. Constant. This variable acts as a cache.
		std::chrono::seconds poll_in_seconds_ = std::chrono::seconds(-1);  ///< The user is asked to poll after this much seconds have passed.

		Glib::Timer timer_;  ///< Counts time since the last percent change

};



#endif

/// @}
