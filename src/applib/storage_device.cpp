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
#include "smartctl_ata_text_parser.h"
#include "storage_settings.h"
#include "smartctl_executor.h"




std::string StorageDevice::get_type_storable_name(DetectedType type)
{
	static const std::unordered_map<DetectedType, std::string> m {
			{DetectedType::unknown, "unknown"},
			{DetectedType::invalid, "invalid"},
			{DetectedType::cddvd, "cd/dvd"},
			{DetectedType::raid, "raid"},
	};
	if (auto iter = m.find(type); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



std::string StorageDevice::get_status_displayable_name(Status status)
{
	static const std::unordered_map<Status, std::string> m {
			{Status::enabled, C_("status", "Enabled")},
			{Status::disabled, C_("status", "Disabled")},
			{Status::unsupported, C_("status", "Unsupported")},
			{Status::unknown, C_("status", "Unknown")},
	};
	if (auto iter = m.find(status); iter != m.end()) {
		return iter->second;
	}
	return "[internal_error]";
}



StorageDevice::StorageDevice(std::string dev_or_vfile, bool is_virtual)
{
	is_virtual_ = is_virtual;

	if (is_virtual) {
		virtual_file_ = hz::fs::u8path(dev_or_vfile);
	} else {
		device_ = std::move(dev_or_vfile);
	}
}



StorageDevice::StorageDevice(std::string dev, std::string type_arg)
		: device_(std::move(dev)), type_arg_(std::move(type_arg))
{ }



void StorageDevice::clear_fetched(bool including_outputs) {
	if (including_outputs) {
		info_output_.clear();
		full_output_.clear();
	}

	parse_status_ = ParseStatus::none;
	test_is_active_ = false;  // not sure

	smart_supported_.reset();
	smart_enabled_.reset();
	model_name_.reset();
	aodc_status_.reset();
	family_name_.reset();
	size_.reset();
	health_property_.reset();

	properties_.clear();
}



std::string StorageDevice::fetch_basic_data_and_parse(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_)
		return _("A test is currently being performed on this drive.");

	this->clear_fetched();  // clear everything fetched before, including outputs

	// We don't use "--all" - it may cause really screwed up the output (tests, etc...).
	// This looks just like "--info" only on non-smart devices.
	std::string error_msg = execute_device_smartctl("--info --health --capabilities", smartctl_ex, this->info_output_, true);  // set type to invalid if needed

	// Smartctl 5.39 cvs/svn version defaults to usb type on at least linux and windows.
	// This means that the old SCSI identify command isn't executed by default,
	// and there is no information about the device manufacturer/etc... in the output.
	// We detect this and set the device type to scsi to at least have _some_ info.
	if (get_detected_type() == DetectedType::invalid && get_type_argument().empty()) {
		debug_out_info("app", "The device seems to be of different type than auto-detected, trying again with scsi.\n");
		this->set_type_argument("scsi");
		return this->fetch_basic_data_and_parse(smartctl_ex);  // try again with scsi
	}

	// Since the type error leads to "command line didn't parse" error here,
	// we do this after the scsi stuff.
	if (!error_msg.empty()) {
		// Still try to parse something. For some reason, running smartctl on usb flash drive
		// under winxp returns "command line didn't parse", while actually printing its name.
		this->parse_basic_data(false, true);
		return error_msg;
	}

	// Set some properties too - they are needed for e.g. AODC status, etc...
	return this->parse_basic_data(true);
}



std::string StorageDevice::parse_basic_data(bool do_set_properties, bool emit_signal)
{
	this->clear_fetched(false);  // clear everything fetched before, except outputs

	if (this->info_output_.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "String to parse is empty.\n");
		return _("Cannot read information from an empty string.");
	}

	std::string version, version_full;
	if (!SmartctlAtaTextParser::parse_version(this->info_output_, version, version_full))  // is this smartctl data at all?
		return _("Cannot get smartctl version information.");

	// Detect type. note: we can't distinguish between sata and scsi (on linux, for -d ata switch).
	// Sample output line 1 (encountered on a CDRW drive):
	// SMART support is: Unavailable - Packet Interface Devices [this device: CD/DVD] don't support ATA SMART
	// Sample output line 2 (encountered on a BDRW drive):
	// Device type:          CD/DVD
	if (app_pcre_match("/this device: CD\\/DVD/mi", info_output_) || app_pcre_match("/^Device type:\\s+CD\\/DVD/mi", info_output_)) {
		debug_out_dump("app", "Drive " << get_device_with_type() << " seems to be a CD/DVD device.\n");
		this->set_detected_type(DetectedType::cddvd);

	// This was encountered on a csmi soft-raid under windows with pd0.
	// The device reported that it had smart supported and enabled.
	// Product:              Raid 5 Volume
	} else if (app_pcre_match("/Product:[ \\t]*Raid/mi", info_output_)) {
		debug_out_dump("app", "Drive " << get_device_with_type() << " seems to be a RAID volume/controller.\n");
		this->set_detected_type(DetectedType::raid);
	}

	// RAID volume may report that it has SMART, but it obviously doesn't.
	if (get_detected_type() == DetectedType::raid) {
		smart_supported_ = false;
		smart_enabled_ = false;

	} else {
		// Note: We don't use SmartctlAtaTextParser here, because this information
		// may be in some other format. If this information is valid, only then it's
		// passed to SmartctlAtaTextParser.
		// Compared to SmartctlAtaTextParser, this one is much looser.

		// Don't put complete messages here - they change across smartctl versions.
		if (app_pcre_match("/^SMART support is:[ \\t]*Unavailable/mi", info_output_)  // cdroms output this
				|| app_pcre_match("/Device does not support SMART/mi", info_output_)  // usb flash drives, non-smart hds
				|| app_pcre_match("/Device Read Identity Failed/mi", info_output_)) {  // solaris scsi, unsupported by smartctl (maybe others?)
			smart_supported_ = false;
			smart_enabled_ = false;

		} else if (app_pcre_match("/^SMART support is:[ \\t]*Available/mi", info_output_)
				|| app_pcre_match("/^SMART support is:[ \\t]*Ambiguous/mi", info_output_)) {
			smart_supported_ = true;

			if (app_pcre_match("/^SMART support is:[ \\t]*Enabled/mi", info_output_)) {
				smart_enabled_ = true;
			} else if (app_pcre_match("/^SMART support is:[ \\t]*Disabled/mi", info_output_)) {
				smart_enabled_ = false;
			}
		}
	}

	std::string model;
	if (app_pcre_match("/^Device Model:[ \\t]*(.*)$/mi", info_output_, &model)) {  // HD's and cdroms
		model_name_ = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(model), ' ');

	} else if (app_pcre_match("/^(?:Device|Product):[ \\t]*(.*)$/mi", info_output_, &model)) {  // usb flash drives
		model_name_ = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(model), ' ');
	}


	std::string family;  // this is from smartctl's database
	if (app_pcre_match("/^Model Family:[ \\t]*(.*)$/mi", info_output_, &family)) {
		family_name_ = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(family), ' ');
	}

	std::string serial;
	if (app_pcre_match("/^Serial Number:[ \\t]*(.*)$/mi", info_output_, &serial)) {
		serial_number_ = hz::string_remove_adjacent_duplicates_copy(hz::string_trim_copy(serial), ' ');
	}

	std::string rpm_str;
	if (app_pcre_match("/^Rotation Rate:[ \\t]*(.*)$/mi", info_output_, &rpm_str)) {
		int rpm = hz::string_to_number_nolocale<int>(rpm_str, false);
		hdd_ = rpm > 0;
	}


	// Note: this property is present since 5.33.
	std::string size;
	if (app_pcre_match("/^User Capacity:[ \\t]*(.*)$/mi", info_output_, &size)) {
		int64_t bytes = 0;
		size_ = SmartctlAtaTextParser::parse_byte_size(size, bytes, false);
	}


	// Try to parse the properties. ignore its errors - we already got what we came for.
	// Note that this may try to parse data the second time (it may already have
	// been parsed by parse_data() which failed at it).
	if (do_set_properties) {
		AtaStorageAttribute::DiskType disk_type = AtaStorageAttribute::DiskType::Any;
		if (hdd_.has_value()) {
			disk_type = hdd_.value() ? AtaStorageAttribute::DiskType::Hdd : AtaStorageAttribute::DiskType::Ssd;
		}
		SmartctlAtaTextParser ps;
		if (ps.parse_full(this->info_output_, disk_type)) {  // try to parse it
			this->set_properties(ps.get_properties());  // copy to our drive, overwriting old data
		}
	}

	// A model field (and its aliases) is a good indication whether there was any data or not
	set_parse_status(model_name_.has_value() ? ParseStatus::info : ParseStatus::none);

	if (emit_signal)
		signal_changed().emit(this);  // notify listeners

	return std::string();
}



