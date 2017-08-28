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

#include "rconfig/rconfig_mini.h"
#include "hz/string_algo.h"  // string_trim_copy, string_any_to_unix_copy
#include "hz/fs_path.h"  // FsPath
#include "hz/fs_path_utils.h"  // hz::filename_make_safe
#include "hz/format_unit.h"  // hz::format_date

#include "app_pcrecpp.h"
#include "storage_device.h"
#include "smartctl_parser.h"
#include "storage_settings.h"
#include "smartctl_executor.h"




std::string StorageDevice::get_type_readable_name(StorageDevice::detected_type_t type)
{
	switch (type) {
	case detected_type_unknown:
		return "unknown";
	case detected_type_invalid:
		return "invalid";
	case detected_type_cddvd:
		return "cd/dvd";
	case detected_type_raid:
		return "raid";
	}
	return "[internal_error]";
}



std::string StorageDevice::get_status_name(StorageDevice::status_t status, bool use_yesno)
{
	switch (status) {
	case status_enabled:
		return (use_yesno ? "Yes" : "Enabled");
	case status_disabled:
		return (use_yesno ? "No" : "Disabled");
	case status_unsupported:
		return "Unsupported";
	case status_unknown:
		return "Unknown";
	};
	return "[internal_error]";
}



StorageDevice::StorageDevice(const std::string& dev_or_vfile, bool is_virtual)
{
	detected_type_ = detected_type_unknown;
	// force_type_ = false;
	is_virtual_ = is_virtual;
	is_manually_added_ = false;
	parse_status_ = parse_status_none;
	test_is_active_ = false;

	if (is_virtual) {
		virtual_file_ = dev_or_vfile;
	} else {
		device_ = dev_or_vfile;
	}
}



StorageDevice::StorageDevice(const std::string& dev, const std::string& type_arg)
{
	detected_type_ = detected_type_unknown;
	// force_type_ = false;
	is_virtual_ = false;
	is_manually_added_ = false;
	parse_status_ = parse_status_none;
	test_is_active_ = false;

	device_ = dev;
	type_arg_ = type_arg;
}



StorageDevice::StorageDevice(const StorageDevice& other)
{
	*this = other;
}



StorageDevice& StorageDevice::operator=(const StorageDevice& other)
{
	info_output_ = other.info_output_;
	full_output_ = other.full_output_;

	device_ = other.device_;
	type_arg_ = other.type_arg_;
	extra_args_ = other.extra_args_;

	// force_type_ = other.force_type_;
	is_virtual_ = other.is_virtual_;
	virtual_file_ = other.virtual_file_;
	is_manually_added_ = other.is_manually_added_;

	parse_status_ = other.parse_status_;
	test_is_active_ = other.test_is_active_;

	detected_type_ = other.detected_type_;
	smart_supported_ = other.smart_supported_;
	smart_enabled_ = other.smart_enabled_;
	aodc_status_ = other.aodc_status_;
	model_name_ = other.model_name_;
	family_name_ = other.family_name_;
	size_ = other.size_;
	health_property_ = other.health_property_;

	properties_ = other.properties_;

	return *this;
}



