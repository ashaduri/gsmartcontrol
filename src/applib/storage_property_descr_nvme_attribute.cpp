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

#include <glibmm.h>
#include <utility>
#include <map>
#include <optional>
#include <string>

#include "hz/string_algo.h"  // string_replace_copy
//#include "applib/app_regex.h"

#include "storage_property_descr_nvme_attribute.h"
//#include "warning_colors.h"
//#include "storage_property_descr_helpers.h"


namespace {


	/// Attribute description for attribute database
	struct NvmeAttributeDescription {
		/// Constructor
		NvmeAttributeDescription() = default;

		/// Constructor
		NvmeAttributeDescription(std::string generic_name_, std::string description_)
				: generic_name(std::move(generic_name_)), description(std::move(description_))
		{ }

//		std::string reported_name;  ///< e.g. Highest Temperature
//		std::string displayable_name;  ///< e.g. Highest Temperature (C)
		std::string generic_name;  ///< Generic name to be set on the property.
		std::string description;  ///< Attribute description, can be "".
	};



	/// Devstat entry description database
	class NvmeAttributeDescriptionDatabase {
		public:

			/// Constructor
			NvmeAttributeDescriptionDatabase()
			{
				add("nvme_smart_health_information_log/temperature",
						_("Drive temperature (Celsius)"));

				add("nvme_smart_health_information_log/available_spare",
						_("Normalized percentage (0% to 100%) of the remaining space capacity. "
						  "If Available Spare is lower than Available Space Threshold, the drive is considered to be in a critical state."));

				add("nvme_smart_health_information_log/available_spare_threshold",
						_("Normalized percentage (0% to 100%). If the Available Spare is lower than this threshold, the drive is considered to be in a critical state."));

				add("nvme_smart_health_information_log/percentage_used",
						_("Vendor-specific estimate of the percentage of device life based on the actual device usage and the manufacturer's prediction of the device life. "
						  "A value of 100 indicates that the estimated endurance of the device has been consumed, but may not indicate a device failure. "
						  "This value is allowed to exceed 100. Percentage values greater than 254 are be represented as 255. This value is updated once "
						  "per power-on hour (when the controller is not in a sleep state)."));

				add("nvme_smart_health_information_log/data_units_read",
						_("The number of 512-byte data units the host has read from the controller. "
						  "This value does not include metadata. "
						  "The value is reported in thousands (i.e. a value of 1 corresponds to 1000 units of 512 bytes read) and is rounded up. "
						  "When the LBA size is a value other than 512 bytes, the controller converts the amount of data read to 512-byte units."));

				add("nvme_smart_health_information_log/data_units_written",
						_("The number of 512-byte data units the host has written to the controller. "
						  "This value does not include metadata. "
						  "The value is reported in thousands (i.e. a value of 1 corresponds to 1000 units of 512 bytes read) and is rounded up."));

				add("nvme_smart_health_information_log/host_reads",
						_("Number of read commands completed by the controller"));

				add("nvme_smart_health_information_log/host_writes",
						_("Number of write commands completed by the controller"));

				add("nvme_smart_health_information_log/controller_busy_time",
						_("The amount of time the controller is busy with I/O commands."));

				add("nvme_smart_health_information_log/power_cycles",
						_("Number of power cycles experienced by the drive"));

				add("nvme_smart_health_information_log/power_on_hours",
						_("Number of hours in power-on state. This does not include the time that the controller was powered in a low power state condition."));

				add("nvme_smart_health_information_log/unsafe_shutdowns",
						_("Number of unsafe shutdowns. This value is incremented when a shutdown notification is not received prior to loss of power."));

				add("nvme_smart_health_information_log/media_errors",
						_("Number of occurrences where the controller detected an unrecovered data integrity error. Errors such as uncorrectable ECC, "
						  "CRC checksum failure or LBA tag mismatch are included in this field."));

				add("nvme_smart_health_information_log/num_err_log_entries",
						_("Maximum number of possible Error Information Log entries preserved over the life of the controller"));

				/// FIXME unit?
				add("nvme_smart_health_information_log/warning_temp_time",
						_("The minimum Composite Temperature field value indicates an overheating condition during which the controller operation continues. "
						  "Immediate remediation is recommended (e.g. additional cooling or workload reduction)."));

				add("nvme_smart_health_information_log/critical_comp_time",
						_("The amount of time in minutes that the controller is operational and the Composite Temperature is >= Critical Composite Temperature Threshold (CCTEMP)."));
			}


			/// Add an attribute description to the attribute database.
			/// Note: Displayable names for known attributes have already been added while parsing.
			/// For unknown attributes, the displayable name is derived from generic name.
			void add(const std::string& generic_name, const std::string& description)
			{
				add(NvmeAttributeDescription(generic_name, description));
			}


