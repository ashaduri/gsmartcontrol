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

#ifndef STORAGE_PROPERTY_H
#define STORAGE_PROPERTY_H

#include "local_glibmm.h"

#include <cstddef>  // std::size_t
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iosfwd>
#include <cstdint>
#include <optional>
#include <chrono>
#include <variant>

#include "warning_level.h"
#include "hz/enum_helper.h"



/// Holds one block of "capabilities" subsection (only for non-time-interval blocks).
/// ATA Text parser only.
class AtaStorageTextCapability {
	public:
		std::string reported_flag_value;  ///< original flag value as a string
		uint16_t flag_value = 0x0;  ///< Flag value. This is one or sometimes two bytes (maybe more?)
		std::string reported_strvalue;  ///< Original flag descriptions
		std::vector<std::string> strvalues;  ///< A list of capabilities in the block.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const AtaStorageTextCapability& p);





/// Holds one line of "attributes" subsection.
/// ATA only.
class AtaStorageAttribute {
	public:

		/// Disk type the attribute may match
//		enum class DiskType {
//			Any,  ///< Any disk type
//			Hdd,  ///< HDD (rotational) only
//			Ssd  ///< SSD only
//		};

		/// Attribute pre-failure / old-age type
		enum class AttributeType {
			Unknown,  ///< Unknown
			Prefail,  ///< Pre-failure (reported: Pre-fail)
			OldAge  ///< Old age (reported: Old_age)
		};

		/// Get readable attribute type name
		[[nodiscard]] static std::string get_readable_attribute_type_name(AttributeType type);


		/// Attribute when-updated type
		enum class UpdateType {
			Unknown,  ///< Unknown
			Always,  ///< Continuously (reported: Always)
			Offline  ///< Only during offline data collection (reported: Offline)
		};

		/// Get readable when-updated type name
		[[nodiscard]] static std::string get_readable_update_type_name(UpdateType type);


		/// Attribute when-failed type
		enum class FailTime {
			Unknown,  ///< Unknown
			None,  ///< Never (reported: -)
			Past,  ///< In the past (reported: In_the_past)
			Now  ///< Now (reported: FAILING_NOW)
		};

		/// Get a readable when-failed type name
		[[nodiscard]] static std::string get_readable_fail_time_name(FailTime type);


		/// Format raw value with commas (if it's a number)
		[[nodiscard]] std::string format_raw_value() const;


		std::int32_t id = -1;  ///< Attribute ID (most vendors agree on this)
		std::string flag;  ///< "Old" format is "0xXXXX", "brief" format is "PO--C-".
		std::optional<uint8_t> value;  ///< Normalized value. May be unset ("---").
		std::optional<uint8_t> worst;  ///< Worst ever value. May be unset ("---").
		std::optional<uint8_t> threshold;  ///< Threshold for normalized value. May be unset ("---").
		AttributeType attr_type = AttributeType::Unknown;  ///< Attribute pre-fail / old-age type
		UpdateType update_type = UpdateType::Unknown;  ///< When-updated type
		FailTime when_failed = FailTime::Unknown;  ///< When-failed type
		std::string raw_value;  ///< Raw value as a string, as presented by smartctl (formatted).
		std::int64_t raw_value_int = 0;  ///< Same as raw_value, but parsed as int64. original value is 6 bytes I think.

};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const AtaStorageAttribute& p);




/// Holds one line of "devstat" subsection.
/// ATA only.
class AtaStorageStatistic {
	public:

		/// Whether the normalization flag is present
		[[nodiscard]] bool is_normalized() const;

		/// Format value with commas (if it's a number)
		[[nodiscard]] std::string format_value() const;

		bool is_header = false;  ///< If the line is a header
		std::string flags;  ///< Flags in "NDC" / "---" format
		std::string value;  ///< Value as a string, as presented by smartctl (formatted).
		std::int64_t value_int = 0;  ///< Same as value, but parsed as int64.
		std::int64_t page = 0;  ///< Page
		std::int64_t offset = 0;  ///< Offset in page
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const AtaStorageStatistic& p);