std::string StorageDevice::fetch_data_and_parse(const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_)
		return _("A test is currently being performed on this drive.");

	this->clear_fetched();  // clear everything fetched before, including outputs

	std::string output;
	std::string error_msg;

	// instead of -x, we use all the individual options -x encompasses, so that
	// an addition to default -x output won't affect us.
	if (this->get_type_argument() == "scsi") {  // not sure about correctness... FIXME probably fails with RAID/scsi
		// This doesn't do much yet, but just in case...
		// SCSI equivalent of -x:
		error_msg = execute_device_smartctl("--health --info --attributes --log=error --log=selftest --log=background --log=sasphy", smartctl_ex, output);
	} else {
		// ATA equivalent of -x:
		error_msg = execute_device_smartctl("--health --info --get=all --capabilities --attributes --format=brief --log=xerror,50,error --log=xselftest,50,selftest --log=selective --log=directory --log=scttemp --log=scterc --log=devstat --log=sataphy",
				smartctl_ex, output, true);  // set type to invalid if needed
	}
	// See notes above (in fetch_basic_data_and_parse()).
	if (get_detected_type() == DetectedType::invalid && get_type_argument().empty()) {
		debug_out_info("app", "The device seems to be of different type than auto-detected, trying again with scsi.\n");
		this->set_type_argument("scsi");
		return this->fetch_data_and_parse(smartctl_ex);  // try again with scsi
	}

	// Since the type error leads to "command line didn't parse" error here,
	// we do this after the scsi stuff.
	if (!error_msg.empty())
		return error_msg;

	this->full_output_ = output;
	return this->parse_data();
}



