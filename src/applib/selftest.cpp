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

#include "hz/error_container.h"
#include "command_executor.h"
#include <glibmm.h>
#include <algorithm>  // std::max, std::min
#include <cmath>  // std::floor
#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string>

#include "smartctl_parser_types.h"
#include "smartctl_parser.h"
#include "storage_device_detected_type.h"
#include "storage_property.h"
#include "smartctl_text_ata_parser.h"
#include "selftest.h"
#include "storage_property_descr.h"
#include "smartctl_version_parser.h"
#include "app_regex.h"



SelfTestStatusSeverity get_self_test_status_severity(SelfTestStatus s)
{
	static const std::unordered_map<SelfTestStatus, SelfTestStatusSeverity> m {
			{SelfTestStatus::Unknown,                SelfTestStatusSeverity::None},
			{SelfTestStatus::CompletedNoError,       SelfTestStatusSeverity::None},
			{SelfTestStatus::ManuallyAborted,          SelfTestStatusSeverity::Warning},
			{SelfTestStatus::Interrupted,            SelfTestStatusSeverity::Warning},
			{SelfTestStatus::CompletedWithError,         SelfTestStatusSeverity::Error},
			{SelfTestStatus::InProgress,             SelfTestStatusSeverity::None},
			{SelfTestStatus::Reserved,               SelfTestStatusSeverity::None},
	};
	if (auto iter = m.find(s); iter != m.end()) {
		return iter->second;
	}
	return SelfTestStatusSeverity::None;
}



