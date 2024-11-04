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
//#include <vector>
#include <map>
//#include <unordered_map>

#include "hz/string_algo.h"  // string_replace_copy
#include "applib/app_regex.h"

#include "storage_property_descr_ata_statistic.h"
//#include "warning_colors.h"
#include "storage_property_descr_helpers.h"


namespace {


	/// Attribute description for attribute database
	struct AtaStatisticDescription {
		/// Constructor
		AtaStatisticDescription() = default;

		/// Constructor
		AtaStatisticDescription(std::string reported_name_,
				std::string displayable_name_, std::string generic_name_, std::string description_)
				: reported_name(std::move(reported_name_)), displayable_name(std::move(displayable_name_)),
				generic_name(std::move(generic_name_)), description(std::move(description_))
		{ }

		std::string reported_name;  ///< e.g. Highest Temperature
		std::string displayable_name;  ///< e.g. Highest Temperature (C)
		std::string generic_name;  ///< Generic name to be set on the property.
		std::string description;  ///< Attribute description, can be "".
	};



	/// Devstat entry description database
	class AtaStatisticDescriptionDatabase {
		public:

			/// Constructor
			AtaStatisticDescriptionDatabase()
			{
				// See http://www.t13.org/documents/UploadedDocuments/docs2016/di529r14-ATAATAPI_Command_Set_-_4.pdf

				// General Statistics

				add("Lifetime Power-On Resets", "", "",
						"The number of times the device has processed a power-on reset.");

				add("Power-on Hours", "", "",
						"The amount of time that the device has been operational since it was manufactured.");

				add("Logical Sectors Written", "", "",
						"The number of logical sectors received from the host. "
						"This statistic is incremented by one for each logical sector that was received from the host without an error.");

				add("Number of Write Commands", "", "",
						"The number of write commands that returned command completion without an error. "
						"This statistic is incremented by one for each write command that returns command completion without an error.");

				add("Logical Sectors Read", "", "",
						"The number of logical sectors sent to the host. "
						"This statistic is incremented by one for each logical sector that was sent to the host without an error.");

				add("Number of Read Commands", "", "",
						"The number of read commands that returned command completion without an error. "
						"This statistic is incremented by one for each read command that returns command completion without an error.");

				add("Date and Time TimeStamp", "", "",
						"a) the TimeStamp set by the most recent SET DATE &amp; TIME EXT command plus the number of "
						"milliseconds that have elapsed since that SET DATE &amp; TIME EXT command was processed;\n"
						"or\n"
						"b) a copy of the Power-on Hours statistic (see A.5.4.4) with the hours unit of measure changed to milliseconds as described");

				add("Pending Error Count", "", "",
						"The number of logical sectors listed in the Pending Errors log.");

				add("Workload Utilization", "", "",
						"An estimate of device utilization as a percentage of the manufacturer's designs for various wear factors "
						"(e.g., wear of the medium, head load events), if any. The reported value can be greater than 100%.");

				add("Utilization Usage Rate", "", "",
						"An estimate of the rate at which device wear factors (e.g., damage to the recording medium) "
						"are being used during a specified interval of time. This statistic is expressed as a percentage of the manufacturer's designs.");

				// Free-Fall Statistics

				add("Number of Free-Fall Events Detected", "", "",
						"The number of free-fall events detected by the device.");

				add("Overlimit Shock Events", "", "",
						"The number of shock events detected by the device "
						"with the magnitude higher than the maximum rating of the device.");

				// Rotating Media Statistics

				add("Spindle Motor Power-on Hours", "", "",
						"The amount of time that the spindle motor has been powered on since the device was manufactured. ");

				add("Head Flying Hours", "", "",
						"The number of hours that the device heads have been flying over the surface of the media since the device was manufactured. ");

				add("Head Load Events", "", "",
						"The number of head load events. A head load event is defined as:\n"
						"a) when the heads are loaded from the ramp to the media for a ramp load device;\n"
						"or\n"
						"b) when the heads take off from the landing zone for a contact start stop device.");

				add("Number of Reallocated Logical Sectors", "", "",
						"The number of logical sectors that have been reallocated after device manufacture.\n\n"
						"If the value is normalized, this is the whole number percentage of the available logical sector reallocation "
						"resources that have been used (i.e., 0-100)."
						"\n\n" + get_suffix_for_uncorrectable_property_description());

				add("Read Recovery Attempts", "", "",
						"The number of logical sectors that require three or more attempts to read the data from the media for each read command. "
						"This statistic is incremented by one for each logical sector that encounters a read recovery attempt. "
						"These events may be caused by external environmental conditions (e.g., operating in a moving vehicle).");

				add("Number of Mechanical Start Failures", "", "",
						"The number of mechanical start failures after device manufacture. "
						"A mechanical start failure is a failure that prevents the device from achieving a normal operating condition");

				add("Number of Realloc. Candidate Logical Sectors", "Number of Reallocation Candidate Logical Sectors", "",
						"The number of logical sectors that are candidates for reallocation. "
						"A reallocation candidate sector is a logical sector that the device has determined may need to be reallocated."
						"\n\n" + get_suffix_for_uncorrectable_property_description());

				add("Number of High Priority Unload Events", "", "",
						"The number of emergency head unload events.");

				// General Errors Statistics

				add("Number of Reported Uncorrectable Errors", "", "",
						"The number of errors that are reported as an Uncorrectable Error. "
						"Uncorrectable errors that occur during background activity shall not be counted. "
						"Uncorrectable errors reported by reads to flagged uncorrectable logical blocks should not be counted"
						"\n\n" + get_suffix_for_uncorrectable_property_description());

				add("Resets Between Cmd Acceptance and Completion", "", "",
						"The number of software reset or hardware reset events that occur while one or more commands have "
						"been accepted by the device but have not reached command completion.");

				// Temperature Statistics

				add("Current Temperature", "Current Temperature (C)", "",
						"Drive temperature (Celsius)");

				add("Average Short Term Temperature", "Average Short Term Temperature (C)", "",
						"A value based on the most recent 144 temperature samples in a 24 hour period.");

				add("Average Long Term Temperature", "Average Long Term Temperature (C)", "",
						"A value based on the most recent 42 Average Short Term Temperature values (1,008 recorded hours).");

				add("Highest Temperature", "Highest Temperature (C)", "",
						"The highest temperature measured after the device is manufactured.");

				add("Lowest Temperature", "Lowest Temperature (C)", "",
						"The lowest temperature measured after the device is manufactured.");

				add("Highest Average Short Term Temperature", "Highest Average Short Term Temperature (C)", "",
						"The highest device Average Short Term Temperature after the device is manufactured.");

				add("Lowest Average Short Term Temperature", "Lowest Average Short Term Temperature (C)", "",
						"The lowest device Average Short Term Temperature after the device is manufactured.");

				add("Highest Average Long Term Temperature", "Highest Average Long Term Temperature (C)", "",
						"The highest device Average Long Term Temperature after the device is manufactured.");

				add("Lowest Average Long Term Temperature", "Lowest Average Long Term Temperature (C)", "",
						"The lowest device Average Long Term Temperature after the device is manufactured.");

				add("Time in Over-Temperature", "Time in Over-Temperature (Minutes)", "",
						"The number of minutes that the device has been operational while the device temperature specification has been exceeded.");

				add("Specified Maximum Operating Temperature", "Specified Maximum Operating Temperature (C)", "",
						"The maximum operating temperature device is designed to operate.");

				add("Time in Under-Temperature", "Time in Under-Temperature (C)", "",
						"The number of minutes that the device has been operational while the temperature is lower than the device minimum temperature specification.");

				add("Specified Minimum Operating Temperature", "Specified Minimum Operating Temperature (C)", "",
						"The minimum operating temperature device is designed to operate.");

				// Transport Statistics

				add("Number of Hardware Resets", "", "",
						"The number of hardware resets received by the device.");

				add("Number of ASR Events", "", "",
						"The number of ASR (Asynchronous Signal Recovery) events.");

				add("Number of Interface CRC Errors", "", "",
						"the number of Interface CRC (checksum) errors reported in the ERROR field since the device was manufactured.");

				// Solid State Device Statistics

				add("Percentage Used Endurance Indicator", "", "",
						"A vendor specific estimate of the percentage of device life used based on the actual device usage "
						"and the manufacturer's prediction of device life. A value of 100 indicates that the estimated endurance "
						"of the device has been consumed, but may not indicate a device failure (e.g., minimum "
						"power-off data retention capability reached for devices using NAND flash technology).");

			}