std::string StorageDevice::parse_data()
{
	this->clear_fetched(false);  // clear everything fetched before, except outputs

	AtaStorageAttribute::DiskType disk_type = AtaStorageAttribute::DiskType::Any;
	if (hdd_.has_value()) {
		disk_type = hdd_.value() ? AtaStorageAttribute::DiskType::Hdd : AtaStorageAttribute::DiskType::Ssd;
	}
	SmartctlAtaTextParser ps;
	if (ps.parse_full(this->full_output_, disk_type)) {  // try to parse it (parse only, set the properties after basic parsing).

		// refresh basic info too
		this->info_output_ = ps.get_data_full();  // put data including version information

		// note: this will clear the non-basic properties!
		// this will parse some info that is already parsed by SmartctlAtaTextParser::parse_full(),
		// but this one sets the StorageDevice class members, not properties.
		this->parse_basic_data(false, false);  // don't emit signal, we're not complete yet.

		// Call this after parse_basic_data(), since it sets parse status to "info".
		this->set_parse_status(StorageDevice::ParseStatus::full);

		// set the full properties
		this->set_properties(ps.get_properties());  // copy to our drive, overwriting old data

		signal_changed().emit(this);  // notify listeners

		return std::string();
	}

	// Don't show any GUI warnings on parse failure - it may just be an unsupported
	// drive (e.g. usb flash disk). Plus, it may flood the string. The data will be
	// parsed again in Info window, and we show the warnings there.
	debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse smartctl output.\n");

	// proper parsing failed. try to at least extract info section
	this->info_output_ = this->full_output_;  // complete output here. sometimes it's only the info section
	if (!this->parse_basic_data(true).empty()) {  // will add some properties too. this will emit signal_changed().
		return ps.get_error_msg();  // return full parser's error messages - they are more detailed.
	}

	return std::string();  // return ok if at least the info was ok.
}



StorageDevice::ParseStatus StorageDevice::get_parse_status() const
{
	return parse_status_;
}



std::string StorageDevice::set_smart_enabled(bool b, const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_)
		return _("A test is currently being performed on this drive.");

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
	std::string error_msg = execute_device_smartctl((b ? "--smart=on --saveauto=on" : "--smart=off"), smartctl_ex, output);
	if (!error_msg.empty()) {
		return error_msg;
	}

	// search at line start, because they are sometimes present in other sentences too.
	if (app_pcre_match("/^SMART Enabled/mi", output) || app_pcre_match("/^SMART Disabled/mi", output)) {
		return std::string();  // success
	}

	if (app_pcre_match("/^A mandatory SMART command failed/mi", output)) {
		return _("Mandatory SMART command failed.");
	}

	return _("Unknown error occurred.");
}



