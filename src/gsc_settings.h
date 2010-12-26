/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef GSC_SETTINGS_H
#define GSC_SETTINGS_H

#include <stdint.h>

#include "rconfig/rconfig_mini.h"



// Initialize ALL default settings here.

// Absolute paths go to root node, relative ones go to /config and /default.

// Note: There must be no degradation if /config is removed entirely
// during runtime. /default must provide every path which /config could
// have held.
// ALL runtime (that is, non-config-file-writable) settings go to /runtime.


inline void init_default_settings()
{
	// Populate /default

	rconfig::set_default_data("system/config_autosave_timeout", uint32_t(3*60));  // 3 minutes. 0 to disable.
	rconfig::set_default_data("system/first_boot", true);  // used to show the first-start warning.

	rconfig::set_default_data("system/smartctl_binary", "smartctl");  // must be in PATH or use absolute path.
	rconfig::set_default_data("system/smartctl_options", "");  // default options on ALL commands
	rconfig::set_default_data("system/smartctl_device_options", "");  // dev1:val1;dev2:val2;... format, each bin2ascii-encoded.

	rconfig::set_default_data("system/linux_udev_byid_path", "/dev/disk/by-id");  // linux hard disk device links here
	rconfig::set_default_data("system/linux_proc_partitions_path", "/proc/partitions");  // file in linux /proc/partitions format
	rconfig::set_default_data("system/solaris_dev_path", "/dev/rdsk");  // path to /dev/rdsk for solaris.
	rconfig::set_default_data("system/unix_sdev_path", "/dev");  // path to /dev. used by other unices
// 	rconfig::set_default_data("system/device_match_patterns", "");  // semicolon-separated PCRE patterns
	rconfig::set_default_data("system/device_blacklist_patterns", "");  // semicolon-separated PCRE patterns

	rconfig::set_default_data("gui/show_smart_capable_only", false);  // show smart-capable drives only
	rconfig::set_default_data("gui/scan_on_startup", true);  // scan drives on startup


	// Populate /runtime too, just in case. The values don't really matter.

	rconfig::set_data("/runtime/gui/hide_tabs_on_smart_disabled", true);
	rconfig::set_data("/runtime/gui/force_no_scan_on_startup", false);
	// rconfig::set_data("/runtime/gui/add_virtuals_on_startup", "");  // vector<string>
	// rconfig::set_data("/runtime/gui/add_devices_on_startup", "");  // vector<string>

}








#endif
