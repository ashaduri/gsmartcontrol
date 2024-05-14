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

#ifndef STORAGE_DEVICE_H
#define STORAGE_DEVICE_H

#include <string>
#include <map>
#include <optional>
#include <memory>
#include <sigc++/sigc++.h>

#include "hz/fs_ns.h"
#include "storage_property.h"
#include "smartctl_text_ata_parser.h"  // prop_list_t
#include "smartctl_executor.h"
#include "storage_property_repository.h"
#include "storage_device_detected_type.h"





class StorageDevice;


/// A reference-counting pointer to StorageDevice
using StorageDevicePtr = std::shared_ptr<StorageDevice>;



enum class StorageDeviceError {
	TestRunning,  ///< A test is running, so the device cannot perform this operation.
	CannotExecuteOnVirtual,  ///< Cannot execute this operation on a virtual device.
	ExecutionError,  ///< Error executing the command.
	CommandFailed,  ///< SMART command (e.g. enable/disable SMART) failed.
	CommandUnknownError,  ///< Unknown error from the command.
	ParseError,  ///< Error parsing the output.
};


/// This class represents a single drive
class StorageDevice {
	public:

		/// Statuses of various states
		enum class SmartStatus {
			Enabled,
			Disabled,
			Unsupported,
		};

		/// Get displayable name for Status.
		[[nodiscard]] static std::string get_status_displayable_name(SmartStatus status);


		/// Statuses of various parse states
		enum class ParseStatus {
			Full,  ///< Parsed with specialized parser
			Basic,  ///< Only info section available, parsed with Basic parser
			None,  ///< No data
		};


		/// Constructor
		explicit StorageDevice(std::string dev_or_vfile, bool is_virtual = false);

		/// Constructor
		StorageDevice(std::string dev, std::string type_arg);


		/// Clear fetched smartctl outputs
		void clear_outputs();

		/// Clear common properties previously imported from repository.
		void clear_parse_results();


		/// Calls "smartctl -i -H -c" (info section, health, capabilities), then parse_basic_data().
		/// Called during drive detection.
		/// Note: this will clear all previous properties!
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> fetch_basic_data_and_parse(
				const std::shared_ptr<CommandExecutor>& smartctl_ex = nullptr);

		/// Detects type, smart support, smart status (on / off).
		/// Note: this will clear all previous properties!
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> parse_basic_data();


		/// Execute smartctl --all / -x (all sections), get output, parse it (basic data too), fill properties.
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> fetch_full_data_and_parse(const std::shared_ptr<CommandExecutor>& smartctl_ex);

		/// Parse full info. If failed, try to parse it as basic info.
//		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> try_parse_data();

		/// Parse full info.
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> parse_full_data(SmartctlParserType parser_type, SmartctlOutputFormat format);

		/// Try to detect the data type and parse it. This is used when loading virtual drives.
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> parse_any_data_for_virtual();

		/// Get the "fully parsed" flag
		[[nodiscard]] ParseStatus get_parse_status() const;


		/// Try to enable SMART.
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> set_smart_enabled(bool b, const std::shared_ptr<CommandExecutor>& smartctl_ex);


		/// Read common properties (smart supported, smart enabled, etc.) from the repository.
		void read_common_properties();

		/// Detect drive type based on parsed properties.
		void detect_drive_type_from_properties(const StoragePropertyRepository& property_repo);


		/// Get SMART status
		[[nodiscard]] SmartStatus get_smart_status() const;

		/// Get if SMART on/off is supported
		[[nodiscard]] bool get_smart_switch_supported() const;


		/// Get format size string, or an empty string on error.
		[[nodiscard]] std::string get_device_size_str() const;

		/// Get the overall health property
		[[nodiscard]] StorageProperty get_health_property() const;


		/// Get device name (e.g. /dev/sda)
		[[nodiscard]] std::string get_device() const;

		/// Get device name without path. For example, "sda".
		[[nodiscard]] std::string get_device_base() const;

		/// Get device name for display purposes (with a type argument in parentheses)
		[[nodiscard]] std::string get_device_with_type() const;


		/// Set detected type
		void set_detected_type(StorageDeviceDetectedType t);

		/// Get detected type
		[[nodiscard]] StorageDeviceDetectedType get_detected_type() const;


		/// Set argument for "-d" smartctl parameter
		void set_type_argument(std::string arg);

		/// Get argument for "-d" smartctl parameter
		[[nodiscard]] std::string get_type_argument() const;


		/// Set extra arguments smartctl
		void set_extra_arguments(std::string args);

