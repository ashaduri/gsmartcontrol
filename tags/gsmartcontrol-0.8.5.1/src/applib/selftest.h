/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef SELFTEST_H
#define SELFTEST_H

#include <string>
#include <glibmm/timer.h>

#include "hz/cstdint.h"
#include "hz/intrusive_ptr.h"

#include "storage_device.h"
#include "cmdex_sync.h"



class SelfTest : public hz::intrusive_ptr_referenced {
	public:

		enum test_t {
			type_ioffline,  // immediate offline, not supported yet
			type_short,
			type_long,
			type_conveyance
		};

		static std::string get_test_name(test_t t)
		{
			switch (t) {
				case type_ioffline: return "Immediate Offline Test";
				case type_short: return "Short Self-test";
				case type_long: return "Extended Self-test";
				case type_conveyance: return "Conveyance Self-test";
			};
			return "[error]";
		}


		// The drive must have the capabilities present in properties.
		SelfTest(StorageDeviceRefPtr drive, test_t type)
			: drive_(drive), type_(type)
		{
			clear();
		}


		void clear()  // clear results of previous test
		{
			status_ = StorageSelftestEntry::status_unknown;
			remaining_percent_ = -1;
			last_seen_percent_ = -1;
			total_duration_ = -1;
			poll_in_seconds_ = -1;
		}


		bool is_active() const
		{
			return (status_ == StorageSelftestEntry::status_in_progress);
		}


		// returns -1 if n/a or unknown.
		int8_t get_remaining_percent() const
		{
			return remaining_percent_;
		}


		// Returns estimated time of completion for the test. returns -1 if n/a or unknown. 0 is a valid value.
		int64_t get_remaining_seconds() const;


		test_t get_test_type() const
		{
			return type_;
		}


		StorageSelftestEntry::status_t get_status() const
		{
			return status_;
		}


		// get the number of seconds after which the caller should call update().
		int64_t get_poll_in_seconds() const
		{
			return poll_in_seconds_;
		}


		// gets a constant "test duration during idle" capability drive's stored capabilities. -1 if N/A.
		int64_t get_min_duration_seconds() const;


		// gets the current test type support status from drive's stored capabilities.
		bool is_supported() const;


		// These return an error message, or empty string if no error.

		// start the test
		std::string start(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


		// abort the running test
		std::string force_stop(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


		// update status variables
		std::string update(hz::intrusive_ptr<CmdexSync> smartctl_ex = 0);


	private:
		StorageDeviceRefPtr drive_;

		test_t type_;

		// status variables:
		StorageSelftestEntry::status_t status_;
		int8_t remaining_percent_;  // 0 means unknown, -1 means n/a. Set to 100 on start.
		int8_t last_seen_percent_;  // to detect changes in percentage (needed for timer update)
		mutable int64_t total_duration_;  // total duration needed for the test, as reported by drive. constant. this is a cache.
		int64_t poll_in_seconds_;  // the user is asked to poll after this much seconds have passed.

		Glib::Timer timer_;  // counts time since the last percent change

};




typedef hz::intrusive_ptr<SelfTest> SelfTestPtr;




#endif