			/// Add an devstat entry description to the devstat database
			void add(const NvmeAttributeDescription& descr)
			{
				nvme_attr_db[descr.generic_name] = descr;
			}


			/// Find the description by smartctl generic name.
			[[nodiscard]] NvmeAttributeDescription find(const std::string& reported_name) const
			{
				// search by ID first
				auto iter = nvme_attr_db.find(reported_name);
				if (iter == nvme_attr_db.end()) {
					return {};  // not found
				}
				return iter->second;
			}


		private:

			std::map<std::string, NvmeAttributeDescription> nvme_attr_db;  ///< generic_name => description

	};



	/// Get program-wide devstat description database
	[[nodiscard]] inline const NvmeAttributeDescriptionDatabase& get_nvme_attribute_description_db()
	{
		static const NvmeAttributeDescriptionDatabase nvme_attr_db;
		return nvme_attr_db;
	}



	/// Check if a property matches a name (generic or reported)
	inline bool name_match(StorageProperty& p, const std::string& name)
	{
		if (p.generic_name.empty()) {
			return hz::string_to_lower_copy(p.reported_name) == hz::string_to_lower_copy(name);
		}
		return hz::string_to_lower_copy(p.generic_name) == hz::string_to_lower_copy(name);
	}


}


/// Find a property's statistic in the statistics database and fill the property
/// with all the readable information we can gather.
bool auto_set_nvme_attribute_description(StorageProperty& p)
{
	NvmeAttributeDescription attr_descr = get_nvme_attribute_description_db().find(p.generic_name);

//		const std::string displayable_name = (attr_descr.displayable_name.empty() ? attr_descr.reported_name : attr_descr.displayable_name);

	const bool found = !attr_descr.description.empty();
	if (!found) {
		// Derive displayable name from generic name
//			p.displayable_name = hz::string_replace_copy(p.generic_name, "_", " ");

		attr_descr.description = "No description is available for this attribute.";

	} else {
		std::string descr =  std::string("<b>") + Glib::Markup::escape_text(p.displayable_name) + "</b>\n";
		descr += attr_descr.description;

		attr_descr.description = descr;
	}

	p.set_description(attr_descr.description);
//		p.generic_name = attr_descr.generic_name;

	return found;
}





void storage_property_nvme_attribute_autoset_warning(StorageProperty& p)
{
	std::optional<WarningLevel> w = WarningLevel::None;
	std::string reason;

	if (p.section != StoragePropertySection::NvmeAttributes) {
		return;
	}

	if (name_match(p, "nvme_smart_health_information_log/temperature") && p.is_value_type<int64_t >() && p.get_value<int64_t>() > 50) {  // 50C
		w = WarningLevel::Notice;
		reason = "The temperature of the drive is higher than 50 degrees Celsius. "
				"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

	} else if (name_match(p, "nvme_smart_health_information_log/available_spare") && p.is_value_type<int64_t >()
			&& p.get_value<int64_t>() <= 10) {  // 10% (arbitrary value)
		w = WarningLevel::Warning;
		reason = "The drive has less than 10% available spare lifetime left.";

	} else if (name_match(p, "nvme_smart_health_information_log/percentage_used") && p.is_value_type<int64_t >()
			&& p.get_value<int64_t>() >= 90) {  // 90% (arbitrary value)
		w = WarningLevel::Warning;
		reason = "The estimate drive lifetime is nearing its limit.";

	} else if (name_match(p, "nvme_smart_health_information_log/media_errors") && p.is_value_type<int64_t >()
			&& p.get_value<int64_t>() > 0) {
		w = WarningLevel::Notice;
		reason = "There are media errors present on this drive.";

//	} else if (name_match(p, "nvme_smart_health_information_log/num_err_log_entries") && p.is_value_type<int64_t >()
//			&& p.get_value<int64_t>() > 0) {
//		w = WarningLevel::Warning;
//		reason = "The drive has errors in its persistent error log.";

	} else if (name_match(p, "nvme_smart_health_information_log/warning_temp_time") && p.is_value_type<int64_t >()
			&& p.get_value<int64_t>() > 0) {
		w = WarningLevel::Notice;
		reason = "The drive detected is or was overheating. "
				 "This may have shortened its lifespan and caused damage. Please install a cooling solution.";

	} else if (name_match(p, "nvme_smart_health_information_log/critical_comp_time") && p.is_value_type<int64_t >()
			&& p.get_value<int64_t>() > 0) {
		w = WarningLevel::Notice;
		reason = "The drive detected is or was overheating. "
				 "This may have shortened its lifespan and caused damage. Please install a cooling solution.";
	}

	if (w.has_value()) {
		p.warning_level = w.value();
		p.warning_reason = reason;
	}
}





/// @}
