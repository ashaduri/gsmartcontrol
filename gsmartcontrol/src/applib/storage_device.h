/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef STORAGE_DEVICE_H
#define STORAGE_DEVICE_H

#include <string>
#include <sigc++/sigc++.h>

#include "hz/optional_value.h"
#include "hz/intrusive_ptr.h"

#include "storage_property.h"
#include "smartctl_parser.h"  // prop_list_t
#include "smartctl_executor.h"



class StorageDevice;


class StorageDevice : public hz::intrusive_ptr_referenced {

	public:

		// these may be used to force smartctl to a special type, as well as
		// to display the correct icon
		enum type_t {
			type_unknown,  // unknown. will be autodetected by smartctl
			type_invalid,  // this is set by smartctl executor if it detects invalid type (but not if it's scsi).
			type_cddvd,  // unsupported by smartctl, only basic info is given.
			type_scsi  // this is used to force "-d scsi" to execute IDENTIFY command.
		};


		// this gives a string which can be displayed in outputs
		static std::string get_type_readable_name(type_t type)
		{
			switch (type) {
				case type_unknown: return "unknown";
				case type_invalid: return "invalid";
				case type_cddvd: return "cd/dvd";
				case type_scsi: return "scsi";
			}
			return "[internal_error]";
		}


		// this gives a string which, if not empty, can be given as a parameter of "-d".
		static std::string get_type_arg_name(type_t type)
		{
			switch (type) {
				case type_unknown: return "";
				case type_invalid: return "";
				case type_cddvd: return "";
				case type_scsi: return "scsi";
			}
			return "";
		}


		enum status_t {  // statuses of various states
			status_enabled,  // smart, aodc
			status_disabled,  // smart, aodc
			status_unsupported,  // smart, aodc
			status_unknown  // aodc - supported but unknown if it's enabled or not.
		};

		static std::string get_status_name(status_t status, bool use_yesno = false)
		{
			switch (status) {
				case status_enabled: return (use_yesno ? "Yes" : "Enabled");
				case status_disabled: return (use_yesno ? "No" : "Disabled");
				case status_unsupported: return "Unsupported";
				case status_unknown: return "Unknown";
			};
			return "[internal_error]";
		}



		StorageDevice(const std::string& dev_or_vfile, bool is_virtual = false)
		{
			type_ = type_unknown;
			// force_type_ = false;
			is_virtual_ = is_virtual;
			is_manually_added_ = false;
			fully_parsed_ = false;
			test_is_active_ = false;

			if (is_virtual) {
				virtual_file_ = dev_or_vfile;
			} else {
				device_ = dev_or_vfile;
			}
		}


		StorageDevice(const StorageDevice& other)
		{
			*this = other;
		}