			/// Add an attribute description to the attribute database
			void add(const std::string& reported_name, const std::string& displayable_name,
					const std::string& generic_name, const std::string& description)
			{
				add(AtaStatisticDescription(reported_name, displayable_name, generic_name, description));
			}


			/// Add an devstat entry description to the devstat database
			void add(const AtaStatisticDescription& descr)
			{
				devstat_db[descr.reported_name] = descr;
			}


			/// Find the description by smartctl name or id, merging them if they're partial.
			[[nodiscard]] AtaStatisticDescription find(const std::string& reported_name) const
			{
				// search by ID first
				auto iter = devstat_db.find(reported_name);
				if (iter == devstat_db.end()) {
					return {};  // not found
				}
				return iter->second;
			}


		private:

			std::map<std::string, AtaStatisticDescription> devstat_db;  ///< reported_name => devstat entry description

	};



	/// Get program-wide devstat description database
	[[nodiscard]] inline const AtaStatisticDescriptionDatabase& get_ata_statistic_description_db()
	{
		static const AtaStatisticDescriptionDatabase devstat_db;
		return devstat_db;
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
bool auto_set_ata_statistic_description(StorageProperty& p)
{
	AtaStatisticDescription sd = get_ata_statistic_description_db().find(p.reported_name);

	const std::string displayable_name = (sd.displayable_name.empty() ? sd.reported_name : sd.displayable_name);

	const bool found = !sd.description.empty();
	if (!found) {
		sd.description = "No description is available for this entry.";

	} else {
		std::string descr =  std::string("<b>") + Glib::Markup::escape_text(displayable_name) + "</b>\n";
		descr += sd.description;

		if (p.get_value<AtaStorageStatistic>().is_normalized()) {
			descr += "\n\nNote: The value is normalized.";
		}

		sd.description = descr;
	}

	if (!displayable_name.empty()) {
		p.displayable_name = displayable_name;
	}
	p.set_description(sd.description);
	p.generic_name = sd.generic_name;

	return found;
}



void storage_property_ata_statistic_autoset_warning(StorageProperty& p)
{
	std::optional<WarningLevel> w = WarningLevel::None;
	std::string reason;

	if (p.section == StoragePropertySection::Statistics && p.is_value_type<AtaStorageStatistic>()) {
		const auto& statistic = p.get_value<AtaStorageStatistic>();

		if (name_match(p, "Pending Error Count") && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

		// "Workload Utilization" is either normalized, or encodes several values, so we can't use it.
/*
		} else if (name_match(p, "Workload Utilization") && statistic.value_int >= 50) {
			w = WarningLevel::notice;
			reason = "The drive has less than half of its estimated life left.";

		} else if (name_match(p, "Workload Utilization") && statistic.value_int >= 100) {
			w = WarningLevel::warning;
			reason = "The drive is past its estimated lifespan.";
*/

		} else if (name_match(p, "Utilization Usage Rate") && statistic.value_int >= 50) {
			w = WarningLevel::Notice;
			reason = "The drive has less than half of its estimated life left.";

		} else if (name_match(p, "Utilization Usage Rate") && statistic.value_int >= 100) {
			w = WarningLevel::Warning;
			reason = "The drive is past its estimated lifespan.";

		} else if (name_match(p, "Number of Reallocated Logical Sectors") && !statistic.is_normalized() && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

		} else if (name_match(p, "Number of Reallocated Logical Sectors") && statistic.is_normalized() && statistic.value_int <= 0) {
			w = WarningLevel::Warning;
			reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

		} else if (name_match(p, "Number of Mechanical Start Failures") && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The drive is reporting mechanical errors.";

		} else if (name_match(p, "Number of Realloc. Candidate Logical Sectors") && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

		} else if (name_match(p, "Number of Reported Uncorrectable Errors") && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The drive is reporting surface errors. This could be an indication of future failures and/or potential data loss in bad sectors.";

		} else if (name_match(p, "Current Temperature") && statistic.value_int > 50) {
			w = WarningLevel::Notice;
			reason = "The temperature of the drive is higher than 50 degrees Celsius. "
					"This may shorten its lifespan and cause damage under severe load. Please install a cooling solution.";

		} else if (name_match(p, "Time in Over-Temperature") && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The temperature of the drive is or was over the manufacturer-specified maximum. "
					"This may have shortened its lifespan and caused damage. Please install a cooling solution.";

		} else if (name_match(p, "Time in Under-Temperature") && statistic.value_int > 0) {
			w = WarningLevel::Notice;
			reason = "The temperature of the drive is or was under the manufacturer-specified minimum. "
					"This may have shortened its lifespan and caused damage. Please operate the drive within manufacturer-specified temperature range.";

		} else if (name_match(p, "Percentage Used Endurance Indicator") && statistic.value_int >= 50) {
			w = WarningLevel::Notice;
			reason = "The drive has less than half of its estimated life left.";

		} else if (name_match(p, "Percentage Used Endurance Indicator") && statistic.value_int >= 100) {
			w = WarningLevel::Warning;
			reason = "The drive is past its estimated lifespan.";
		}
	}

	if (w.has_value()) {
		p.warning_level = w.value();
		p.warning_reason = reason;
	}
}





/// @}