/// Holds one error block of "error log" subsection.
/// ATA only.
class AtaStorageErrorBlock {
	public:

		/// Get readable error types from reported types
		[[nodiscard]] static std::string format_readable_error_types(const std::vector<std::string>& types);

		/// Get warning level (Warning) for an error type
		[[nodiscard]] static WarningLevel get_warning_level_for_error_type(const std::string& type);


		std::uint32_t error_num = 0;  ///< Error number
		std::uint64_t log_index = 0;  ///< Log index
		std::uint32_t lifetime_hours = 0;  ///< When the error occurred (in lifetime hours)
		std::string device_state;  ///< Device state during the error - "active or idle", standby, etc.
		std::vector<std::string> reported_types;  ///< Array of reported types (strings), e.g. "UNC".
		std::string type_more_info;  ///< More info on error type (e.g. "at LBA = 0x0253eac0 = 39054016")
		std::uint64_t lba = 0;  ///< LBA of the error
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const AtaStorageErrorBlock& b);




/// Holds one entry of selftest_log subsection.
/// Also, holds "Self-test execution status" capability's "internal" section version.
/// ATA only.
class AtaStorageSelftestEntry {
	public:

		/// Self-test log entry status.
		enum class Status {
			Unknown = -1,  ///< Initial state
			Reserved = -2,  ///< Reserved
			// Values correspond to (ata_smart_self_test_log/extended/table/status/value >> 4).
			CompletedNoError = 0x0,  ///< Completed with no error, or no test was run
			AbortedByHost = 0x1,  ///< Aborted by host
			Interrupted = 0x2,  ///< Interrupted by user
			FatalOrUnknown = 0x3,  ///< Fatal or unknown error. Treated as test failure.
			ComplUnknownFailure = 0x4,  ///< Completed with unknown error. Treated as test failure.
			ComplElectricalFailure = 0x5,  ///< Completed with electrical error. Treated as test failure.
			ComplServoFailure = 0x6,  ///< Completed with servo error. Treated as test failure.
			ComplReadFailure = 0x7,  ///< Completed with read error. Treated as test failure.
			ComplHandlingDamage = 0x8,  ///< Completed with handling damage error. Treated as test failure.
			InProgress = 0xf,  ///< Test in progress
		};

		/// Self-test error severity
		enum class StatusSeverity {
			None,
			Warning,
			Error
		};

		/// Get log entry status displayable name
		[[nodiscard]] static std::string get_readable_status_name(Status s);

		/// Get severity of error status
		[[nodiscard]] static StatusSeverity get_status_severity(Status s);


		/// Get error status as a string
		[[nodiscard]] std::string get_readable_status() const;


		std::uint32_t test_num = 0;  ///< Test number. always starts from 1. larger means older or newer, depending on model. 0 for capability.
		std::string type;  ///< Extended offline, Short offline, Conveyance offline, etc. . capability: unused.
		std::string status_str;  ///< Self-test routine in progress, Completed without error, etc. (as reported by log or capability)
		Status status = Status::Unknown;  ///< same as status_str, but from enum
		std::int8_t remaining_percent = -1;  ///< Remaining %. 0% for completed, 90% for started. -1 if n/a.
		std::uint32_t lifetime_hours = 0;  ///< When the test happened (in lifetime hours). capability: unused.
		std::string lba_of_first_error;  ///< LBA of the first error. "-" or value (format? usually hex). capability: unused.
		bool passed = false;  ///< Test passed or not.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const AtaStorageSelftestEntry& b);




/// Decoded of nvme_self_test_log/current_self_test_operation/value
enum class NvmeSelfTestCurrentOperationType {
	Unknown = -1,
	None = 0x0,
	ShortInProgress = 0x1,
	ExtendedInProgress = 0x2,
	VendorSpecificInProgress = 0xe,
};



/// Helper structure for enum-related functions
struct NvmeSelfTestCurrentOperationTypeExt
		: public hz::EnumHelper<
				NvmeSelfTestCurrentOperationType,
				NvmeSelfTestCurrentOperationTypeExt,
				Glib::ustring>
{
	static constexpr NvmeSelfTestCurrentOperationType default_value = NvmeSelfTestCurrentOperationType::Unknown;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{NvmeSelfTestCurrentOperationType::Unknown, {"unknown", _("Unknown")}},
			{NvmeSelfTestCurrentOperationType::None, {"none", _("None")}},
			{NvmeSelfTestCurrentOperationType::ShortInProgress, {"shortInProgress", _("Short Test in Progress")}},
			{NvmeSelfTestCurrentOperationType::ExtendedInProgress, {"extendedInProgress", _("Extended Test in Progress")}},
			{NvmeSelfTestCurrentOperationType::VendorSpecificInProgress, {"vendorSpecificInProgress", _("Vendor-Specific Test in Progress")}},
		};
	}
};



