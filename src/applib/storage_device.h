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

#ifndef STORAGE_DEVICE_H
#define STORAGE_DEVICE_H

#include <string>
#include <map>
#include <sigc++/sigc++.h>

#include "hz/optional_value.h"
#include "hz/intrusive_ptr.h"

#include "storage_property.h"
#include "smartctl_parser.h"  // prop_list_t
#include "smartctl_executor.h"



class StorageDevice;


/// A reference-counting pointer to StorageDevice
typedef hz::intrusive_ptr<StorageDevice> StorageDeviceRefPtr;



/// This class represents a single drive
class StorageDevice : public hz::intrusive_ptr_referenced {

	public:

		/// These may be used to force smartctl to a special type, as well as
		/// to display the correct icon
		enum detected_type_t {
			detected_type_unknown,  // Unknown. Will be autodetected by smartctl.
			detected_type_invalid,  // This is set by smartctl executor if it detects invalid type (but not if it's scsi).
			detected_type_cddvd,  // CD/DVD/Blu-Ray. Unsupported by smartctl, only basic info is given.
			detected_type_raid,  // RAID controller or volume. Unsupported by smartctl, only basic info is given.
		};


		/// This gives a string which can be displayed in outputs
		static std::string get_type_readable_name(detected_type_t type);


		/// Statuses of various states
		enum status_t {
			status_enabled,  ///< SMART, AODC
			status_disabled,  ///< SMART, AODC
			status_unsupported,  ///< SMART, AODC
			status_unknown  ///< AODC - supported but unknown if it's enabled or not.
		};

		/// Get displayable name for status_t.
		static std::string get_status_name(status_t status, bool use_yesno = false);


		/// Comparison function for sorting the devices
		static bool order_less_than(const StorageDeviceRefPtr& a, const StorageDeviceRefPtr& b);


		/// Statuses of various parse states
		enum parse_status_t {
			parse_status_full,  ///< Fully parsed
			parse_status_info,  ///< Only info section available
			parse_status_none,  ///< No data
		};


		/// Constructor
		StorageDevice(const std::string& dev_or_vfile, bool is_virtual = false);

		/// Constructor
		StorageDevice(const std::string& dev, const std::string& type_arg);

		/// Copy constructor
		StorageDevice(const StorageDevice& other);

		/// Assignment operator
		StorageDevice& operator= (const StorageDevice& other);


		// clear everything fetched before.
		void clear_fetched(bool including_outputs = true);

		/// Calls "smartctl -i -H -c" (info section, health, capabilities), then parse_basic_data().
		/// Called during drive detection.
		/// Note: this will clear the non-basic properties!
		std::string fetch_basic_data_and_parse(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);

		/// Detects type, smart support, smart status (on / off).
		/// Note: this will clear the non-basic properties!
		std::string parse_basic_data(bool do_set_properties = true, bool emit_signal = true);

		/// Execute smartctl --all (all sections), get output, parse it (basic data too), fill properties.
		std::string fetch_data_and_parse(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);  // returns error message on error.

		// Parses full info. If failed, try to parse it as basic info.
		/// \return error message on error.
		std::string parse_data();

		/// Get the "fully parsed" flag
		parse_status_t get_parse_status() const;


		/// Try to enable SMART.
		/// \return error message on error, empty string on success
		std::string set_smart_enabled(bool b, hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);