		StorageDevice& operator= (const StorageDevice& other)
		{
			info_output_ = other.info_output_;
			full_output_ = other.full_output_;

			device_ = other.device_;

			// force_type_ = other.force_type_;
			is_virtual_ = other.is_virtual_;
			virtual_file_ = other.virtual_file_;
			is_manually_added_ = other.is_manually_added_;

			fully_parsed_ = other.fully_parsed_;
			test_is_active_ = other.test_is_active_;

			type_ = other.type_;
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


		// clear everything fetched before.
		void clear_fetched(bool including_outputs = true)
		{
			if (including_outputs) {
				info_output_.clear();
				full_output_.clear();
			}

			fully_parsed_ = false;
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


		// calls "smartctl --info" (info section), then parse_basic_data()
		// called during drive detection.
		// note: this will clear the non-basic properties!
		std::string fetch_basic_data_and_parse(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


		// detects type, smart support, smart status (on / off).
		// note: this will clear the non-basic properties!
		std::string parse_basic_data(bool do_set_properties = true, bool emit_signal = true);


		// execute smartctl --all (all sections), get output, parse it, fill properties.
		std::string fetch_data_and_parse(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);  // returns error message on error.


		std::string parse_data();  // returns error message on error.


		void set_fully_parsed(bool b)
		{
			fully_parsed_ = b;
		}

		bool get_fully_parsed() const
		{
			return fully_parsed_;
		}


		// try to enable SMART. will return error message on error.
		std::string set_smart_enabled(bool b, hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);

		// try to enable Automatic Offline Data Collection. will return error message on error.
		std::string set_aodc_enabled(bool b, hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


		status_t get_smart_status() const;

		status_t get_aodc_status() const;

		// returns empty string on error, format size string on success.
		std::string get_device_size_str() const;

		StorageProperty get_health_property() const;


		std::string get_device() const
		{
			return device_;
		}


		std::string get_device_base() const
		{
			if (is_virtual_)
				return "";

			std::string::size_type pos = device_.rfind('/');  // find basename
			if (pos == std::string::npos)
				return device_;  // fall back
			return device_.substr(pos+1, std::string::npos);
		}


		// for display purposes
		std::string get_device_pretty(bool extended = false) const
		{
			std::string ret;
			if (this->get_is_virtual()) {
				ret = "Virtual";
				if (extended) {
					std::string vf = this->get_virtual_filename();
					ret += (" (" + (vf.empty() ? "[empty]" : vf) + ")");
				}
			} else {
				ret = (extended ? this->get_device() : this->get_device_base());
			}

			return ret;
		}



		void set_type(type_t t)
		{
			type_ = t;
		}

		type_t get_type() const
		{
			return type_;
		}



		bool get_is_virtual() const
		{
			return is_virtual_;
		}

		std::string get_virtual_file() const
		{
			return (is_virtual_ ? virtual_file_ : std::string());
		}

		// get only the filename portion
		std::string get_virtual_filename() const;


		const SmartctlParser::prop_list_t& get_properties() const
		{
			return properties_;
		}


		StorageProperty lookup_property(const std::string& generic_name,
				StorageProperty::section_t section = StorageProperty::section_unknown,  // if unknown, search in all.
				StorageProperty::subsection_t subsection = StorageProperty::subsection_unknown) const
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


		// returns an empty string if unknown
		std::string get_model_name() const
		{
			return (model_name_.defined() ? model_name_.value() : "");
		}


		// returns an empty string if unknown
		std::string get_family_name() const
		{
			return (family_name_.defined() ? family_name_.value() : "");
		}


		// returns an empty string if unknown
		std::string get_serial_number() const
		{
			return (serial_number_.defined() ? serial_number_.value() : "");
		}


		void set_info_output(const std::string& s)
		{
			info_output_ = s;
		}

		void set_full_output(const std::string& s)
		{
			full_output_ = s;
		}


		std::string get_info_output() const
		{
			return info_output_;
		}

		std::string get_full_output() const
		{
			return full_output_;
		}


		void set_is_manually_added(bool b)
		{
			is_manually_added_ = b;
		}

		bool get_is_manually_added() const
		{
			return is_manually_added_;
		}


		void set_test_is_active(bool b)
		{
			bool changed = (test_is_active_ != b);
			test_is_active_ = b;
			if (changed)
				signal_changed.emit(this);  // so that everybody stops any test-aborting operations.
		}

		bool get_test_is_active() const
		{
			return test_is_active_;
		}


		// get the recommended filename to save output to. includes model and date.
		std::string get_save_filename() const;


		std::string get_device_options() const;  // get smartctl options for this device from config and type info.


		// execute smartctl on this device. nothing is modified in this class.
		std::string execute_smartctl(const std::string& command_options,
				hz::intrusive_ptr<CmdexSync> smartctl_ex, std::string& output, bool check_type = false);


		// emitted whenever new information is available
		sigc::signal<void, StorageDevice*> signal_changed;


	protected:

		void set_properties(const SmartctlParser::prop_list_t& props)
		{
			properties_ = props;
		}


		std::string info_output_;  // "smartctl --info" output
		std::string full_output_;  // "smartctl --all" output

		std::string device_;  // e.g. /dev/sda. empty if virtual.

		// bool force_type_;  // force "-d type" to smartctl, e.g. "-d scsi". DISCONTINUED, use per-device options.
		bool is_virtual_;  // if true, then this is not a real device - merely a loaded description of it.
		std::string virtual_file_;  // a file (smartctl data) the virtual device was loaded from
		bool is_manually_added_;  // StorageDevice doesn't use it, but it's useful for its users.

		bool fully_parsed_;

		// sort of "lock". if true, the device is not allowed to perform any commands
		// except "-l selftest" and maybe "--capabilities" and "--info" (not sure).
		bool test_is_active_;

		// these are detected through info output
		type_t type_;  // e.g. type_ata
		hz::OptionalValue<bool> smart_supported_;
		hz::OptionalValue<bool> smart_enabled_;
		mutable hz::OptionalValue<status_t> aodc_status_;  // cached aodc status.
		hz::OptionalValue<std::string> model_name_;
		hz::OptionalValue<std::string> family_name_;
		hz::OptionalValue<std::string> serial_number_;
		hz::OptionalValue<std::string> size_;  // formatted size
		mutable hz::OptionalValue<StorageProperty> health_property_;  // cached health property.

		// smart properties. detected through full output.
		SmartctlParser::prop_list_t properties_;


};




typedef hz::intrusive_ptr<StorageDevice> StorageDeviceRefPtr;


// for sorting, hard drives first
inline bool operator< (const StorageDeviceRefPtr& d1, const StorageDeviceRefPtr& d2)
{
	if (d1->get_type() != d2->get_type()) {
		return (d1->get_type() == StorageDevice::type_unknown);  // hard drives first
	}
	return d1->get_device_base() < d2->get_device_base();
}







#endif
