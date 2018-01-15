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

#include <algorithm>  // std::max, std::min
#include <cmath>  // std::floor
#include <chrono>

#include "app_pcrecpp.h"
#include "storage_property.h"
#include "smartctl_parser.h"
#include "selftest.h"



// Returns estimated time of completion for the test. returns -1 if n/a or unknown. 0 is a valid value.
std::chrono::seconds SelfTest::get_remaining_seconds() const
{
	using namespace std::literals;

	std::chrono::seconds total = get_min_duration_seconds();
	if (total <= 0s)
		return -1s;  // unknown

	double gran = (double(total.count()) / 9.);  // seconds per 10%
	// since remaining_percent_ may be manually set to 100, we limit from the above.
	double rem_seconds_at_last_change = std::min(double(total.count()), gran * remaining_percent_ / 10.);
	double rem = rem_seconds_at_last_change - timer_.elapsed();
	return std::chrono::seconds(std::max(int64_t(0), (int64_t)std::round(rem)));  // don't return negative values.
}



// a drive reports a constant "test duration during idle" capability.
std::chrono::seconds SelfTest::get_min_duration_seconds() const
{
	using namespace std::literals;

	if (!drive_)
		return -1s;  // n/a

	if (total_duration_ != -1s)  // cache
		return total_duration_;

	std::string prop_name;
	switch(type_) {
		case type_ioffline: prop_name = "iodc_total_time_length"; break;
		case type_short: prop_name = "short_total_time_length"; break;
		case type_long: prop_name = "long_total_time_length"; break;
		case type_conveyance: prop_name = "conveyance_total_time_length"; break;
	}

	StorageProperty p = drive_->lookup_property(prop_name,
			StorageProperty::section_data, StorageProperty::subsection_capabilities);

	// p stores it as uint64_t
	return (total_duration_ = (p.empty() ? 0s : p.get_value<std::chrono::seconds>()));
}



bool SelfTest::is_supported() const
{
	if (!drive_)
		return false;

	if (type_ == type_ioffline)  // disable this for now - it's unsupported.
		return false;

	std::string prop_name;
	switch(type_) {
		case type_ioffline: prop_name = "iodc_support"; break;
		case type_short: prop_name = "selftest_support"; break;
		case type_long: prop_name = "selftest_support"; break;  // same for short and long
		case type_conveyance: prop_name = "conveyance_support"; break;
	}

	StorageProperty p = drive_->lookup_property(prop_name, StorageProperty::section_internal);
	return (!p.empty() && p.get_value<bool>());
}




// start the test
std::string SelfTest::start(hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	this->clear();  // clear previous results

	if (!drive_)
		return "Invalid drive given.";
	if (drive_->get_test_is_active())
		return "A test is already running on this drive.";
	if (!this->is_supported())
		return get_test_name(type_) + " is unsupported by this drive.";

	std::string test_param;
	switch(type_) {
		case type_ioffline: test_param = "offline"; break;
		case type_short: test_param = "short"; break;
		case type_long: test_param = "long"; break;
		case type_conveyance: test_param = "conveyance"; break;
		// no default - this way we get warned by compiler if we're not listing all of them.
	}
	if (test_param.empty())
		return "Invalid test specified";

	std::string output;
	std::string error_msg = drive_->execute_device_smartctl("--test=" + test_param, smartctl_ex, output);

	if (!error_msg.empty())  // checks for empty output too
		return error_msg;

	if (!app_pcre_match(R"(/^Drive command .* successful\.\nTesting has begun\.$/mi)", output)) {
		return "Sending command failed.";
	}


	// update our members
// 	error_msg = this->update(smartctl_ex);
// 	if (!error_msg.empty())  // update can error out too.
// 		return error_msg;

	// Don't update here - the logs may not be updated this fast.
	// Better to wait several seconds and then call it manually.

	// Set up everything so that the caller won't have to.

	status_ = StorageSelftestEntry::status_in_progress;

	remaining_percent_ = 100;
	// set to 90 to avoid the 100->90 timer reset. this way we won't be looking at
	// "remaining 60sec" on 60sec test twice (5 seconds apart). Since the test starts
	// at 90% anyway, it's a good thing.
	last_seen_percent_ = 90;
	poll_in_seconds_ = std::chrono::seconds(5);  // first update() in 5 seconds
	timer_.start();

	drive_->set_test_is_active(true);


	return std::string();  // everything ok
}