		/// Try to enable Automatic Offline Data Collection.
		/// \return error message on error, empty string on success
		std::string set_aodc_enabled(bool b, hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


		/// Get SMART status
		status_t get_smart_status() const;

		/// Get AODC status
		status_t get_aodc_status() const;


		/// Get format size string, or an empty string on error.
		std::string get_device_size_str() const;

		/// Get the overall health property
		StorageProperty get_health_property() const;


		/// Get device name (e.g. /dev/sda)
		std::string get_device() const;

		/// Get device name without path. For example, "sda".
		std::string get_device_base() const;

		/// Get device name for display purposes (with a type argument in parentheses)
		std::string get_device_with_type() const;


		/// Set detected type
		void set_detected_type(detected_type_t t);

		/// Get detected type
		detected_type_t get_detected_type() const;


		/// Set argument for "-d" smartctl parameter
		void set_type_argument(const std::string& arg);

		/// Get argument for "-d" smartctl parameter
		std::string get_type_argument() const;


		/// Set extra arguments smartctl
		void set_extra_arguments(const std::string& args);

		/// Get extra arguments smartctl
		std::string get_extra_arguments() const;


		/// Set windows drive letters for this drive
		void set_drive_letters(const std::map<char, std::string>& letters_volnames);

		/// Get windows drive letters for this drive
		const std::map<char, std::string>& get_drive_letters() const;

		/// Get comma-separated win32 drive letters (if present)
		std::string format_drive_letters(bool with_volnames) const;


		/// Get "virtual" status
		bool get_is_virtual() const;

		/// If the device is virtual, return its file
		std::string get_virtual_file() const;

		/// Get only the filename portion of a virtual file
		std::string get_virtual_filename() const;


		/// Get all detected properties
		const SmartctlParser::prop_list_t& get_properties() const;


		/// Find a property
		StorageProperty lookup_property(const std::string& generic_name,
				StorageProperty::section_t section = StorageProperty::section_unknown,  // if unknown, search in all.
				StorageProperty::subsection_t subsection = StorageProperty::subsection_unknown) const;


		/// Get model name.
		/// \return empty string if not found
		std::string get_model_name() const;

		/// Get family name.
		/// \return empty string if not found
		std::string get_family_name() const;

		/// Get serial number.
		/// \return empty string if not found
		std::string get_serial_number() const;

		/// Check whether this drive is a rotational HDD.
		bool get_is_hdd() const;


		/// Set "info" output to parse
		void set_info_output(const std::string& s);

		/// Get "info" output to parse
		std::string get_info_output() const;


		/// Set "full" output to parse
		void set_full_output(const std::string& s);

		/// Get "full" output to parse
		std::string get_full_output() const;


		/// Set "manually added" flag
		void set_is_manually_added(bool b);

		/// Get "manually added" flag
		bool get_is_manually_added() const;


		/// Set "test is active" flag, emit the "changed" signal if needed.
		void set_test_is_active(bool b);

		/// Get "test is active" flag
		bool get_test_is_active() const;


		/// Get the recommended filename to save output to. Includes model and date.
		std::string get_save_filename() const;


		/// Get final smartctl options for this device from config and type info.
		std::string get_device_options() const;


		/// Execute smartctl on this device. Nothing is modified in this class.
		/// \return error message on error, empty string on success
		std::string execute_device_smartctl(const std::string& command_options,
				hz::intrusive_ptr<CmdexSync> smartctl_ex, std::string& output, bool check_type = false);


		/// Emitted whenever new information is available
		sigc::signal<void, StorageDevice*> signal_changed;


	protected:

		/// Set the "fully parsed" flag
		void set_parse_status(parse_status_t value);

		/// Set parsed properties
		void set_properties(const SmartctlParser::prop_list_t& props);


	private:

		std::string info_output_;  ///< "smartctl --info" output
		std::string full_output_;  ///< "smartctl --all" output

		std::string device_;  ///< e.g. /dev/sda or pd0. empty if virtual.
		std::string type_arg_;  ///< Device type (for -d smartctl parameter), as specified when adding the device.
		std::string extra_args_;  ///< Extra parameters for smartctl, as specified when adding the device.

		std::map<char, std::string> drive_letters_;  ///< Windows drive letters (if detected), with volume names

		bool is_virtual_;  ///< If true, then this is not a real device - merely a loaded description of it.
		std::string virtual_file_;  ///< A file (smartctl data) the virtual device was loaded from
		bool is_manually_added_;  ///< StorageDevice doesn't use it, but it's useful for its users.

		parse_status_t parse_status_;  ///< "Fully parsed" flag

		/// Sort of a "lock". If true, the device is not allowed to perform any commands
		/// except "-l selftest" and maybe "--capabilities" and "--info" (not sure).
		bool test_is_active_;

		// Note: These are detected through info output
		detected_type_t detected_type_;  ///< e.g. type_unknown
		hz::OptionalValue<bool> smart_supported_;  ///< SMART support status
		hz::OptionalValue<bool> smart_enabled_;  ///< SMART enabled status
		mutable hz::OptionalValue<status_t> aodc_status_;  ///< Cached aodc status.
		hz::OptionalValue<std::string> model_name_;  ///< Model name
		hz::OptionalValue<std::string> family_name_;  ///< Family name
		hz::OptionalValue<std::string> serial_number_;  ///< Serial number
		hz::OptionalValue<std::string> size_;  ///< Formatted size
		hz::OptionalValue<bool> hdd_;  ///< Whether it's a rotational drive (HDD) or something else (SSD, flash, etc...)
		mutable hz::OptionalValue<StorageProperty> health_property_;  ///< Cached health property.

		SmartctlParser::prop_list_t properties_;  ///< Smart properties. Detected through full output.


};




/// Operator for sorting, hard drives first, then device name base
inline bool operator< (const StorageDeviceRefPtr& a, const StorageDeviceRefPtr& b)
{
// 	if (a->get_detected_type() != a->get_detected_type()) {
// 		return (a->get_detected_type() == StorageDevice::detected_type_unknown);  // hard drives first
// 	}
	if (a->get_is_virtual() != b->get_is_virtual()) {
		return int(a->get_is_virtual()) < int(b->get_is_virtual());
	}
	if (a->get_is_virtual()) {
		return a->get_virtual_file() < b->get_virtual_file();
	}
	if (a->get_device_base() != b->get_device_base()) {
		return a->get_device_base() < b->get_device_base();
	}
	return a->get_type_argument() < b->get_type_argument();
}







#endif

/// @}
