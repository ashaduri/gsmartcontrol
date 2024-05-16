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

#ifndef GSC_SETTINGS_H
#define GSC_SETTINGS_H

#include "rconfig/rconfig.h"
#include "build_config.h"


/// Initializes ALL default settings.
/// "default" must provide every path which config may hold.
inline void init_default_settings()
{
	// Populate default

	rconfig::set_default_data("system/config_autosave_timeout_sec", 3*60);  // 3 minutes. 0 to disable.
	// rconfig::set_default_data("system/first_boot", true);  // used to show the first-start warning.

	if constexpr(!BuildEnv::is_kernel_family_windows()) {
		rconfig::set_default_data("system/smartctl_binary", "smartctl");  // must be in PATH or use absolute path.
		rconfig::set_default_data("system/tw_cli_binary", "tw_cli");  // must be in PATH or use absolute path.
	} else {
		rconfig::set_default_data("system/smartctl_binary", "smartctl-nc.exe");  // use no-console version by default.
		rconfig::set_default_data("system/tw_cli_binary", "tw_cli.exe");
		rconfig::set_default_data("system/areca_cli_binary", "cli.exe");  // if relative, an installation path is prepended (if found).
	}
	// search for "smartctl-nc.exe" in smartmontools installation first.
	rconfig::set_default_data("system/win32_search_smartctl_in_smartmontools", true);
	rconfig::set_default_data("system/win32_smartmontools_regpath", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\smartmontools");  // in HKLM
	rconfig::set_default_data("system/win32_smartmontools_regpath_wow", "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\smartmontools");  // in HKLM
	rconfig::set_default_data("system/win32_smartmontools_regkey", "InstallLocation");
	rconfig::set_default_data("system/win32_smartmontools_smartctl_binary", "bin\\smartctl-nc.exe");  // relative to smt install path
	rconfig::set_default_data("system/win32_areca_scan_controllers", 2);  // 0 - no, 1 - yes, 2 - auto (if areca tools are found)
	rconfig::set_default_data("system/win32_areca_use_cli", 2);  // 0 - no, 1 - yes, 2 - auto (if it's found)
	rconfig::set_default_data("system/win32_areca_max_controllers", 4);  // Maximum number of areca controllers (a safety measure). CLI supports 4.
	rconfig::set_default_data("system/win32_areca_enc_max_scan_port", 36);  // 1-128 (areca with enclosures). The last RAID port to scan if no other method is available
	rconfig::set_default_data("system/win32_areca_enc_max_enclosure", 3);  // 1-8 (areca with enclosures). The last RAID enclosure to scan if no other method is available
	rconfig::set_default_data("system/win32_areca_neonc_max_scan_port", 24);  // 1-24 (areca without enclosures). The last RAID port to scan if no other method is available

	rconfig::set_default_data("system/smartctl_options", "");  // default options on ALL commands
	rconfig::set_default_data("system/smartctl_device_options", "");  // dev1:val1;dev2:val2;... format, each bin2ascii-encoded.

	rconfig::set_default_data("system/linux_udev_byid_path", "/dev/disk/by-id");  // linux hard disk device links here
	rconfig::set_default_data("system/linux_proc_partitions_path", "/proc/partitions");  // file in linux /proc/partitions format
	rconfig::set_default_data("system/linux_proc_devices_path", "/proc/devices");  // file in linux /proc/devices format
	rconfig::set_default_data("system/linux_proc_scsi_scsi_path", "/proc/scsi/scsi");  // file in linux /proc/scsi/scsi format
	rconfig::set_default_data("system/linux_proc_scsi_sg_devices_path", "/proc/scsi/sg/devices");  // file in linux /proc/scsi/sg/devices format
	rconfig::set_default_data("system/linux_3ware_max_scan_port", 23);  // 0-127 (3ware). The last RAID port to scan if no other method is available
	rconfig::set_default_data("system/linux_areca_enc_max_scan_port", 36);  // 1-128 (areca with enclosures). The last RAID port to scan if no other method is available
	rconfig::set_default_data("system/linux_areca_enc_max_enclosure", 4);  // 1-8 (areca with enclosures). The last RAID enclosure to scan if no other method is available
	rconfig::set_default_data("system/linux_areca_neonc_max_scan_port", 24);  // 1-24 (areca without enclosures). The last RAID port to scan if no other method is available
	rconfig::set_default_data("system/solaris_dev_path", "/dev/rdsk");  // path to /dev/rdsk for solaris.
	rconfig::set_default_data("system/unix_sdev_path", "/dev");  // path to /dev. used by other unices
// 	rconfig::set_default_data("system/device_match_patterns", "");  // semicolon-separated Regex patterns
	rconfig::set_default_data("system/device_blacklist_patterns", "");  // semicolon-separated Regex patterns

	rconfig::set_default_data("gui/drive_data_open_save_dir", "");

	rconfig::set_default_data("gui/show_smart_capable_only", false);  // show smart-capable drives only
	rconfig::set_default_data("gui/scan_on_startup", true);  // scan drives on startup

	rconfig::set_default_data("gui/smartctl_output_filename_format", "{model}_{serial}_{date}.json");  // when suggesting filename

	rconfig::set_default_data("gui/icons_show_device_name", false);  // text under icons
	rconfig::set_default_data("gui/icons_show_serial_number", false);  // text under icons

	rconfig::set_default_data("gui/main_window/default_size_w", 0);
	rconfig::set_default_data("gui/main_window/default_size_h", 0);
	rconfig::set_default_data("gui/main_window/default_pos_x", -1);
	rconfig::set_default_data("gui/main_window/default_pos_y", -1);

	rconfig::set_default_data("gui/info_window/default_size_w", 0);
	rconfig::set_default_data("gui/info_window/default_size_h", 0);
}








#endif

/// @}