void StorageDevice::clear_fetched(bool including_outputs) {
	if (including_outputs) {
		info_output_.clear();
		full_output_.clear();
	}

	parse_status_ = parse_status_none;
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



std::string StorageDevice::fetch_basic_data_and_parse(hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	if (this->test_is_active_)
		return "A test is currently being performed on this drive.";

	this->clear_fetched();  // clear everything fetched before, including outputs

	// We don't use "--all" - it may cause really screwed up the output (tests, etc...).
	// This looks just like "--info" only on non-smart devices.
	std::string error_msg = execute_device_smartctl("--info --health --capabilities", smartctl_ex, this->info_output_, true);  // set type to invalid if needed

	// Smartctl 5.39 cvs/svn version defaults to usb type on at least linux and windows.
	// This means that the old SCSI identify command isn't executed by default,
	// and there is no information about the device manufacturer/etc... in the output.
	// We detect this and set the device type to scsi to at least have _some_ info.
	if (get_detected_type() == detected_type_invalid && get_type_argument().empty()) {
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
		return "Cannot read information from an empty string.";
	}

	std::string version, version_full;
	if (!SmartctlParser::parse_version(this->info_output_, version, version_full))  // is this smartctl data at all?
		return "Cannot get smartctl version information.";

	// Detect type. note: we can't distinguish between sata and scsi (on linux, for -d ata switch).
	// Sample output line 1 (encountered on a CDRW drive):
	// SMART support is: Unavailable - Packet Interface Devices [this device: CD/DVD] don't support ATA SMART
	// Sample output line 2 (encountered on a BDRW drive):
	// Device type:          CD/DVD
	if (app_pcre_match("/this device: CD\\/DVD/mi", info_output_) || app_pcre_match("/^Device type:\\s+CD\\/DVD/mi", info_output_)) {
		debug_out_dump("app", "Drive " << get_device_with_type() << " seems to be a CD/DVD device.\n");
		this->set_detected_type(detected_type_cddvd);

	// This was encountered on a csmi soft-raid under windows with pd0.
	// The device reported that it had smart supported and enabled.
	// Product:              Raid 5 Volume
	} else if (app_pcre_match("/Product:[ \\t]*Raid/mi", info_output_)) {
		debug_out_dump("app", "Drive " << get_device_with_type() << " seems to be a RAID volume/controller.\n");
		this->set_detected_type(detected_type_raid);
	}

	// RAID volume may report that it has SMART, but it obviously doesn't.
	if (get_detected_type() == detected_type_raid) {
		smart_supported_ = false;
		smart_enabled_ = false;

	} else {
		// Note: We don't use SmartctlParser here, because this information
		// may be in some other format. If this information is valid, only then it's
		// passed to SmartctlParser.
		// Compared to SmartctlParser, this one is much looser.

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
		int rpm = 0;
		hz::string_is_numeric(rpm_str, rpm, false);
		hdd_ = rpm > 0;
	}


	// Note: this property is present since 5.33.
	std::string size;
	if (app_pcre_match("/^User Capacity:[ \\t]*(.*)$/mi", info_output_, &size)) {
		uint64_t bytes = 0;
		size_ = SmartctlParser::parse_byte_size(size, bytes, false);
	}


	// Try to parse the properties. ignore its errors - we already got what we came for.
	// Note that this may try to parse data the second time (it may already have
	// been parsed by parse_data() which failed at it).
	if (do_set_properties) {
		StorageAttribute::DiskType disk_type = StorageAttribute::DiskAny;
		if (hdd_.defined()) {
			disk_type = hdd_.value() ? StorageAttribute::DiskHDD : StorageAttribute::DiskSSD;
		}
		SmartctlParser ps;
		if (ps.parse_full(this->info_output_, disk_type)) {  // try to parse it
			this->set_properties(ps.get_properties());  // copy to our drive, overwriting old data
		}
	}

	// A model field (and its aliases) is a good indication whether there was any data or not
	set_parse_status(model_name_.defined() ? parse_status_info : parse_status_none);

	if (emit_signal)
		signal_changed.emit(this);  // notify listeners

	return std::string();
}



std::string StorageDevice::fetch_data_and_parse(hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	if (this->test_is_active_)
		return "A test is currently being performed on this drive.";

	this->clear_fetched();  // clear everything fetched before, including outputs

	std::string output;
	std::string error_msg;

	// instead of -a, we use all the individual options -a encompasses, so that
	// an addition to default -a output won't affect us.
	if (this->get_type_argument() == "scsi") {  // not sure about correctness... FIXME probably fails with RAID/scsi
		// This doesn't do much yet, but just in case...
		// SCSI equivalent of -a --get=all:
		error_msg = execute_device_smartctl("--health --info --get=all --attributes --log=error --log=selftest", smartctl_ex, output);
	} else {
		// ATA equivalent of -a --get=all:
		error_msg = execute_device_smartctl("--health --info --get=all --capabilities --attributes --log=error --log=selftest --log=selective",
				smartctl_ex, output, true);  // set type to invalid if needed
	}
	// See notes above (in fetch_basic_data_and_parse()).
	if (get_detected_type() == detected_type_invalid && get_type_argument().empty()) {
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

	StorageAttribute::DiskType disk_type = StorageAttribute::DiskAny;
	if (hdd_.defined()) {
		disk_type = hdd_.value() ? StorageAttribute::DiskHDD : StorageAttribute::DiskSSD;
	}
	SmartctlParser ps;
	if (ps.parse_full(this->full_output_, disk_type)) {  // try to parse it (parse only, set the properties after basic parsing).

		// refresh basic info too
		this->info_output_ = ps.get_data_full();  // put data including version information

		// note: this will clear the non-basic properties!
		// this will parse some info that is already parsed by SmartctlParser::parse_full(),
		// but this one sets the StorageDevice class members, not properties.
		this->parse_basic_data(false, false);  // don't emit signal, we're not complete yet.

		// Call this after parse_basic_data(), since it sets parse status to "info".
		this->set_parse_status(StorageDevice::parse_status_full);

		// set the full properties
		this->set_properties(ps.get_properties());  // copy to our drive, overwriting old data

		signal_changed.emit(this);  // notify listeners

		return std::string();
	}

	// Don't show any GUI warnings on parse failure - it may just be an unsupported
	// drive (e.g. usb flash disk). Plus, it may flood the string. The data will be
	// parsed again in Info window, and we show the warnings there.
	debug_out_warn("app", DBG_FUNC_MSG << "Cannot parse smartctl output.\n");

	// proper parsing failed. try to at least extract info section
	this->info_output_ = this->full_output_;  // complete output here. sometimes it's only the info section
	if (!this->parse_basic_data(true).empty()) {  // will add some properties too. this will emit signal_changed.
		return ps.get_error_msg();  // return full parser's error messages - they are more detailed.
	}

	return std::string();  // return ok if at least the info was ok.
}



StorageDevice::parse_status_t StorageDevice::get_parse_status() const
{
	return parse_status_;
}



std::string StorageDevice::set_smart_enabled(bool b, hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	if (this->test_is_active_)
		return "A test is currently being performed on this drive.";

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
	if (!error_msg.empty())
		return error_msg;

	// search at line start, because they are sometimes present in other sentences too.
	if (app_pcre_match("/^SMART Enabled/mi", output) || app_pcre_match("/^SMART Disabled/mi", output)) {
		return std::string();  // success

	} else if (app_pcre_match("/^A mandatory SMART command failed/mi", output)) {
		return "Mandatory SMART command failed.";
	}

	return "Unknown error occurred.";
}



std::string StorageDevice::set_aodc_enabled(bool b, hz::intrusive_ptr<CmdexSync> smartctl_ex)
{
	if (this->test_is_active_)
		return "A test is currently being performed on this drive.";

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

	} else if (app_pcre_match("/^A mandatory SMART command failed/mi", output)) {
		return "Mandatory SMART command failed.";
	}

	return "Unknown error occurred.";
}



StorageDevice::status_t StorageDevice::get_smart_status() const
{
	status_t status = status_unsupported;
	if (smart_enabled_.defined()) {
		if (smart_enabled_.value()) {  // enabled, supported
			status = status_enabled;
		} else {  // if it's disabled, maybe it's unsupported, check that:
			if (smart_supported_.defined()) {
				if (smart_supported_.value()) {  // disabled, supported
					status = status_disabled;
				} else {  // disabled, unsupported
					status = status_unsupported;
				}
			} else {  // disabled, support unknown
				status = status_disabled;
			}
		}
	} else {  // status unknown
		if (smart_supported_.defined()) {
			if (smart_supported_.value()) {  // status unknown, supported
				status = status_disabled;  // at least give the user a chance to try enabling it
			} else {  // status unknown, unsupported
				status = status_unsupported;  // most likely
			}
		} else {  // status unknown, support unknown
			status = status_unsupported;
		}
	}
	return status;
}



StorageDevice::status_t StorageDevice::get_aodc_status() const
{
	// smart-disabled drives are known to print some garbage, so
	// let's protect us from it.
	if (get_smart_status() != status_enabled)
		return status_unsupported;

	if (aodc_status_.defined())  // cached return value
		return aodc_status_.value();

	status_t status = status_unknown;  // for now

	bool aodc_supported = false;
	int found = 0;

	for (SmartctlParser::prop_list_t::const_iterator iter = properties_.begin(); iter != properties_.end(); ++iter) {
		if (iter->section == StorageProperty::section_internal) {
			if (iter->generic_name == "aodc_enabled") {  // if this is not present at all, we set the unknown status.
				status = (iter->value_bool ? status_enabled : status_disabled);
				//++found;
				continue;
			}
			if (iter->generic_name == "aodc_support") {
				aodc_supported = iter->value_bool;
				++found;
				continue;
			}
			if (found >= 2)
				break;
		}
	}

	if (!aodc_supported)
		status = status_unsupported;
	// if it's supported, then status may be enabled, disabled or unknown.

	aodc_status_ = status;  // store to cache

	debug_out_info("app", DBG_FUNC_MSG << "AODC status: " << get_status_name(status) << "\n");

	return status;
}



std::string StorageDevice::get_device_size_str() const
{
	return (size_.defined() ? size_.value() : "");
}



StorageProperty StorageDevice::get_health_property() const
{
	if (health_property_.defined())  // cached return value
		return health_property_.value();

	StorageProperty p = this->lookup_property("overall_health",
			StorageProperty::section_data, StorageProperty::subsection_health);
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
		std::string ret = "Virtual";
		std::string vf = this->get_virtual_filename();
		ret += (" (" + (vf.empty() ? "[empty]" : vf) + ")");
		return ret;
	}
	std::string device = get_device();
	if (!get_type_argument().empty()) {
		device += " (" + get_type_argument() + ")";
	}
	return device;
}



