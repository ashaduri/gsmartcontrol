/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include "storage_detector_win32.h"

#if defined CONFIG_KERNEL_FAMILY_WINDOWS

#include <windows.h>  // CreateFileA(), CloseHandle(), etc...
#include <glibmm.h>

#include "hz/string_sprintf.h"
#include "rconfig/rconfig_mini.h"
#include "app_pcrecpp.h"


/*
3ware Windows (XP so far, maybe the same under the others):
For 3ware 9xxx only.
Call as: smartctl -i sd[a-z],N
	N is port, a-z is logical drive (unit) provided by controller.
	N is limited to [0, 31] in the code.
	The sd[a-z] device actually exists as \\.\PhysicalDrive[0-N].
		No idea how to check if it's 3ware.
Call as: smarctl -i tw_cli/cx/py
	This runs tw_cli tool and parses the output; controller x, port y.
	tw_cli is needed for 64-bit systems, as well as older controllers.
	In tw_cli mode only limited information-gathering is supported.
tw_cli (part of 3DM2) is automatically added to system PATH,
	no need to look for it.
3DM2 install can be detected by checking:
	HKEY_USERS\.DEFAULT\Software\3ware\3DM2, InstallPath
Another option for detection (whether it's 3ware) would be getting
	\\.\PhysicalDrive0 properties, like smartctl does.
Newer (added after 5.39.1) smartctl supports --scan-open, which will give us:
	/dev/sda,0 -d ata (opened)
	/dev/sda,1 -d ata (opened)
-d 3ware is not needed under Windows. We should treat sda as pd0
	and remove pd0 from PhysicalDrive-detected list.
Running smartctl on sda gives almost the same result as on sda,0.


Intel Matrix RAID (since smartmontools SVN version on 2011-02-04):
Call as: "/dev/csmi[0-9],N" where N is the port behind the logical
	scsi controller "\\.\Scsi[0-9]:".
	The prefix "/dev/" is optional.
	This is detected (with /dev/ prefix) by --scan-open.
*/


/**
	smartctl --scan-open output for win32 (3ware):
	/dev/sda,0 -d ata (opened)
	/dev/sda,1 -d ata (opened)

	smartctl --scan-open output for linux:
	/dev/sda -d sat # /dev/sda [SAT], ATA device
	/dev/sdb -d sat # /dev/sdb [SAT], ATA device
	/dev/sdc -d sat # /dev/sdc [SAT], ATA device
*/


namespace {



/// Run "smartctl --scan-open" and pick the devices which have
/// a port parameter. We don't pick the others because the may
/// conflict with pd* devices, and we like pd* better than sd*.
std::string get_scan_open_multiport_devices(std::vector<StorageDeviceRefPtr>& drives,
		ExecutorFactoryRefPtr ex_factory, std::vector<int>& equivalent_pds)
{
	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	std::string smartctl_binary = get_smartctl_binary();

	if (smartctl_binary.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl binary is not set in config.\n");
		return "Smartctl binary is not specified in configuration.";
	}

	std::string smartctl_def_options;
	rconfig::get_data("system/smartctl_options", smartctl_def_options);

	if (!smartctl_def_options.empty())
		smartctl_def_options += " ";

	smartctl_ex->set_command(Glib::shell_quote(smartctl_binary),
			smartctl_def_options + "--scan-open");

	if (!smartctl_ex->execute() || !smartctl_ex->get_error_msg().empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Error while executing smartctl binary.\n");
		return smartctl_ex->get_error_msg();
	}

	// any_to_unix is needed for windows
	std::string output = hz::string_trim_copy(hz::string_any_to_unix_copy(smartctl_ex->get_stdout_str()));
	if (output.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Smartctl returned an empty output.\n");
		return "Smartctl returned an empty output.";
	}

	// if we've reached smartctl port limit (older versions may have smaller limits), abort.
	if (app_pcre_match("/UNRECOGNIZED OPTION/mi", output)) {
		return "Smartctl doesn't support --scan-open switch.";
	}

	std::vector<std::string> lines;
	hz::string_split(output, '\n', lines, true);


// 	/dev/sda,0 -d ata (opened)
// 	/dev/sda,1 -d ata (opened)
// 	/dev/sda -d sat # /dev/sda [SAT], ATA device

	// we only pick the ones with ports
	pcrecpp::RE port_re = app_pcre_re("/^(/dev/[a-z0-9]),([0-9])+[ \\t]+-d[ \\t]+([^\\t\\n]+)/i");
	pcrecpp::RE dev_re = app_pcre_re("/^/dev/sd([a-z])$/");

	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string dev, port_str, type;
		if (port_re.PartialMatch(hz::string_trim_copy(lines.at(i)), &dev, &port_str, &type)) {
			std::string letter;
			if (dev_re.PartialMatch(dev, &letter)) {
				// don't use pd* devices equivalent to these sd* devices.
				equivalent_pds.push_back(letter.at(0) - 'a');
			}

			std::string full_dev = dev + "," + port_str;
			drives.push_back(StorageDeviceRefPtr(new StorageDevice(full_dev, type)));
		}
	}

	return std::string();
}



}




// smartctl accepts various variants, the most straight being pdN,
// (or /dev/pdN, /dev/ being optional) where N comes from
// "\\.\PhysicalDriveN" (winnt only).
// http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx

std::string detect_drives_win32(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	std::vector<int> used_pds;
	std::string error_msg = get_scan_open_multiport_devices(drives, ex_factory, used_pds);

	for (int drive_num = 0; ; ++drive_num) {
		std::string name = hz::string_sprintf("\\\\.\\PhysicalDrive%d", drive_num);

		// If the drive is openable, then it's there.
		// NOTE: Administrative privileges are required to open it.
		// Yes, CreateFile() is open, not create. Yes, it's silly (ah, win32...).
		// We don't use any long/unopenable files here, so use the ANSI version.
		HANDLE h = CreateFileA(name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL);

		// The numbers seem to be consecutive, so break on first invalid.
		if (h == INVALID_HANDLE_VALUE)
			break;

		CloseHandle(h);

		if (std::find(used_pds.begin(), used_pds.end(), drive_num) == used_pds.end()) {
			std::string dev = hz::string_sprintf("pd%d", drive_num);
			drives.push_back(new StorageDevice(dev));
		}
	}

	return std::string();
}




#endif  // CONFIG_KERNEL_FAMILY_WINDOWS