std::string SelfTest::get_test_displayable_name(SelfTest::TestType type)
{
	static const std::unordered_map<TestType, std::string> m {
//			{TestType::ImmediateOffline, _("Immediate Offline Test")},
			{TestType::ShortTest,        _("Short Self-Test")},
			{TestType::LongTest,         _("Extended Self-Test")},
			{TestType::Conveyance,       _("Conveyance Self-Test")},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



bool SelfTest::is_active() const
{
	return (status_ == SelfTestStatus::InProgress);
}



int8_t SelfTest::get_remaining_percent() const
{
	return remaining_percent_;
}



// Returns estimated time of completion for the test. returns -1 if n/a or unknown. 0 is a valid value.
std::chrono::seconds SelfTest::get_remaining_seconds() const
{
	using namespace std::literals;

	const std::chrono::seconds total = get_min_duration_seconds();
	if (total <= 0s)
		return -1s;  // unknown

	const double gran = (double(total.count()) / 9.);  // seconds per 10%
	// since remaining_percent_ may be manually set to 100, we limit from the above.
	const double rem_seconds_at_last_change = std::min(double(total.count()), gran * remaining_percent_ / 10.);
	const double rem = rem_seconds_at_last_change - timer_.elapsed();
	return std::chrono::seconds(std::max(int64_t(0), (int64_t)std::round(rem)));  // don't return negative values.
}



SelfTest::TestType SelfTest::get_test_type() const
{
	return type_;
}



SelfTestStatus SelfTest::get_status() const
{
	return status_;
}



std::chrono::seconds SelfTest::get_poll_in_seconds() const
{
	return poll_in_seconds_;
}



// a drive reports a constant "test duration during idle" capability.
std::chrono::seconds SelfTest::get_min_duration_seconds() const
{
	using namespace std::literals;

	if (!drive_)
		return -1s;  // n/a

	if (total_duration_ != -1s)  // cache
		return total_duration_;

	if (drive_->get_detected_type() == StorageDeviceDetectedType::Nvme) {
		return -1s;  // NVMe doesn't report this.
	}

	// ATA
	std::string prop_name;
	switch(type_) {
//		case TestType::ImmediateOffline: prop_name = "ata_smart_data/offline_data_collection/completion_seconds"; break;
		case TestType::ShortTest: prop_name = "ata_smart_data/self_test/polling_minutes/short"; break;
		case TestType::LongTest: prop_name = "ata_smart_data/self_test/polling_minutes/extended"; break;
		case TestType::Conveyance: prop_name = "ata_smart_data/self_test/polling_minutes/conveyance"; break;
	}

	const StorageProperty p = drive_->get_property_repository().lookup_property(prop_name,
			StoragePropertySection::Capabilities);

	// p stores it as uint64_t
	return (total_duration_ = (p.empty() ? 0s : p.get_value<std::chrono::seconds>()));
}



bool SelfTest::is_supported() const
{
	if (!drive_)
		return false;

	if (drive_->get_detected_type() == StorageDeviceDetectedType::Nvme) {
		switch (type_) {
//			case TestType::ImmediateOffline:
			case TestType::Conveyance:
				return false;  // not supported by nvme
			case TestType::ShortTest:
			case TestType::LongTest:
				// NVMe spec
				return true;
		}

	} else if (drive_->get_detected_type() == StorageDeviceDetectedType::AtaAny
			|| drive_->get_detected_type() == StorageDeviceDetectedType::AtaHdd
			|| drive_->get_detected_type() == StorageDeviceDetectedType::AtaSsd) {

		// Find appropriate capability
		std::string prop_name;
		switch(type_) {
//			case TestType::ImmediateOffline:
				// prop_name = "ata_smart_data/capabilities/exec_offline_immediate_supported";
				// break;
//				return false;  // disable this for now - it's unsupported by this application.
			case TestType::ShortTest:
			case TestType::LongTest:  // same for short and long
				prop_name = "ata_smart_data/capabilities/self_tests_supported";
				break;
			case TestType::Conveyance:
				prop_name = "ata_smart_data/capabilities/conveyance_self_test_supported";
				break;
		}

		const StorageProperty p = drive_->get_property_repository().lookup_property(prop_name);
		return (!p.empty() && p.get_value<bool>());
	}

	return false;
}




// start the test
hz::ExpectedVoid<SelfTestExecutionError> SelfTest::start(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (!drive_) {
		return hz::Unexpected(SelfTestExecutionError::InternalError, _("Internal Error: Drive must not be NULL."));
	}
	if (drive_->get_test_is_active()) {
		return hz::Unexpected(SelfTestExecutionError::AlreadyRunning, _("A test is already running on this drive."));
	}
	if (!this->is_supported()) {
		// Translators: {} is a test name - Short test, etc.
		std::string type_name = get_test_displayable_name(type_);
		return hz::Unexpected(SelfTestExecutionError::UnsupportedTest,
				std::vformat(_("{} is unsupported by this drive."), std::make_format_args(type_name)));
	}

	std::string test_param;
	switch(type_) {
//		case TestType::ImmediateOffline: test_param = "offline"; break;
		case TestType::ShortTest: test_param = "short"; break;
		case TestType::LongTest: test_param = "long"; break;
		case TestType::Conveyance: test_param = "conveyance"; break;
		// no default - this way we get warned by compiler if we're not listing all of them.
	}
	if (test_param.empty()) {
		return hz::Unexpected(SelfTestExecutionError::InvalidTestType, _("Invalid test specified."));
	}

	std::string output;
	auto execute_status = drive_->execute_device_smartctl({"--test=" + test_param}, smartctl_ex, output);

	if (!execute_status.has_value()) {
		std::string message = execute_status.error().message();
		return hz::Unexpected(SelfTestExecutionError::CommandFailed,
				std::vformat(_("Sending command to drive failed: {}"), std::make_format_args(message)));
	}

	const bool ata_test_started = app_regex_partial_match(R"(/^Drive command .* successful\.\nTesting has begun\.$/mi)", output);
	const bool nvme_test_started = app_regex_partial_match(R"(/^Self-test has begun$/mi)", output);
	const bool nvme_test_running = app_regex_partial_match(R"(/^Can't start self-test without aborting current test/mi)", output);

	if (!ata_test_started && !nvme_test_started && !nvme_test_running) {
		return hz::Unexpected(SelfTestExecutionError::CommandUnknownError, _("Sending command to drive failed."));
	}

	// update our members
// 	error_message = this->update(smartctl_ex);
// 	if (!error_message.empty())  // update can error out too.
// 		return error_message;

	// Don't update here - the logs may not be updated this fast.
	// Better to wait several seconds and then call it manually.

	// Set up everything so that the caller won't have to.

	status_ = SelfTestStatus::InProgress;

	remaining_percent_ = 100;
	// set to 90 to avoid the 100->90 timer reset. this way we won't be looking at
	// "remaining 60sec" on 60sec test twice (5 seconds apart). Since the test starts
	// at 90% anyway, it's a good thing.
	last_seen_percent_ = 90;
	poll_in_seconds_ = std::chrono::seconds(5);  // first update() in 5 seconds
	timer_.start();

	drive_->set_test_is_active(true);

	return {};  // everything ok
}



// abort test.
hz::ExpectedVoid<SelfTestExecutionError> SelfTest::force_stop(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (!drive_) {
		return hz::Unexpected(SelfTestExecutionError::InternalError, _("Internal Error: Drive must not be NULL."));
	}
	if (!drive_->get_test_is_active()) {
		return hz::Unexpected(SelfTestExecutionError::NotRunning, _("No test is currently running on this drive."));
	}

	// To abort immediate offline test, the device MUST have
	// "Abort Offline collection upon new command" capability,
	// any command (e.g. "--abort") will abort it. If it has "Suspend Offline...",
	// there's no way to abort such test.
//	if (type_ == TestType::ImmediateOffline) {
//		const StorageProperty p = drive_->get_property_repository().lookup_property(
//				"ata_smart_data/capabilities/offline_is_aborted_upon_new_cmd");
//		if (!p.empty() && p.get_value<bool>()) {  // if empty, give a chance to abort anyway.
//			return hz::Unexpected(SelfTestError::StopUnsupported, _("Aborting this test is unsupported by the drive."));
//		}
//		// else, proceed as any other test
//	}

	// To abort non-captive short, long and conveyance tests, use "--abort".
	std::string output;
	auto execute_status = drive_->execute_device_smartctl({"--abort"}, smartctl_ex, output);

	if (!execute_status) {
		std::string message = execute_status.error().message();
		return hz::Unexpected(SelfTestExecutionError::CommandFailed,
				std::vformat(_("Sending command to drive failed: {}"), std::make_format_args(message)));
	}

	// this command prints success even if no test was running.
	const bool ata_aborted = app_regex_partial_match("/^Self-testing aborted!$/mi", output);
	const bool nvme_aborted = app_regex_partial_match("/^Self-test aborted!$/mi", output);

	if (!ata_aborted && !nvme_aborted) {
		return hz::Unexpected(SelfTestExecutionError::CommandUnknownError, _("Sending command to drive failed."));
	}

	// update our members
	auto update_status = this->update(smartctl_ex);

	// the thing is, update() may fail to actually update the statuses, so
	// do it manually.
	if (status_ == SelfTestStatus::InProgress) {  // update() couldn't do its job
		status_ = SelfTestStatus::ManuallyAborted;
		remaining_percent_ = -1;
		last_seen_percent_ = -1;
		poll_in_seconds_ = std::chrono::seconds(-1);
		timer_.stop();
		drive_->set_test_is_active(false);
	}

	if (!update_status) {  // update can error out too.
		std::string message = update_status.error().message();
		return hz::Unexpected(SelfTestExecutionError::UpdateError,
				std::vformat(_("Error fetching test progress information: {}"), std::make_format_args(message)));
	}

	return {};  // everything ok
}



// update status variables. note: the returned error is an error in logic,
// not a hw defect error.
hz::ExpectedVoid<SelfTestExecutionError> SelfTest::update(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	using namespace std::literals;

	if (!drive_) {
		return hz::Unexpected(SelfTestExecutionError::InternalError, _("Internal Error: Drive must not be NULL."));
	}

	SmartctlParserType parser_type = SmartctlParserType::Ata;
	if (drive_->get_detected_type() == StorageDeviceDetectedType::Nvme) {
		parser_type = SmartctlParserType::Nvme;
	}
	auto parser_format = SmartctlVersionParser::get_default_format(parser_type);

	// ATA shows status in capabilities; NVMe shows it in self-test log.
	std::vector<std::string> command_options = {"--capabilities", "--log=selftest"};
	if (parser_format == SmartctlOutputFormat::Json) {
		// --json flags: o means include original output (just in case).
		command_options.push_back(" --json=o");
	}

	std::string output;
	auto execute_status = drive_->execute_device_smartctl(command_options, smartctl_ex, output);

	if (!execute_status) {
		std::string message = execute_status.error().message();
		return hz::Unexpected(SelfTestExecutionError::CommandFailed,
				std::vformat(_("Sending command to drive failed: {}"), std::make_format_args(message)));
	}


	std::shared_ptr<SmartctlParser> parser = SmartctlParser::create(parser_type, parser_format);
	DBG_ASSERT_RETURN(parser, hz::Unexpected(SelfTestExecutionError::ParseError, _("Cannot create parser.")));

	auto parse_status = parser->parse(output);
	if (!parse_status) {
		return hz::Unexpected(SelfTestExecutionError::ParseError,
				std::vformat(_("Cannot parse smartctl output: {}"), std::make_format_args(parse_status.error().message())));
	}
	const auto property_repo = StoragePropertyProcessor::process_properties(
			parser->get_property_repository(), drive_->get_detected_type());


	if (drive_->get_detected_type() == StorageDeviceDetectedType::Nvme) {

		const StorageProperty current_operation = property_repo.lookup_property("nvme_self_test_log/current_self_test_operation/value/_decoded");

		// If no test is active, the property may be absent, or set to None.
		if (!current_operation.empty()
				&& current_operation.get_value<std::string>() != NvmeSelfTestCurrentOperationTypeExt::get_storable_name(NvmeSelfTestCurrentOperationType::None)) {
			status_ = SelfTestStatus::InProgress;

			auto remaining_percent = property_repo.lookup_property("nvme_self_test_log/current_self_test_completion_percent");
			if (!remaining_percent.empty()) {
				remaining_percent_ = static_cast<int8_t>(100 - remaining_percent.get_value<int64_t>());
			}
		} else {  // no test is active
			// The first self-test table entry is the latest.
			std::optional<NvmeStorageSelftestEntry> entry;
			for (const auto& e : property_repo.get_properties()) {
				if (e.is_value_type<NvmeStorageSelftestEntry>() && e.get_value<NvmeStorageSelftestEntry>().test_num == 1) {
					entry = e.get_value<NvmeStorageSelftestEntry>();
				}
			}
			if (!entry) {
				return hz::Unexpected(SelfTestExecutionError::ReportUnsupported, _("The drive doesn't report the test status."));
			}

			switch (entry->result) {
				case NvmeSelfTestResultType::Unknown:
					status_ = SelfTestStatus::Unknown;
					break;
				case NvmeSelfTestResultType::CompletedNoError:
					status_ = SelfTestStatus::CompletedNoError;
					break;
				case NvmeSelfTestResultType::AbortedSelfTestCommand:
					status_ = SelfTestStatus::ManuallyAborted;
					break;
				case NvmeSelfTestResultType::AbortedControllerReset:
				case NvmeSelfTestResultType::AbortedNamespaceRemoved:
				case NvmeSelfTestResultType::AbortedFormatNvmCommand:
				case NvmeSelfTestResultType::AbortedUnknownReason:
				case NvmeSelfTestResultType::AbortedSanitizeOperation:
					status_ = SelfTestStatus::Interrupted;
					break;
				case NvmeSelfTestResultType::FatalOrUnknownTestError:
				case NvmeSelfTestResultType::CompletedUnknownFailedSegment:
				case NvmeSelfTestResultType::CompletedFailedSegments:
					status_ = SelfTestStatus::CompletedWithError;
					break;
			}
		}

	} else {
		// ATA:
		// Note: Since the self-test log is sometimes late
		// and in undetermined order (sorting by hours is too rough),
		// we use the "self-test status" capability.
		StorageProperty p;
		for (const auto& e : property_repo.get_properties()) {
			if (e.is_value_type<AtaStorageSelftestEntry>() || e.get_value<AtaStorageSelftestEntry>().test_num != 0
					|| e.generic_name != "ata_smart_data/self_test/status/_merged")
				continue;
			p = e;
		}
		if (p.empty()) {
			return hz::Unexpected(SelfTestExecutionError::ReportUnsupported, _("The drive doesn't report the test status."));
		}

		switch (p.get_value<AtaStorageSelftestEntry>().status) {
			case AtaStorageSelftestEntry::Status::InProgress:
				status_ = SelfTestStatus::InProgress;
				break;
			case AtaStorageSelftestEntry::Status::Unknown:
				status_ = SelfTestStatus::Unknown;
				break;
			case AtaStorageSelftestEntry::Status::Reserved:
				status_ = SelfTestStatus::Reserved;
				break;
			case AtaStorageSelftestEntry::Status::CompletedNoError:
				status_ = SelfTestStatus::CompletedNoError;
				break;
			case AtaStorageSelftestEntry::Status::AbortedByHost:
				status_ = SelfTestStatus::ManuallyAborted;
				break;
			case AtaStorageSelftestEntry::Status::Interrupted:
				status_ = SelfTestStatus::Interrupted;
				break;
			case AtaStorageSelftestEntry::Status::FatalOrUnknown:
			case AtaStorageSelftestEntry::Status::ComplUnknownFailure:
			case AtaStorageSelftestEntry::Status::ComplElectricalFailure:
			case AtaStorageSelftestEntry::Status::ComplServoFailure:
			case AtaStorageSelftestEntry::Status::ComplReadFailure:
			case AtaStorageSelftestEntry::Status::ComplHandlingDamage:
				status_ = SelfTestStatus::CompletedWithError;
				break;
		}

		if (status_ == SelfTestStatus::InProgress) {
			remaining_percent_ = p.get_value<AtaStorageSelftestEntry>().remaining_percent;
		}
	}

	// Note that the test needs 90% to complete, not 100. It starts at 90%
	// and reaches 00% on completion. That's 9 pieces.
	if (status_ == SelfTestStatus::InProgress) {
		if (remaining_percent_ != last_seen_percent_) {
			last_seen_percent_ = remaining_percent_;
			timer_.start();  // restart the timer
		}

		const std::chrono::seconds total = get_min_duration_seconds();

		if (total <= 0s) {  // unknown
			poll_in_seconds_ = 15s;  // just a guess, quick enough for nvme

		} else {
			// seconds per 10%. use double, because e.g. 60sec test gives silly values with int.
			const double gran = (double(total.count()) / 9.);

			// Add 1/10 for disk load delays, etc. . Limit to 15sec, in case of very quick tests.
			poll_in_seconds_ = std::chrono::seconds(std::max(int64_t(15), int64_t(gran / 3. + (gran / 10.))));

			// for long tests we don't want to make the user wait too much, so
			// we need to poll more frequently by the end, in case it's completed.
			if (type_ == TestType::LongTest && remaining_percent_ == 10)
				poll_in_seconds_ = std::chrono::seconds(std::max(int64_t(1*60), int64_t(gran / 10.)));  // that's 2 min for 180min extended test

			debug_out_dump("app", DBG_FUNC_MSG << "total: " << total.count() << ", gran: " << gran
					<< ", poll in: " << poll_in_seconds_.count() << ", remaining secs: " << get_remaining_seconds().count()
					<< ", remaining %: " << int(remaining_percent_) << ", last seen %: " << int(last_seen_percent_) << ".\n");
		}

	} else {
		remaining_percent_ = -1;
		last_seen_percent_ = -1;
		poll_in_seconds_ = -1s;
		timer_.stop();
	}

	drive_->set_test_is_active(status_ == SelfTestStatus::InProgress);

	return {};  // everything ok
}






/// @}