		/// Get extra arguments smartctl
		[[nodiscard]] std::string get_extra_arguments() const;


		/// Set windows drive letters for this drive
		void set_drive_letters(std::map<char, std::string> letters_volnames);

		/// Get windows drive letters for this drive
		[[nodiscard]] const std::map<char, std::string>& get_drive_letters() const;

		/// Get comma-separated win32 drive letters (if present)
		[[nodiscard]] std::string format_drive_letters(bool with_volnames) const;


		/// Get "virtual" status
		[[nodiscard]] bool get_is_virtual() const;

		/// If the device is virtual, return its file
		[[nodiscard]] hz::fs::path get_virtual_file() const;

		/// Get only the filename portion of a virtual file
		[[nodiscard]] std::string get_virtual_filename() const;


		/// Get properties
		[[nodiscard]] const StoragePropertyRepository& get_property_repository() const;


		/// Get model name.
		/// \return empty string if not found
		[[nodiscard]] std::string get_model_name() const;

		/// Get family name.
		/// \return empty string if not found
		[[nodiscard]] std::string get_family_name() const;

		/// Get serial number.
		/// \return empty string if not found
		[[nodiscard]] std::string get_serial_number() const;


		/// Set "info" output to parse
		void set_info_output(std::string s);

		/// Get "info" output to parse
		[[nodiscard]] std::string get_basic_output() const;


		/// Set "full" output to parse
		void set_full_output(std::string s);

		/// Get "full" output to parse
		[[nodiscard]] std::string get_full_output() const;


		/// Set "manually added" flag
		void set_is_manually_added(bool b);

		/// Get "manually added" flag
		[[nodiscard]] bool get_is_manually_added() const;


		/// Set "test is active" flag, emit the "changed" signal if needed.
		void set_test_is_active(bool b);

		/// Get "test is active" flag
		[[nodiscard]] bool get_test_is_active() const;


		/// Get the recommended filename to save output to. Includes model and date.
		[[nodiscard]] std::string get_save_filename() const;


		/// Get final smartctl options for this device from config and type info.
		[[nodiscard]] std::string get_device_options() const;


		/// Execute smartctl on this device. Nothing is modified in this class.
		[[nodiscard]] hz::ExpectedVoid<StorageDeviceError> execute_device_smartctl(const std::string& command_options,
				const std::shared_ptr<CommandExecutor>& smartctl_ex, std::string& output, bool check_type = false);


		/// Emitted whenever new information is available
		[[nodiscard]] sigc::signal<void, StorageDevice*>& signal_changed();


	protected:

		/// Set the "fully parsed" flag
		void set_parse_status(ParseStatus value);

		/// Set properties
		void set_property_repository(StoragePropertyRepository repository);


	private:

		std::string device_;  ///< e.g. /dev/sda or pd0. empty if virtual.
		std::string type_arg_;  ///< Device type (for -d smartctl parameter), as specified when adding the device.
		std::string extra_args_;  ///< Extra parameters for smartctl, as specified when adding the device.

		std::map<char, std::string> drive_letters_;  ///< Windows drive letters (if detected), with volume names

		bool is_virtual_ = false;  ///< If true, then this is not a real device - merely a loaded description of it.
		hz::fs::path virtual_file_;  ///< A file (smartctl data) the virtual device was loaded from
		bool is_manually_added_ = false;  ///< StorageDevice doesn't use it, but it's useful for its users.

		/// Sort of a "lock". If true, the device is not allowed to perform any commands
		/// except "-l selftest" and maybe "--capabilities" and "--info" (not sure).
		bool test_is_active_ = false;

		// Outputs
		std::string basic_output_;  ///< "smartctl --info" output
		std::string full_output_;  ///< "smartctl --all" or "-x" output

		StorageDeviceDetectedType detected_type_ = StorageDeviceDetectedType::Unknown;  ///< Detected by basic parser

		ParseStatus parse_status_ = ParseStatus::None;  ///< "Fully parsed" flag

		StoragePropertyRepository property_repository_;  ///< Parsed data properties

		// Common properties
		std::optional<bool> smart_supported_;  ///< SMART support status
		std::optional<bool> smart_enabled_;  ///< SMART enabled status
		std::optional<std::string> model_name_;  ///< Model name
		std::optional<std::string> family_name_;  ///< Family name
		std::optional<std::string> serial_number_;  ///< Serial number
		std::optional<std::string> size_;  ///< Formatted size
		mutable std::optional<StorageProperty> health_property_;  ///< Cached health property.


		/// Emitted whenever new information is available
		sigc::signal<void, StorageDevice*> signal_changed_;


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