/// Self-test types in log
enum class NvmeSelfTestType {
	Unknown = -1,
	Short = 0x1,
	Extended = 0x2,
	VendorSpecific = 0xe,  ///< Can be encountered in log
};



/// Helper structure for enum-related functions
struct NvmeSelfTestTypeExt
		: public hz::EnumHelper<
				NvmeSelfTestType,
				NvmeSelfTestTypeExt,
				Glib::ustring>
{
	static constexpr NvmeSelfTestType default_value = NvmeSelfTestType::Unknown;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{NvmeSelfTestType::Unknown, {"unknown", _("Unknown")}},
			{NvmeSelfTestType::Short, {"short", _("Short Test")}},
			{NvmeSelfTestType::Extended, {"extended", _("Extended Test")}},
			{NvmeSelfTestType::VendorSpecific, {"vendorSpecific", _("Vendor-Specific Test")}},
		};
	}
};



/// Self-test log entry status.
enum class NvmeSelfTestResultType {
	Unknown = -1,  ///< Unknown result
	// Values correspond to "nvme_self_test_log/table/self_test_result/value".
	CompletedNoError = 0x0,  ///< Completed with no error
	AbortedSelfTestCommand = 0x1,  ///< Aborted: Self-test command (manually aborted)
	AbortedControllerReset = 0x2,  ///< Aborted: Controller Reset
	AbortedNamespaceRemoved = 0x3,  ///< Aborted: Namespace removed
	AbortedFormatNvmCommand = 0x4,  ///< Aborted: Format NVM command
	FatalOrUnknownTestError = 0x5,  ///< Fatal or unknown test error
	CompletedUnknownFailedSegment = 0x6,  ///< Completed: unknown failed segment
	CompletedFailedSegments = 0x7,  ///< Completed: failed segments
	AbortedUnknownReason = 0x8,  ///< Aborted: unknown reason
	AbortedSanitizeOperation = 0x9,  ///< Aborted: sanitize operation
};



/// Helper structure for enum-related functions
struct NvmeSelfTestResultTypeExt
		: public hz::EnumHelper<
				NvmeSelfTestResultType,
				NvmeSelfTestResultTypeExt,
				Glib::ustring>
{
	static constexpr NvmeSelfTestResultType default_value = NvmeSelfTestResultType::Unknown;

	static std::unordered_map<EnumType, std::pair<std::string, Glib::ustring>> build_enum_map()
	{
		return {
			{NvmeSelfTestResultType::Unknown, {"unknown", _("Unknown")}},
			{NvmeSelfTestResultType::CompletedNoError, {"completedNoError", _("Completed with No Error")}},
			{NvmeSelfTestResultType::AbortedSelfTestCommand, {"abortedSelfTestCommand", _("Aborted: Self-Test Command")}},
			{NvmeSelfTestResultType::AbortedControllerReset, {"abortedControllerReset", _("Aborted: Controller Reset")}},
			{NvmeSelfTestResultType::AbortedNamespaceRemoved, {"abortedNamespaceRemoved", _("Aborted: Namespace Removed")}},
			{NvmeSelfTestResultType::AbortedFormatNvmCommand, {"abortedFormatNvmCommand", _("Aborted: Format NVM Command")}},
			{NvmeSelfTestResultType::FatalOrUnknownTestError, {"fatalOrUnknownTestError", _("Fatal or Unknown Test Error")}},
			{NvmeSelfTestResultType::CompletedUnknownFailedSegment, {"completedUnknownFailedSegment", _("Completed: Unknown Failed Segment")}},
			{NvmeSelfTestResultType::CompletedFailedSegments, {"completedFailedSegments", _("Completed: Failed Segments")}},
			{NvmeSelfTestResultType::AbortedUnknownReason, {"abortedUnknownReason", _("Aborted: Unknown Reason")}},
			{NvmeSelfTestResultType::AbortedSanitizeOperation, {"abortedSanitizeOperation", _("Aborted: Sanitize Operation")}},
		};
	}
};



