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
#include <memory>
#include <string>
#include <cstdint>
#include <chrono>
#include <unordered_map>

#include "storage_device.h"
#include "command_executor.h"
#include "hz/error_container.h"


enum class SelfTestExecutionError {
	InternalError,
	AlreadyRunning,
	UnsupportedTest,
	InvalidTestType,
	CommandFailed,
	CommandUnknownError,
	NotRunning,
	StopUnsupported,
	UpdateError,
	ParseError,
	ReportUnsupported,
};


/// Self-test status
enum class SelfTestStatus {
	Unknown,
	InProgress,
	ManuallyAborted,
	Interrupted,
	CompletedNoError,
	CompletedWithError,
	Reserved,
};



/// Helper structure for enum-related functions
struct SelfTestStatusExt
		: public hz::EnumHelper<
				SelfTestStatus,
				SelfTestStatusExt,
				Glib::ustring>
{
	static constexpr EnumType default_value = EnumType::Unknown;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{SelfTestStatus::Unknown, {"unknown", _("Unknown")}},
			{SelfTestStatus::InProgress, {"in_progress", _("In Progress")}},
			{SelfTestStatus::ManuallyAborted, {"manually_aborted", _("Manually Aborted")}},
			{SelfTestStatus::Interrupted, {"interrupted", _("Interrupted")}},
			{SelfTestStatus::CompletedNoError, {"completed_no_error", _("Completed Successfully")}},
			{SelfTestStatus::CompletedWithError, {"completed_with_error", _("Completed With Errors")}},
			{SelfTestStatus::Reserved, {"reserved", _("Reserved")}},
		};
	}

};




/// Self-test error severity
enum class SelfTestStatusSeverity {
	None,
	Warning,
	Error,
};


/// Get severity of error status
[[nodiscard]] SelfTestStatusSeverity get_self_test_status_severity(SelfTestStatus s);




/// SMART self-test runner.
class SelfTest {
	public:

		/// Test type
		enum class TestType {
//			ImmediateOffline,  ///< Immediate offline, not supported
			ShortTest,  ///< Short self-test
			LongTest,  ///< Extended (a.k.a. long) self-test
			Conveyance  ///< Conveyance self-test
		};


		/// Get displayable name for a test type
		[[nodiscard]] static std::string get_test_displayable_name(TestType type);


		/// Constructor. \c drive must have the capabilities present in its properties.
		SelfTest(StorageDevicePtr drive, TestType type)
			: drive_(std::move(drive)), type_(type)
		{ }


		/// Check if the test is currently active
		[[nodiscard]] bool is_active() const;


		/// Get remaining time percent until the test completion.
		/// \return -1 if N/A or unknown.
		[[nodiscard]] int8_t get_remaining_percent() const;


		/// Get estimated time of completion for the test.
		/// \return -1 if N/A or unknown. Note that 0 is a valid value.
		[[nodiscard]] std::chrono::seconds get_remaining_seconds() const;


		/// Get test type
		[[nodiscard]] TestType get_test_type() const;


		/// Get test status
		[[nodiscard]] SelfTestStatus get_status() const;


		/// Get the number of seconds after which the caller should call update().
		/// Returns -1 if the test is not running.
		[[nodiscard]] std::chrono::seconds get_poll_in_seconds() const;


		/// Get a constant "test duration during idle" capability drive's stored capabilities. -1 if N/A.
		[[nodiscard]] std::chrono::seconds get_min_duration_seconds() const;


		/// Gets the current test type support status from drive's stored capabilities.
		[[nodiscard]] bool is_supported() const;


		/// Start the test. Note that this object is not reusable, start() must be called
		/// only on newly constructed objects.
		/// \return error message on error, empty string on success.
		hz::ExpectedVoid<SelfTestExecutionError> start(const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);


		/// Abort the running test.
		/// \return error message on error, empty string on success.
		hz::ExpectedVoid<SelfTestExecutionError> force_stop(const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);


		/// Update status variables. The user should call this every get_poll_in_seconds() seconds.
		/// \return error message on error, empty string on success.
		hz::ExpectedVoid<SelfTestExecutionError> update(const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);


	private:

		StorageDevicePtr drive_;  ///< Drive to run the tests on
		TestType type_ = TestType::ShortTest;  ///< Test type

		// status variables:
		SelfTestStatus status_ = SelfTestStatus::Unknown;  ///< Current status of the test as reported by the drive
		int8_t remaining_percent_ = -1;  ///< Remaining %. 0 means unknown, -1 means N/A. This is set to 100 on start.
		int8_t last_seen_percent_ = -1;  ///< Last reported %, to detect changes in percentage (needed for timer update).
		mutable std::chrono::seconds total_duration_ = std::chrono::seconds(-1);  ///< Total duration needed for the test, as reported by the drive. Constant. This variable acts as a cache.
		std::chrono::seconds poll_in_seconds_ = std::chrono::seconds(-1);  ///< The user is asked to poll after this much seconds have passed.

		Glib::Timer timer_;  ///< Counts time since the last percent change

};



#endif

/// @}