std::string StorageDevice::set_aodc_enabled(bool b, const std::shared_ptr<CommandExecutor>& smartctl_ex)
{
	if (this->test_is_active_) {
		return _("A test is currently being performed on this drive.");
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
	std::string error_msg = execute_device_smartctl((b ? "--offlineauto=on" : "--offlineauto=off"), smartctl_ex, output);
	if (!error_msg.empty())
		return error_msg;

	if (app_pcre_match("/Testing Enabled/mi", output) || app_pcre_match("/Testing Disabled/mi", output)) {
		return std::string();  // success
	}

	if (app_pcre_match("/^A mandatory SMART command failed/mi", output)) {
		return _("Mandatory SMART command failed.");
	}

	return _("Unknown error occurred.");
}



StorageDevice::Status StorageDevice::get_smart_status() const
{
	Status status = Status::unsupported;
	if (smart_enabled_.has_value()) {
		if (smart_enabled_.value()) {  // enabled, supported
			status = Status::enabled;
		} else {  // if it's disabled, maybe it's unsupported, check that:
			if (smart_supported_.has_value()) {
				if (smart_supported_.value()) {  // disabled, supported
					status = Status::disabled;
				} else {  // disabled, unsupported
					status = Status::unsupported;
				}
			} else {  // disabled, support unknown
				status = Status::disabled;
			}
		}
	} else {  // status unknown
		if (smart_supported_.has_value()) {
			if (smart_supported_.value()) {  // status unknown, supported
				status = Status::disabled;  // at least give the user a chance to try enabling it
			} else {  // status unknown, unsupported
				status = Status::unsupported;  // most likely
			}
		} else {  // status unknown, support unknown
			status = Status::unsupported;
		}
	}
	return status;
}



StorageDevice::Status StorageDevice::get_aodc_status() const
{
	// smart-disabled drives are known to print some garbage, so
	// let's protect us from it.
	if (get_smart_status() != Status::enabled)
		return Status::unsupported;

	if (aodc_status_.has_value())  // cached return value
		return aodc_status_.value();

	Status status = Status::unknown;  // for now

	bool aodc_supported = false;
	int found = 0;

	for (const auto& p : properties_) {
		if (p.section == AtaStorageProperty::Section::internal) {
			if (p.generic_name == "aodc_enabled") {  // if this is not present at all, we set the unknown status.
				status = (p.get_value<bool>() ? Status::enabled : Status::disabled);
				//++found;
				continue;
			}
			if (p.generic_name == "aodc_support") {
				aodc_supported = p.get_value<bool>();
				++found;
				continue;
			}
			if (found >= 2)
				break;
		}
	}

	if (!aodc_supported)
		status = Status::unsupported;
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

	AtaStorageProperty p = this->lookup_property("overall_health",
			AtaStorageProperty::Section::data, AtaStorageProperty::SubSection::health);
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

	std::string::size_type pos = device_.rfind('/');  // find basename
	if (pos == std::string::npos)
		return device_;  // fall back
	return device_.substr(pos+1, std::string::npos);
}



std::string StorageDevice::get_device_with_type() const
{
	if (this->get_is_virtual()) {
		std::string vf = this->get_virtual_filename();
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
	return (is_virtual_ ? virtual_file_.filename().u8string() : std::string());
}



const std::vector<AtaStorageProperty>& StorageDevice::get_properties() const
{
	return properties_;
}



AtaStorageProperty StorageDevice::lookup_property(const std::string& generic_name, AtaStorageProperty::Section section, AtaStorageProperty::SubSection subsection) const
{
	for (const auto& p : properties_) {
		if (section != AtaStorageProperty::Section::unknown && p.section != section)
			continue;
		if (subsection != AtaStorageProperty::SubSection::unknown && p.subsection != subsection)
			continue;

		if (p.generic_name == generic_name)
			return p;
	}
	return AtaStorageProperty();  // check with .empty()
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
	info_output_ = std::move(s);
}



std::string StorageDevice::get_info_output() const
{
	return info_output_;
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
	bool changed = (test_is_active_ != b);
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
	std::string model = this->get_model_name();  // may be empty
	std::string serial = this->get_serial_number();
	std::string date = hz::format_date("%Y-%m-%d_%H%M", true);

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
		return std::string();
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



std::string StorageDevice::execute_device_smartctl(const std::string& command_options,
		const std::shared_ptr<CommandExecutor>& smartctl_ex, std::string& smartctl_output, bool check_type)
{
	// don't forbid running on currently tested drive - we need to call this from the test code.

	if (is_virtual_) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot execute smartctl on a virtual device.\n");
		return _("Cannot execute smartctl on a virtual device.");
	}

	std::string device = get_device();

	std::string error_msg = execute_smartctl(device, this->get_device_options(),
			command_options, smartctl_ex, smartctl_output);

	if (!error_msg.empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Smartctl binary did not execute cleanly.\n");

		// Smartctl 5.39 cvs/svn version defaults to usb type on at least linux and windows.
		// This means that the old SCSI identify command isn't executed by default,
		// and there is no information about the device manufacturer/etc... in the output.
		// We detect this and set the device type to scsi to at least have _some_ info.
		if (check_type && this->get_detected_type() == DetectedType::unknown
				&& app_pcre_match("/specify device type with the -d option/mi", smartctl_output)) {
			this->set_detected_type(DetectedType::invalid);
		}

		return error_msg;
	}

	return std::string();
}



sigc::signal<void, StorageDevice*>& StorageDevice::signal_changed()
{
	return signal_changed_;
}



void StorageDevice::set_parse_status(ParseStatus value)
{
	parse_status_ = value;
}



void StorageDevice::set_properties(std::vector<AtaStorageProperty> props)
{
	properties_ = std::move(props);
}







/// @}
