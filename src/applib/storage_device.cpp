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
#include <unordered_map>

#include "rconfig/rconfig.h"
#include "hz/string_algo.h"  // string_trim_copy, string_any_to_unix_copy
#include "hz/fs.h"
#include "hz/format_unit.h"  // hz::format_date

#include "app_pcrecpp.h"
#include "storage_device.h"
#include "smartctl_text_ata_parser.h"
#include "storage_settings.h"
#include "smartctl_executor.h"
#include "smartctl_version_parser.h"
//#include "smartctl_text_parser_helper.h"
#include "ata_storage_property_descr.h"



std::string StorageDevice::get_type_storable_name(DetectedType type)
{
	static const std::unordered_map<DetectedType, std::string> m {
			{DetectedType::Unknown,           "unknown"},
			{DetectedType::NeedsExplicitType, "invalid"},
			{DetectedType::CdDvd,             "cd/dvd"},
			{DetectedType::Raid,              "raid"},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



std::string StorageDevice::get_status_displayable_name(Status status)
{
	static const std::unordered_map<Status, std::string> m {
			{Status::Enabled,     C_("status", "Enabled")},
			{Status::Disabled,    C_("status", "Disabled")},
			{Status::Unsupported, C_("status", "Unsupported")},
			{Status::Unknown,     C_("status", "Unknown")},
	};
	if (auto iter = m.find(status); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



StorageDevice::StorageDevice(std::string dev_or_vfile, bool is_virtual)
		: is_virtual_(is_virtual)
{
	if (is_virtual_) {
		virtual_file_ = hz::fs_path_from_string(dev_or_vfile);
	} else {
		device_ = std::move(dev_or_vfile);
	}
}



StorageDevice::StorageDevice(std::string dev, std::string type_arg)
		: device_(std::move(dev)), type_arg_(std::move(type_arg))
{ }



void StorageDevice::clear_fetched(bool including_outputs)
{
	if (including_outputs) {
		basic_output_.clear();
		full_output_.clear();
	}

	parse_status_ = ParseStatus::None;
	test_is_active_ = false;  // not sure

	smart_supported_.reset();
	smart_enabled_.reset();
	model_name_.reset();
	aodc_status_.reset();
	family_name_.reset();
	size_.reset();
	health_property_.reset();

	property_repository_.clear();
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::fetch_basic_data_and_parse(
		const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_) {
		return hz::Unexpected(StorageDeviceError::TestRunning, _("A test is currently being performed on this drive."));
	}

	this->clear_fetched();  // clear everything fetched before, including outputs

	// We don't use "--all" - it may cause really screwed up the output (tests, etc.).
	// This looks just like "--info" only on non-smart devices.
	const auto default_parser_type = SmartctlVersionParser::get_default_format(SmartctlParserType::Basic);
	std::string command_options = "--info --health --capabilities";
	if (default_parser_type == SmartctlOutputFormat::Json) {
		// --json flags: o means include original output (just in case).
		command_options += " --json=o";
	}

	auto execute_status = execute_device_smartctl(command_options, smartctl_ex, this->basic_output_, true);  // set type to invalid if needed

	// Smartctl 5.39 cvs/svn version defaults to usb type on at least linux and windows.
	// This means that the old SCSI identify command isn't executed by default,
	// and there is no information about the device manufacturer/etc. in the output.
	// We detect this and set the device type to scsi to at least have _some_ info.
	if ((execute_status || execute_status.error().data() == StorageDeviceError::ExecutionError)
		&& get_detected_type() == DetectedType::NeedsExplicitType && get_type_argument().empty()) {
		debug_out_info("app", "The device seems to be of different type than auto-detected, trying again with scsi.\n");
		this->set_type_argument("scsi");
		return this->fetch_basic_data_and_parse(smartctl_ex);  // try again with scsi
	}

	// Since the type error leads to "command line didn't parse" error here,
	// we do this after the scsi stuff.
//	if (!execute_status.has_value()) {
		// Still try to parse something. For some reason, running smartctl on usb flash drive
		// under winxp returns "command line didn't parse", while actually printing its name.
//		this->parse_basic_data(false, true);
//		return execute_status;
//	}

	// Set some properties too - they are needed for e.g. AODC status, etc.
	return this->parse_basic_data(true);
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::parse_basic_data(bool do_set_properties, bool emit_signal)
{
	this->clear_fetched(false);  // clear everything fetched before, except outputs

	AtaStorageAttribute::DiskType disk_type = AtaStorageAttribute::DiskType::Any;

	// Try the basic parser first. If it succeeds, use the specialized parser.
	auto basic_parser = SmartctlParser::create(SmartctlParserType::Basic, SmartctlOutputFormat::Json);
	DBG_ASSERT_RETURN(basic_parser, hz::Unexpected(StorageDeviceError::ParseError, _("Cannot create parser")));

	auto parse_status = basic_parser->parse(this->get_basic_output());
	if (!parse_status) {
		return hz::Unexpected(StorageDeviceError::ParseError,
				std::vformat(_("Cannot parse smartctl output: {}"), std::make_format_args(parse_status.error().message())));
	}

	auto basic_property_repo = basic_parser->get_property_repository();

	auto drive_type_prop = basic_property_repo.lookup_property("_custom/drive_type");
	if (!drive_type_prop.empty()) {
		const auto& drive_type = drive_type_prop.get_value<std::string>();
		if (drive_type == "CD/DVD") {
			debug_out_dump("app", "Drive " << get_device_with_type() << " seems to be a CD/DVD device.\n");
			this->set_detected_type(DetectedType::CdDvd);
		} else if (drive_type == "RAID") {
			debug_out_dump("app", "Drive " << get_device_with_type() << " seems to be a RAID volume/controller.\n");
			this->set_detected_type(DetectedType::Raid);
		}
	}

	{
		auto rpm_prop = basic_property_repo.lookup_property("rotation_rate");
		if (!rpm_prop.empty()) {
			auto rpm = rpm_prop.get_value<int64_t>();
			this->hdd_ = rpm > 0;
		}
		if (hdd_.has_value()) {
			disk_type = hdd_.value() ? AtaStorageAttribute::DiskType::Hdd : AtaStorageAttribute::DiskType::Ssd;
		}
	}

	if (auto prop = basic_property_repo.lookup_property("smart_support/available"); !prop.empty()) {
		smart_supported_ = prop.get_value<bool>();
	}
	if (auto prop = basic_property_repo.lookup_property("smart_support/enabled"); !prop.empty()) {
		smart_enabled_ = prop.get_value<bool>();
	}
	if (auto prop = basic_property_repo.lookup_property("model_name"); !prop.empty()) {
		model_name_ = prop.get_value<std::string>();
	}
	if (auto prop = basic_property_repo.lookup_property("model_family"); !prop.empty()) {
		family_name_ = prop.get_value<std::string>();
	}
	if (auto prop = basic_property_repo.lookup_property("serial_number"); !prop.empty()) {
		serial_number_ = prop.get_value<std::string>();
	}
	if (auto prop = basic_property_repo.lookup_property("user_capacity/bytes/_short"); !prop.empty()) {
		size_ = prop.readable_value;
	} else if (auto prop2 = basic_property_repo.lookup_property("user_capacity/bytes"); !prop.empty()) {
		size_ = prop2.readable_value;
	}


	// Try to parse the properties. ignore its errors - we already got what we came for.
	// Note that this may try to parse data the second time (it may already have
	// been parsed by parse_data() which failed at it).
//	if (do_set_properties) {
//		auto parser = SmartctlParser::create(SmartctlParserType::Ata, SmartctlOutputFormat::Json);
//		DBG_ASSERT_RETURN(parser, hz::Unexpected(StorageDeviceError::ParseError, _("Cannot create parser")));
//
//		if (parser->parse(this->basic_output_)) {  // try to parse it
//			this->set_property_repository(
//					StoragePropertyProcessor::process_properties(parser->get_property_repository(), disk_type));  // copy to our drive, overwriting old data
//		}
//	}
	if (do_set_properties) {
		this->set_property_repository(StoragePropertyProcessor::process_properties(basic_property_repo, disk_type));
	}

	// A model field (and its aliases) is a good indication whether there was any data or not
	set_parse_status(model_name_.has_value() ? ParseStatus::Basic : ParseStatus::None);

	if (emit_signal)
		signal_changed().emit(this);  // notify listeners

	return {};
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::fetch_full_data_and_parse(
		const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_) {
		return hz::Unexpected(StorageDeviceError::TestRunning, _("A test is currently being performed on this drive."));
	}

	this->clear_fetched();  // clear everything fetched before, including outputs

	std::string output;
	hz::ExpectedVoid<StorageDeviceError> execute_status;

	// instead of -x, we use all the individual options -x encompasses, so that
	// an addition to default -x output won't affect us.
	if (this->get_type_argument() == "scsi") {  // not sure about correctness... FIXME probably fails with RAID/scsi
		const auto default_parser_type = SmartctlVersionParser::get_default_format(SmartctlParserType::Basic);
		// This doesn't do much yet, but just in case...
		// SCSI equivalent of -x:
		std::string command_options = "--health --info --attributes --log=error --log=selftest --log=background --log=sasphy";
		if (default_parser_type == SmartctlOutputFormat::Json) {
			// --json flags: o means include original output (just in case).
			command_options += " --json=o";
		}

		execute_status = execute_device_smartctl(command_options, smartctl_ex, output);

	} else {
		const auto default_parser_type = SmartctlVersionParser::get_default_format(SmartctlParserType::Ata);
		// ATA equivalent of -x.
		std::string command_options = "--health --info --get=all --capabilities --attributes --format=brief --log=xerror,50,error --log=xselftest,50,selftest --log=selective --log=directory --log=scttemp --log=scterc --log=devstat --log=sataphy";
		if (default_parser_type == SmartctlOutputFormat::Json) {
			// --json flags: o means include original output (just in case).
			command_options += " --json=o";
		}

		execute_status = execute_device_smartctl(command_options, smartctl_ex, output, true);  // set type to invalid if needed
	}

	// See notes above (in fetch_basic_data_and_parse()).
	if ((execute_status || execute_status.error().data() == StorageDeviceError::ExecutionError)
		&& get_detected_type() == DetectedType::NeedsExplicitType && get_type_argument().empty()) {
		debug_out_info("app", "The device seems to be of different type than auto-detected, trying again with scsi.\n");
		this->set_type_argument("scsi");
		return this->fetch_full_data_and_parse(smartctl_ex);  // try again with scsi
	}

	// Since the type error leads to "command line didn't parse" error here,
	// we do this after the scsi stuff.
	if (!execute_status)
		return execute_status;

	this->full_output_ = output;
	return this->try_parse_data();
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::try_parse_data()
{
	this->clear_fetched(false);  // clear everything fetched before, except outputs

	AtaStorageAttribute::DiskType disk_type = AtaStorageAttribute::DiskType::Any;
	if (hdd_.has_value()) {
		disk_type = hdd_.value() ? AtaStorageAttribute::DiskType::Hdd : AtaStorageAttribute::DiskType::Ssd;
	}

	auto parser_format = SmartctlParser::detect_output_format(this->full_output_);

	if (!parser_format.has_value()) {
		return hz::Unexpected(StorageDeviceError::ParseError, parser_format.error().message());
	}

	// TODO Choose format according to device type
	SmartctlParserType parser_type = SmartctlParserType::Ata;

	auto parser = SmartctlParser::create(parser_type, parser_format.value());
	DBG_ASSERT_RETURN(parser, hz::Unexpected(StorageDeviceError::ParseError, _("Cannot create parser")));

	// Try to parse it (parse only, set the properties after basic parsing).
	const auto parse_status = parser->parse(this->full_output_);
	if (parse_status.has_value()) {

		// refresh basic info too
		this->basic_output_ = this->full_output_;  // put data including version information

		// note: this will clear the non-basic properties!
		// this will parse some info that is already parsed by SmartctlAtaTextParser::parse(),
		// but this one sets the StorageDevice class members, not properties.
		static_cast<void>(this->parse_basic_data(false, false));  // don't emit signal, we're not complete yet.

		// Call this after parse_basic_data(), since it sets parse status to "info".
		this->set_parse_status(StorageDevice::ParseStatus::Full);

		// set the full properties.
		// copy to our drive, overwriting old data.
		this->set_property_repository(StoragePropertyProcessor::process_properties(parser->get_property_repository(), disk_type));

		signal_changed().emit(this);  // notify listeners

		return {};
	}

	// Don't show any GUI warnings on parse failure - it may just be an unsupported
	// drive (e.g. usb flash disk). Plus, it may flood the string. The data will be
	// parsed again in Info window, and we show the warnings there.
	debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse smartctl output.\n");

	// proper parsing failed. try to at least extract info section
	this->basic_output_ = this->full_output_;  // complete output here. sometimes it's only the info section
	auto basic_parse_status = this->parse_basic_data(true);  // will add some properties too. this will emit signal_changed().
	if (!basic_parse_status) {
		// return full parser's error messages - they are more detailed.
		return hz::Unexpected(StorageDeviceError::ParseError,
				std::vformat(_("Cannot parse smartctl output: {}"), std::make_format_args(parse_status.error().message())));
	}

	return {};  // return ok if at least the info was ok.
}



StorageDevice::ParseStatus StorageDevice::get_parse_status() const
{
	return parse_status_;
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::set_smart_enabled(bool b,
		const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_) {
		return hz::Unexpected(StorageDeviceError::TestRunning, _("A test is currently being performed on this drive."));
	}

	// execute smartctl --smart=on|off /dev/...
	// --saveauto=on is also executed when enabling smart.

	// Output:
/*
=== START OF ENABLE/DISABLE COMMANDS SECTION ===
SMART Enabled.
SMART Attribute Autosave Enabled.
--------------------------- OR ---------------------------
=== START OF ENABLE/DISABLE COMMANDS SECTION ===
SMART Disabled. Use option -s with argument 'on' to enable it.
--------------------------- OR ---------------------------
A mandatory SMART command failed: exiting. To continue, add one or more '-T permissive' options.
*/

	std::string output;
	auto status = execute_device_smartctl((b ? "--smart=on --saveauto=on" : "--smart=off"), smartctl_ex, output);
	if (!status) {
		return status;
	}

	// search at line start, because they are sometimes present in other sentences too.
	if (app_pcre_match("/^SMART Enabled/mi", output) || app_pcre_match("/^SMART Disabled/mi", output)) {
		return {};  // success
	}

	if (app_pcre_match("/^A mandatory SMART command failed/mi", output)) {
		return hz::Unexpected(StorageDeviceError::CommandFailed, _("Mandatory SMART command failed."));
	}

	return hz::Unexpected(StorageDeviceError::CommandUnknownError, _("Unknown error occurred."));
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::set_aodc_enabled(bool b, const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_) {
		return hz::Unexpected(StorageDeviceError::TestRunning, _("A test is currently being performed on this drive."));
	}

	// execute smartctl --offlineauto=on|off /dev/...
	// Output:
/*
=== START OF ENABLE/DISABLE COMMANDS SECTION ===
SMART Automatic Offline Testing Enabled every four hours.
--------------------------- OR ---------------------------
=== START OF ENABLE/DISABLE COMMANDS SECTION ===
SMART Automatic Offline Testing Disabled.
--------------------------- OR ---------------------------
A mandatory SMART command failed: exiting. To continue, add one or more '-T permissive' options.
*/
	std::string output;
	auto status = execute_device_smartctl((b ? "--offlineauto=on" : "--offlineauto=off"), smartctl_ex, output);
	if (!status) {
		return status;
	}

	if (app_pcre_match("/Testing Enabled/mi", output) || app_pcre_match("/Testing Disabled/mi", output)) {
		return {};  // success
	}

	if (app_pcre_match("/^A mandatory SMART command failed/mi", output)) {
		return hz::Unexpected(StorageDeviceError::CommandFailed, _("Mandatory SMART command failed."));
	}

	return hz::Unexpected(StorageDeviceError::CommandUnknownError, _("Unknown error occurred."));
}



StorageDevice::Status StorageDevice::get_smart_status() const
{
	Status status = Status::Unsupported;
	if (smart_enabled_.has_value()) {
		if (smart_enabled_.value()) {  // enabled, supported
			status = Status::Enabled;
		} else {  // if it's disabled, maybe it's unsupported, check that:
			if (smart_supported_.has_value()) {
				if (smart_supported_.value()) {  // disabled, supported
					status = Status::Disabled;
				} else {  // disabled, unsupported
					status = Status::Unsupported;
				}
			} else {  // disabled, support unknown
				status = Status::Disabled;
			}
		}
	} else {  // status unknown
		if (smart_supported_.has_value()) {
			if (smart_supported_.value()) {  // status unknown, supported
				status = Status::Disabled;  // at least give the user a chance to try enabling it
			} else {  // status unknown, unsupported
				status = Status::Unsupported;  // most likely
			}
		} else {  // status unknown, support unknown
			status = Status::Unsupported;
		}
	}
	return status;
}



StorageDevice::Status StorageDevice::get_aodc_status() const
{
	// smart-disabled drives are known to print some garbage, so
	// let's protect us from it.
	if (get_smart_status() != Status::Enabled)
		return Status::Unsupported;

	if (aodc_status_.has_value())  // cached return value
		return aodc_status_.value();

	Status status = Status::Unknown;  // for now

	bool aodc_supported = false;
	int found = 0;

	for (const auto& p : property_repository_.get_properties()) {
//		if (p.section == AtaStorageProperty::Section::Internal) {
			if (p.generic_name == "ata_smart_data/offline_data_collection/status/value/_parsed") {  // if this is not present at all, we set the unknown status.
				status = (p.get_value<bool>() ? Status::Enabled : Status::Disabled);
				//++found;
				continue;
			}
			if (p.generic_name == "_text_only/aodc_support") {
				aodc_supported = p.get_value<bool>();
				++found;
				continue;
			}
			if (found >= 2)
				break;
//		}
	}

	if (!aodc_supported)
		status = Status::Unsupported;
	// if it's supported, then status may be enabled, disabled or unknown.

	aodc_status_ = status;  // store to cache

	debug_out_info("app", DBG_FUNC_MSG << "AODC status: " << get_status_displayable_name(status) << "\n");

	return status;
}



std::string StorageDevice::get_device_size_str() const
{
	return (size_.has_value() ? size_.value() : "");
}



AtaStorageProperty StorageDevice::get_health_property() const
{
	if (health_property_.has_value())  // cached return value
		return health_property_.value();

	AtaStorageProperty p = property_repository_.lookup_property("smart_status/passed",
			AtaStorageProperty::Section::Health);
	if (!p.empty())
		health_property_ = p;  // store to cache

	return p;
}



std::string StorageDevice::get_device() const
{
	return device_;
}



std::string StorageDevice::get_device_base() const
{
	if (is_virtual_)
		return "";

	const std::string::size_type pos = device_.rfind('/');  // find basename
	if (pos == std::string::npos)
		return device_;  // fall back
	return device_.substr(pos+1, std::string::npos);
}



std::string StorageDevice::get_device_with_type() const
{
	if (this->get_is_virtual()) {
		const std::string vf = this->get_virtual_filename();
		/// Translators: %1 is filename
		std::string ret = Glib::ustring::compose(C_("filename", "Virtual (%1)"), (vf.empty() ? (std::string("[") + C_("filename", "empty") + "]") : vf));
		return ret;
	}
	std::string device = get_device();
	if (!get_type_argument().empty()) {
		device = Glib::ustring::compose(_("%1 (%2)"), device, get_type_argument());
	}
	return device;
}



void StorageDevice::set_detected_type(DetectedType t)
{
	detected_type_ = t;
}



StorageDevice::DetectedType StorageDevice::get_detected_type() const
{
	return detected_type_;
}



void StorageDevice::set_type_argument(std::string arg)
{
	type_arg_ = std::move(arg);
}



std::string StorageDevice::get_type_argument() const
{
	return type_arg_;
}



void StorageDevice::set_extra_arguments(std::string args)
{
	extra_args_ = std::move(args);
}



std::string StorageDevice::get_extra_arguments() const
{
	return extra_args_;
}



void StorageDevice::set_drive_letters(std::map<char, std::string> letters)
{
	drive_letters_ = std::move(letters);
}



const std::map<char, std::string>& StorageDevice::get_drive_letters() const
{
	return drive_letters_;
}



std::string StorageDevice::format_drive_letters(bool with_volnames) const
{
	std::vector<std::string> drive_letters_decorated;
	for (const auto& iter : drive_letters_) {
		drive_letters_decorated.push_back(std::string() + (char)std::toupper(iter.first) + ":");
		if (with_volnames && !iter.second.empty()) {
			// e.g. "C: (Local Drive)"
			drive_letters_decorated.back() = Glib::ustring::compose(_("%1 (%2)"), drive_letters_decorated.back(), iter.second);
		}
	}
	return hz::string_join(drive_letters_decorated, ", ");
}



bool StorageDevice::get_is_virtual() const
{
	return is_virtual_;
}



hz::fs::path StorageDevice::get_virtual_file() const
{
	return (is_virtual_ ? virtual_file_ : hz::fs::path());
}



std::string StorageDevice::get_virtual_filename() const
{
	return (is_virtual_ ? hz::fs_path_to_string(virtual_file_.filename()) : std::string());
}



const StoragePropertyRepository& StorageDevice::get_property_repository() const
{
	return property_repository_;
}



std::string StorageDevice::get_model_name() const
{
	return (model_name_.has_value() ? model_name_.value() : "");
}



std::string StorageDevice::get_family_name() const
{
	return (family_name_.has_value() ? family_name_.value() : "");
}



std::string StorageDevice::get_serial_number() const
{
	return (serial_number_.has_value() ? serial_number_.value() : "");
}



bool StorageDevice::get_is_hdd() const
{
	return hdd_.has_value() && hdd_.value();
}



void StorageDevice::set_info_output(std::string s)
{
	basic_output_ = std::move(s);
}



std::string StorageDevice::get_basic_output() const
{
	return basic_output_;
}



void StorageDevice::set_full_output(std::string s)
{
	full_output_ = std::move(s);
}



std::string StorageDevice::get_full_output() const
{
	return full_output_;
}



void StorageDevice::set_is_manually_added(bool b)
{
	is_manually_added_ = b;
}



bool StorageDevice::get_is_manually_added() const
{
	return is_manually_added_;
}



void StorageDevice::set_test_is_active(bool b)
{
	const bool changed = (test_is_active_ != b);
	test_is_active_ = b;
	if (changed) {
		signal_changed().emit(this);  // so that everybody stops any test-aborting operations.
	}
}



bool StorageDevice::get_test_is_active() const
{
	return test_is_active_;
}



std::string StorageDevice::get_save_filename() const
{
	const std::string model = this->get_model_name();  // may be empty
	const std::string serial = this->get_serial_number();
	const std::string date = hz::format_date("%Y-%m-%d_%H%M", true);

	auto filename_format = rconfig::get_data<std::string>("gui/smartctl_output_filename_format");
	hz::string_replace(filename_format, "{serial}", serial);
	hz::string_replace(filename_format, "{model}", model);
	hz::string_replace(filename_format, "{date}", date);

	return hz::fs_filename_make_safe(filename_format);
}



std::string StorageDevice::get_device_options() const
{
	if (is_virtual_) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot get device options of a virtual device.\n");
		return {};
	}

	// If we have some special type or option, specify it on the command line (like "-d scsi").
	// Note that the latter "-d" option overrides the former.

	// lowest priority - the detected type
	std::vector<std::string> args;
	if (!get_type_argument().empty()) {
		args.push_back("-d " + get_type_argument());
	}
	// extra args, as specified manually in CLI or when adding the drive
	if (!get_extra_arguments().empty()) {
		args.push_back(get_extra_arguments());
	}

	// config options, as specified in preferences.
	std::string config_options = app_get_device_option(get_device(), get_type_argument());
	if (!config_options.empty()) {
		args.push_back(config_options);
	}

	return hz::string_join(args, " ");
}



hz::ExpectedVoid<StorageDeviceError> StorageDevice::execute_device_smartctl(const std::string& command_options,
		const std::shared_ptr<CommandExecutor>& smartctl_ex, std::string& smartctl_output, bool check_type)
{
	// don't forbid running on currently tested drive - we need to call this from the test code.

	if (is_virtual_) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot execute smartctl on a virtual device.\n");
		return hz::Unexpected(StorageDeviceError::CannotExecuteOnVirtual, _("Cannot execute smartctl on a virtual device."));
	}

	std::string device = get_device();

	auto smartctl_status = execute_smartctl(device, this->get_device_options(),
			command_options, smartctl_ex, smartctl_output);

	if (!smartctl_status) {
		debug_out_warn("app", DBG_FUNC_MSG << "Smartctl binary did not execute cleanly.\n");

		// Smartctl 5.39 cvs/svn version defaults to usb type on at least linux and windows.
		// This means that the old SCSI identify command isn't executed by default,
		// and there is no information about the device manufacturer/etc. in the output.
		// We detect this and set the device type to scsi to at least have _some_ info.
		if (check_type && this->get_detected_type() == DetectedType::Unknown
				&& app_pcre_match("/specify device type with the -d option/mi", smartctl_output)) {
			this->set_detected_type(DetectedType::NeedsExplicitType);
		}

		return hz::Unexpected(StorageDeviceError::ExecutionError, smartctl_status.error().message());
	}

	return {};
}



sigc::signal<void, StorageDevice*>& StorageDevice::signal_changed()
{
	return signal_changed_;
}



void StorageDevice::set_parse_status(ParseStatus value)
{
	parse_status_ = value;
}


void StorageDevice::set_property_repository(StoragePropertyRepository repository)
{
	property_repository_ = std::move(repository);
}







/// @}