void StorageDevice::set_detected_type(StorageDevice::detected_type_t t)
{
	detected_type_ = t;
}



StorageDevice::detected_type_t StorageDevice::get_detected_type() const
{
	return detected_type_;
}



void StorageDevice::set_type_argument(const std::string& arg)
{
	type_arg_ = arg;
}



std::string StorageDevice::get_type_argument() const
{
	return type_arg_;
}



void StorageDevice::set_extra_arguments(const std::string& args)
{
	extra_args_ = args;
}



std::string StorageDevice::get_extra_arguments() const
{
	return extra_args_;
}



void StorageDevice::set_drive_letters(const std::vector< char >& letters)
{
	drive_letters_ = letters;
}



const std::vector< char >& StorageDevice::get_drive_letters() const
{
	return drive_letters_;
}



std::string StorageDevice::format_drive_letters() const
{
	std::vector<std::string> drive_letters_decorated;
	for (std::size_t i = 0; i < drive_letters_.size(); ++i) {
		drive_letters_decorated.push_back(std::string() + (char)std::toupper(drive_letters_[i]) + ":");
	}
	return hz::string_join(drive_letters_decorated, ", ");
}



bool StorageDevice::get_is_virtual() const
{
	return is_virtual_;
}



std::string StorageDevice::get_virtual_file() const
{
	return (is_virtual_ ? virtual_file_ : std::string());
}



