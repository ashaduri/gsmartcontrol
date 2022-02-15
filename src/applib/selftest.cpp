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

#include "local_glibmm.h"
#include <algorithm>  // std::max, std::min
#include <cmath>  // std::floor
#include <chrono>

#include "app_pcrecpp.h"
#include "ata_storage_property.h"
#include "smartctl_ata_text_parser.h"
#include "selftest.h"
#include "ata_storage_property_descr.h"




std::string SelfTest::get_test_displayable_name(SelfTest::TestType type)
{
	static const std::unordered_map<TestType, std::string> m {
			{TestType::immediate_offline, _("Immediate Offline Test")},
			{TestType::short_test, _("Short Self-Test")},
			{TestType::long_test, _("Extended Self-Test")},
			{TestType::conveyance, _("Conveyance Self-Test")},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



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
		case TestType::immediate_offline: prop_name = "ata_smart_data/offline_data_collection/completion_seconds"; break;
		case TestType::short_test: prop_name = "ata_smart_data/self_test/polling_minutes/short"; break;
		case TestType::long_test: prop_name = "ata_smart_data/self_test/polling_minutes/extended"; break;
		case TestType::conveyance: prop_name = "ata_smart_data/self_test/polling_minutes/conveyance"; break;
	}

	AtaStorageProperty p = drive_->lookup_property(prop_name,
			AtaStorageProperty::Section::data, AtaStorageProperty::SubSection::capabilities);

	// p stores it as uint64_t
	return (total_duration_ = (p.empty() ? 0s : p.get_value<std::chrono::seconds>()));
}



bool SelfTest::is_supported() const
{
	if (!drive_)
		return false;

	std::string prop_name;
	switch(type_) {
		case TestType::immediate_offline:
			// prop_name = "ata_smart_data/capabilities/exec_offline_immediate_supported";
			// break;
			return false;  // disable this for now - it's unsupported.
		case TestType::short_test:
		case TestType::long_test:  // same for short and long
			prop_name = "ata_smart_data/capabilities/self_tests_supported";
			break;
		case TestType::conveyance: prop_name = "ata_smart_data/capabilities/conveyance_self_test_supported"; break;
	}

	AtaStorageProperty p = drive_->lookup_property(prop_name, AtaStorageProperty::Section::internal);
	return (!p.empty() && p.get_value<bool>());
}




// start the test
std::string SelfTest::start(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (!drive_)
		return "[internal error: drive must not be NULL]";
	if (drive_->get_test_is_active())
		return _("A test is already running on this drive.");
	if (!this->is_supported()) {
		/// Translators: %1 is test name - Short test, etc...
		return Glib::ustring::compose(_("%1 is unsupported by this drive."), get_test_displayable_name(type_));
	}

	std::string test_param;
	switch(type_) {
		case TestType::immediate_offline: test_param = "offline"; break;
		case TestType::short_test: test_param = "short"; break;
		case TestType::long_test: test_param = "long"; break;
		case TestType::conveyance: test_param = "conveyance"; break;
		// no default - this way we get warned by compiler if we're not listing all of them.
	}
	if (test_param.empty())
		return _("Invalid test specified");

	std::string output;
	std::string error_msg = drive_->execute_device_smartctl("--test=" + test_param, smartctl_ex, output);

	if (!error_msg.empty())  // checks for empty output too
		return error_msg;

	if (!app_pcre_match(R"(/^Drive command .* successful\.\nTesting has begun\.$/mi)", output)) {
		return _("Sending command to drive failed.");
	}


	// update our members
// 	error_message = this->update(smartctl_ex);
// 	if (!error_message.empty())  // update can error out too.
// 		return error_message;

	// Don't update here - the logs may not be updated this fast.
	// Better to wait several seconds and then call it manually.

	// Set up everything so that the caller won't have to.

	status_ = AtaStorageSelftestEntry::Status::in_progress;

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
std::string SelfTest::force_stop(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (!drive_)
		return "[internal error: drive must not be NULL]";
	if (!drive_->get_test_is_active())
		return _("No test is currently running on this drive.");

	// To abort immediate offline test, the device MUST have
	// "Abort Offline collection upon new command" capability,
	// any command (e.g. "--abort") will abort it. If it has "Suspend Offline...",
	// there's no way to abort such test.
	if (type_ == TestType::immediate_offline) {
		AtaStorageProperty p = drive_->lookup_property(
				"ata_smart_data/capabilities/offline_is_aborted_upon_new_cmd", AtaStorageProperty::Section::internal);
		if (!p.empty() && p.get_value<bool>()) {  // if empty, give a chance to abort anyway.
			return _("Aborting this test is unsupported by the drive.");
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
		return _("Sending command to drive failed.");
	}

	// update our members
	error_msg = this->update(smartctl_ex);

	// the thing is, update() may fail to actually update the statuses, so
	// do it manually.
	if (status_ == AtaStorageSelftestEntry::Status::in_progress) {  // update() couldn't do its job
		status_ = AtaStorageSelftestEntry::Status::aborted_by_host;
		remaining_percent_ = -1;
		last_seen_percent_ = -1;
		poll_in_seconds_ = std::chrono::seconds(-1);
		timer_.stop();
		drive_->set_test_is_active(false);
	}

	if (!error_msg.empty())  // update can error out too.
		return error_msg;
	return {};  // everything ok
}



// update status variables. note: the returned error is an error in logic,
// not an hw defect error.
std::string SelfTest::update(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	using namespace std::literals;

	if (!drive_)
		return "[internal error: drive must not be NULL]";

	std::string output;
// 	std::string error_message = drive_->execute_device_smartctl("--log=selftest", smartctl_ex, output);
	std::string error_msg = drive_->execute_device_smartctl("--capabilities", smartctl_ex, output);

	if (!error_msg.empty())  // checks for empty output too
		return error_msg;

	AtaStorageAttribute::DiskType disk_type = drive_->get_is_hdd() ? AtaStorageAttribute::DiskType::Hdd : AtaStorageAttribute::DiskType::Ssd;
	auto parser = SmartctlParser::create(SmartctlOutputParserType::Text);
	DBG_ASSERT_RETURN(parser, "Cannot create parser");

	if (!parser->parse_full(output)) {  // try to parse it
		return parser->get_error_msg();
	}
	auto properties = StoragePropertyProcessor::process_properties(parser->get_properties(), disk_type);

	// Note: Since the self-test log is sometimes late
	// and in undetermined order (sorting by hours is too rough),
	// we use the "self-test status" capability.
	AtaStorageProperty p;
	for (const auto& e : properties) {
// 		if (e.section != AtaStorageProperty::Section::data || e.subsection != AtaStorageProperty::SubSection::selftest_log
		if (e.section != AtaStorageProperty::Section::internal
				|| !e.is_value_type<AtaStorageSelftestEntry>() || e.get_value<AtaStorageSelftestEntry>().test_num != 0
				|| e.generic_name != "ata_smart_data/self_test/status/passed")
			continue;
		p = e;
	}

	if (p.empty())
		return _("The drive doesn't report the test status.");

	status_ = p.get_value<AtaStorageSelftestEntry>().status;
	bool active = (status_ == AtaStorageSelftestEntry::Status::in_progress);


	// Note that the test needs 90% to complete, not 100. It starts at 90%
	// and reaches 00% on completion. That's 9 pieces.
	if (active) {

		remaining_percent_ = p.get_value<AtaStorageSelftestEntry>().remaining_percent;
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
			if (type_ == TestType::long_test && remaining_percent_ == 10)
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

	return {};  // everything ok
}






/// @}