/// Holds one entry of nvme_self_test_log section.
/// NVMe only.
class NvmeStorageSelftestEntry {
	public:

		/// Self-test error severity
		enum class StatusSeverity {
			None,
			Warning,
			Error
		};

		std::uint32_t test_num = 0;  ///< Test number, auto-generated
		NvmeSelfTestType type = NvmeSelfTestType::Unknown;  ///< Test type
		NvmeSelfTestResultType result = NvmeSelfTestResultType::Unknown;  ///< Test result
		std::uint32_t power_on_hours = 0;  ///< When the test happened (in power-on hours).
		std::optional<std::uint64_t> lba;  ///< LBA of the first error.
};


/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const NvmeStorageSelftestEntry& b);



/// Sections in output
enum class StoragePropertySection {
	Unknown,  ///< Used when searching in all sections
	Info,  ///< Short info (--info)
	OverallHealth,  ///< Overall-health (-H, --health)
	Capabilities,  ///< General SMART Values, aka Capabilities (-c, --capabilities)
	AtaAttributes,  ///< Attributes (-A, --attributes). These need decoding.
	Statistics,  ///< Device statistics (--log=devstat). These need decoding.
	AtaErrorLog,  ///< Error Log (--log=error)
	SelftestLog,  ///< Self-test log (--log=selftest)
	SelectiveSelftestLog,  ///< Selective self-test log (--log=selective)
	TemperatureLog,  ///< SCT temperature (current and history) (--log=scttemp)
	ErcLog,  ///< SCT Error Recovery Control settings (--log=scterc)
	PhyLog,  ///< Phy log (--log=sataphy)
	DirectoryLog,  ///< Directory log (--log=directory)
	NvmeHealth,  ///< NVMe health (-H, --health)
	NvmeAttributes,  ///< NVMe attributes (health log) (-A, --attributes)
	NvmeErrorLog,  ///< NVMe error log (--log=error)
};



/// Helper structure for enum-related functions
struct StoragePropertySectionExt
		: public hz::EnumHelper<
				StoragePropertySection,
				StoragePropertySectionExt,
				std::string>
{
	static constexpr StoragePropertySection default_value = StoragePropertySection::Unknown;

	static std::unordered_map<EnumType, std::pair<std::string, std::string>> build_enum_map()
	{
		return {
			{StoragePropertySection::Unknown,              {"unknown",              _("Unknown")}},
			{StoragePropertySection::Info,                 {"info",                 _("Short Info")}},
			{StoragePropertySection::OverallHealth,        {"overallHealth",        _("Overall Health")}},
			{StoragePropertySection::Capabilities,         {"capabilities",         _("Capabilities")}},
			{StoragePropertySection::AtaAttributes,        {"attributes",           _("Attributes")}},
			{StoragePropertySection::Statistics,           {"statistics",           _("Statistics")}},
			{StoragePropertySection::AtaErrorLog,          {"errorLog",             _("Error Log")}},
			{StoragePropertySection::SelftestLog,          {"selftestLog",          _("Self-test Log")}},
			{StoragePropertySection::SelectiveSelftestLog, {"selectiveSelftestLog", _("Selective Self-test Log")}},
			{StoragePropertySection::TemperatureLog,       {"temperatureLog",       _("Temperature Log")}},
			{StoragePropertySection::ErcLog,               {"ercLog",               _("Error Recovery Control Log")}},
			{StoragePropertySection::PhyLog,               {"phyLog",               _("Phy Log")}},
			{StoragePropertySection::DirectoryLog,         {"directoryLog",         _("Directory Log")}},
			{StoragePropertySection::NvmeHealth,           {"nvmeHealth",           _("NVMe Health")}},
			{StoragePropertySection::NvmeAttributes,       {"nvmeAttributes",       _("NVMe Attributes")}},
			{StoragePropertySection::NvmeErrorLog,         {"nvmeErrorLog",         _("NVMe Error Log")}},
		};
	}
};