// abort test.
std::string SelfTest::force_stop(hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	if (!drive_)
		return "Invalid drive given.";
	if (!drive_->get_test_is_active())
		return "No test is currently running on this drive.";

	// To abort immediate offline test, the device MUST have
	// "Abort Offline collection upon new command" capability,
	// any command (e.g. "--abort") will abort it. If it has "Suspend Offline...",
	// there's no way to abort such test.
	if (type_ == type_ioffline) {
		StorageProperty p = drive_->lookup_property("iodc_command_suspends", StorageProperty::section_internal);
		if (!p.empty() && p.get_value<bool>()) {  // if empty, give a chance to abort anyway.
			return "Aborting this test is unsupported by the drive.";
		}
		// else, proceed as any other test
	}

	// To abort non-captive short, long and conveyance tests, use "--abort".
	std::string output;
	std::string error_msg = drive_->execute_device_smartctl("--abort", smartctl_ex, output);

	if (!error_msg.empty())  // checks for empty output too
		return error_msg;

	// this command prints success even if no test was running.
	if (!app_pcre_match("/^Self-testing aborted!$/mi", output)) {
		return "Sending command failed.";
	}

	// update our members
	error_msg = this->update(smartctl_ex);

	// the thing is, update() may fail to actually update the statuses, so
	// do it manually.
	if (status_ == StorageSelftestEntry::status_in_progress) {  // update() couldn't do its job
		status_ = StorageSelftestEntry::status_aborted_by_host;
		remaining_percent_ = -1;
		last_seen_percent_ = -1;
		poll_in_seconds_ = std::chrono::seconds(-1);
		timer_.stop();
		drive_->set_test_is_active(false);
	}

	if (!error_msg.empty())  // update can error out too.
		return error_msg;
	return std::string();  // everything ok
}



// update status variables. note: the returned error is an error in logic,
// not an hw defect error.
std::string SelfTest::update(hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	using namespace std::literals;

	if (!drive_)
		return "Invalid drive given.";

	std::string output;
// 	std::string error_msg = drive_->execute_device_smartctl("--log=selftest", smartctl_ex, output);
	std::string error_msg = drive_->execute_device_smartctl("--capabilities", smartctl_ex, output);

	if (!error_msg.empty())  // checks for empty output too
		return error_msg;

	StorageAttribute::DiskType disk_type = drive_->get_is_hdd() ? StorageAttribute::DiskHDD : StorageAttribute::DiskSSD;
	SmartctlParser ps;
	if (!ps.parse_full(output, disk_type)) {  // try to parse it
		return ps.get_error_msg();
	}

	// Note: Since the self-test log is sometimes late
	// and in undetermined order (sorting by hours is too rough),
	// we use the "self-test status" capability.
	StorageProperty p;
	for (auto&& e : ps.get_properties()) {
// 		if (e.section != StorageProperty::section_data || e.subsection != StorageProperty::subsection_selftest_log
		if (e.section != StorageProperty::section_internal
				|| !e.is_value_type<StorageSelftestEntry>() || e.get_value<StorageSelftestEntry>().test_num != 0
				|| e.generic_name != "last_selftest_status")
			continue;
		p = e;
	}

	if (p.empty())
		return "The drive doesn't report the test status.";

	status_ = p.get_value<StorageSelftestEntry>().status;
	bool active = (status_ == StorageSelftestEntry::status_in_progress);


	// Note that the test needs 90% to complete, not 100. It starts at 90%
	// and reaches 00% on completion. That's 9 pieces.
	if (active) {

		remaining_percent_ = p.get_value<StorageSelftestEntry>().remaining_percent;
		if (remaining_percent_ != last_seen_percent_) {
			last_seen_percent_ = remaining_percent_;
			timer_.start();  // restart the timer
		}

		std::chrono::seconds total = get_min_duration_seconds();

		if (total <= 0s) {  // unknown
			poll_in_seconds_ = 30s;  // just a guess

		} else {
			// seconds per 10%. use double, because e.g. 60sec test gives silly values with int.
			double gran = (double(total.count()) / 9.);

			// Add 1/10 for disk load delays, etc... . Limit to 15sec, in case of very quick tests.
			poll_in_seconds_ = std::chrono::seconds(std::max(int64_t(15), int64_t(gran / 3. + (gran / 10.))));

			// for long tests we don't want to make the user wait too much, so
			// we need to poll more frequently by the end, in case it's completed.
			if (type_ == type_long && remaining_percent_ == 10)
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

	drive_->set_test_is_active(active);

	return std::string();  // everything ok
}






/// @}