std::string StorageDevice::get_virtual_filename() const
{
	return (is_virtual_ ? hz::FsPath(virtual_file_).get_basename() : std::string());
}



const SmartctlParser::prop_list_t& StorageDevice::get_properties() const
{
	return properties_;
}



StorageProperty StorageDevice::lookup_property(const std::string& generic_name, StorageProperty::section_t section, StorageProperty::subsection_t subsection) const
{
	for (SmartctlParser::prop_list_t::const_iterator iter = properties_.begin(); iter != properties_.end(); ++iter) {
		if (section != StorageProperty::section_unknown && iter->section != section)
			continue;
		if (subsection != StorageProperty::subsection_unknown && iter->subsection != subsection)
			continue;

		if (iter->generic_name == generic_name)
			return *iter;
	}
	return StorageProperty();  // check with .empty()
}



std::string StorageDevice::get_model_name() const
{
	return (model_name_.defined() ? model_name_.value() : "");
}



std::string StorageDevice::get_family_name() const
{
	return (family_name_.defined() ? family_name_.value() : "");
}



std::string StorageDevice::get_serial_number() const
{
	return (serial_number_.defined() ? serial_number_.value() : "");
}



bool StorageDevice::get_is_hdd() const
{
	return hdd_.defined() ? hdd_.value() : false;
}



void StorageDevice::set_info_output(const std::string& s)
{
	info_output_ = s;
}



std::string StorageDevice::get_info_output() const
{
	return info_output_;
}



void StorageDevice::set_full_output(const std::string& s)
{
	full_output_ = s;
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
		signal_changed.emit(this);  // so that everybody stops any test-aborting operations.
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
	std::string date = hz::format_date("%Y-%m-%d", false);

	std::string filename_format;
	rconfig::get_data("gui/smartctl_output_filename_format", filename_format);
	hz::string_replace(filename_format, "{serial}", serial);
	hz::string_replace(filename_format, "{model}", model);
	hz::string_replace(filename_format, "{date}", date);

	return hz::filename_make_safe(filename_format);
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
		hz::intrusive_ptr<CmdexSync> smartctl_ex, std::string& smartctl_output, bool check_type)
{
	// don't forbid running on currently tested drive - we need to call this from the test code.

	if (is_virtual_) {
		debug_out_warn("app", DBG_FUNC_MSG << "Cannot execute smartctl on a virtual device.\n");
		return "Cannot execute smartctl on a virtual device.";
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
		if (check_type && this->get_detected_type() == detected_type_unknown
				&& app_pcre_match("/specify device type with the -d option/mi", smartctl_output)) {
			this->set_detected_type(detected_type_invalid);
		}

		return error_msg;
	}

	return std::string();
}



void StorageDevice::set_parse_status(parse_status_t value)
{
	parse_status_ = value;
}



void StorageDevice::set_properties(const SmartctlParser::prop_list_t& props)
{
	properties_ = props;
}







/// @}