/// A single parser-extracted property
class StorageProperty {
	public:

		using ValueVariantType = std::variant<
			std::monostate,  ///< None
			std::string,  ///< Value (if it's a string)
			std::int64_t,   ///< Value (if it's an integer)
			bool,  ///< Value (if it's bool)
			std::chrono::seconds,  ///< Value in seconds (if it's time interval)
			AtaStorageTextCapability,  ///< Value (if it's a capability)
			AtaStorageAttribute,  ///< Value (if it's an attribute)
			AtaStorageStatistic,  ///< Value (if it's a statistic from devstat)
			AtaStorageErrorBlock,  ///< Value (if it's a error block)
			AtaStorageSelftestEntry,  ///< Value (if it's ATA self-test log entry)
			NvmeStorageSelftestEntry  ///< Value (if it's NVMe self-test log entry)
		>;


		/// Constructor
		StorageProperty() = default;

		/// Constructor
		StorageProperty(StoragePropertySection section_, ValueVariantType value_)
				: section(section_), value(std::move(value_))
		{ }


		/// Get displayable value type name
		[[nodiscard]] std::string get_storable_value_type_name() const;


		/// Check if this is an empty object with no value set.
		[[nodiscard]] bool empty() const;


		/// Dump the property to a stream for debugging purposes
		void dump(std::ostream& os, std::size_t internal_offset = 0) const;


		/// Format this property for debugging purposes
		[[nodiscard]] std::string format_value(bool add_reported_too = false) const;


		/// Get value of type T
		template<typename T>
		[[nodiscard]] const T& get_value() const;


		/// Check if value is of type T
		template<typename T>
		[[nodiscard]] bool is_value_type() const;


		/// Get property description (used in tooltips)
		[[nodiscard]] std::string get_description(bool clean = false) const;


		/// Set property description (used in tooltips)
		void set_description(const std::string& descr);


		/// Set smartctl-reported name, generic (internal) name, readable name
		void set_name(const std::string& rep_name, const std::string& gen_name = "", const std::string& read_name = "");


		std::string reported_name;  ///< Property name as reported by smartctl. Mainly used by Text parser.
		std::string generic_name;  ///< Generic (internal) name. May be same as reported_name, or something more program-identifiable.
		std::string displayable_name;  ///< Readable property name. May be same as reported_name, or something more user-readable. Possibly translatable.

		std::string description;  ///< Property description (for tooltips, etc.). May contain markup.

		StoragePropertySection section = StoragePropertySection::Unknown;  ///< Section this property belongs to

		std::string reported_value;  ///< String representation of the value as reported
		std::string readable_value;  ///< User-friendly readable representation of value. if empty, use the other members.

		ValueVariantType value;  ///< Stored value

		WarningLevel warning_level = WarningLevel::None;  ///< Warning severity for this property
		std::string warning_reason;  // Warning reason (displayable)

		bool show_in_ui = true;  ///< Whether to show this property in UI or not

};




/// Output operator for debug purposes
std::ostream& operator<< (std::ostream& os, const StorageProperty& p);





// ------------------------------------------- Implementation



template<typename T>
const T& StorageProperty::get_value() const
{
	return std::get<T>(value);
}



template<typename T>
bool StorageProperty::is_value_type() const
{
	return std::holds_alternative<T>(value);
}





#endif

/// @}
