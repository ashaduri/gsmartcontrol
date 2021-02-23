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

#ifndef STORAGE_DEVICE_H
#define STORAGE_DEVICE_H

#include <string>
#include <map>
#include <optional>
#include <memory>
#include <sigc++/sigc++.h>

#include "hz/fs_ns.h"
#include "storage_property.h"
#include "smartctl_parser.h"  // prop_list_t
#include "smartctl_executor.h"



class StorageDevice;


/// A reference-counting pointer to StorageDevice
using StorageDevicePtr = std::shared_ptr<StorageDevice>;



/// This class represents a single drive
class StorageDevice {
	public:

		/// These may be used to force smartctl to a special type, as well as
		/// to display the correct icon
		enum class DetectedType {
			unknown,  // Unknown. Will be autodetected by smartctl.
			invalid,  // This is set by smartctl executor if it detects invalid type (but not if it's scsi).
			cddvd,  // CD/DVD/Blu-Ray. Unsupported by smartctl, only basic info is given.
			raid,  // RAID controller or volume. Unsupported by smartctl, only basic info is given.
		};


		/// This gives a string which can be displayed in outputs
		static std::string get_type_storable_name(DetectedType type);


		/// Statuses of various states
		enum class Status {
			enabled,  ///< SMART, AODC
			disabled,  ///< SMART, AODC
			unsupported,  ///< SMART, AODC
			unknown  ///< AODC - supported but unknown if it's enabled or not.
		};

		/// Get displayable name for Status.
		static std::string get_status_displayable_name(Status status);


		/// Statuses of various parse states
		enum class ParseStatus {
			full,  ///< Fully parsed
			info,  ///< Only info section available
			none,  ///< No data
		};


		/// Constructor
		explicit StorageDevice(std::string dev_or_vfile, bool is_virtual = false);

		/// Constructor
		StorageDevice(std::string dev, std::string type_arg);


		// clear everything fetched before.
		void clear_fetched(bool including_outputs = true);

		/// Calls "smartctl -i -H -c" (info section, health, capabilities), then parse_basic_data().
		/// Called during drive detection.
		/// Note: this will clear the non-basic properties!
		std::string fetch_basic_data_and_parse(const std::shared_ptr<CmdexSync>& smartctl_ex = nullptr);

		/// Detects type, smart support, smart status (on / off).
		/// Note: this will clear the non-basic properties!
		std::string parse_basic_data(bool do_set_properties = true, bool emit_signal = true);

		/// Execute smartctl --all (all sections), get output, parse it (basic data too), fill properties.
		std::string fetch_data_and_parse(const std::shared_ptr<CmdexSync>& smartctl_ex);  // returns error message on error.

		// Parses full info. If failed, try to parse it as basic info.
		/// \return error message on error.
		std::string parse_data();

		/// Get the "fully parsed" flag
		ParseStatus get_parse_status() const;


		/// Try to enable SMART.
		/// \return error message on error, empty string on success
		std::string set_smart_enabled(bool b, const std::shared_ptr<CmdexSync>&);

		/// Try to enable Automatic Offline Data Collection.
		/// \return error message on error, empty string on success
		std::string set_aodc_enabled(bool b, const std::shared_ptr<CmdexSync>&);


		/// Get SMART status
		Status get_smart_status() const;

		/// Get AODC status
		Status get_aodc_status() const;


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
		void set_detected_type(DetectedType t);

		/// Get detected type
		DetectedType get_detected_type() const;


		/// Set argument for "-d" smartctl parameter
		void set_type_argument(std::string arg);

		/// Get argument for "-d" smartctl parameter
		std::string get_type_argument() const;


		/// Set extra arguments smartctl
		void set_extra_arguments(std::string args);

		/// Get extra arguments smartctl
		std::string get_extra_arguments() const;


		/// Set windows drive letters for this drive
		void set_drive_letters(std::map<char, std::string> letters_volnames);

		/// Get windows drive letters for this drive
		const std::map<char, std::string>& get_drive_letters() const;

		/// Get comma-separated win32 drive letters (if present)
		std::string format_drive_letters(bool with_volnames) const;


		/// Get "virtual" status
		bool get_is_virtual() const;

		/// If the device is virtual, return its file
		hz::fs::path get_virtual_file() const;

		/// Get only the filename portion of a virtual file
		std::string get_virtual_filename() const;


		/// Get all detected properties
		const std::vector<StorageProperty>& get_properties() const;


		/// Find a property
		StorageProperty lookup_property(const std::string& generic_name,
				StorageProperty::Section section = StorageProperty::Section::unknown,  // if unknown, search in all.
				StorageProperty::SubSection subsection = StorageProperty::SubSection::unknown) const;


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
		void set_info_output(std::string s);

		/// Get "info" output to parse
		std::string get_info_output() const;


		/// Set "full" output to parse
		void set_full_output(std::string s);

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
				const std::shared_ptr<CmdexSync>& smartctl_ex, std::string& output, bool check_type = false);


		/// Emitted whenever new information is available
		sigc::signal<void, StorageDevice*> signal_changed;


	protected:

		/// Set the "fully parsed" flag
		void set_parse_status(ParseStatus value);

		/// Set parsed properties
		void set_properties(std::vector<StorageProperty> props);


	private:

		std::string info_output_;  ///< "smartctl --info" output
		std::string full_output_;  ///< "smartctl --all" output

		std::string device_;  ///< e.g. /dev/sda or pd0. empty if virtual.
		std::string type_arg_;  ///< Device type (for -d smartctl parameter), as specified when adding the device.
		std::string extra_args_;  ///< Extra parameters for smartctl, as specified when adding the device.

		std::map<char, std::string> drive_letters_;  ///< Windows drive letters (if detected), with volume names

		bool is_virtual_ = false;  ///< If true, then this is not a real device - merely a loaded description of it.
		hz::fs::path virtual_file_;  ///< A file (smartctl data) the virtual device was loaded from
		bool is_manually_added_ = false;  ///< StorageDevice doesn't use it, but it's useful for its users.

		ParseStatus parse_status_ = ParseStatus::none;  ///< "Fully parsed" flag

		/// Sort of a "lock". If true, the device is not allowed to perform any commands
		/// except "-l selftest" and maybe "--capabilities" and "--info" (not sure).
		bool test_is_active_ = false;

		// Note: These are detected through info output
		DetectedType detected_type_ = DetectedType::unknown;  ///< e.g. type_unknown
		std::optional<bool> smart_supported_;  ///< SMART support status
		std::optional<bool> smart_enabled_;  ///< SMART enabled status
		mutable std::optional<Status> aodc_status_;  ///< Cached aodc status.
		std::optional<std::string> model_name_;  ///< Model name
		std::optional<std::string> family_name_;  ///< Family name
		std::optional<std::string> serial_number_;  ///< Serial number
		std::optional<std::string> size_;  ///< Formatted size
		std::optional<bool> hdd_;  ///< Whether it's a rotational drive (HDD) or something else (SSD, flash, etc...)
		mutable std::optional<StorageProperty> health_property_;  ///< Cached health property.

		std::vector<StorageProperty> properties_;  ///< Smart properties. Detected through full output.


};




/// Operator for sorting, hard drives first, then device name base
inline bool operator< (const StorageDevicePtr& a, const StorageDevicePtr& b)
{
// 	if (a->get_detected_type() != a->get_detected_type()) {
// 		return (a->get_detected_type() == StorageDevice::DetectedType::unknown);  // hard drives first
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
