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

#include "storage_detector_win32.h"

#if defined CONFIG_KERNEL_FAMILY_WINDOWS

#include <windows.h>  // CreateFileA(), CloseHandle(), etc...
#include <glibmm.h>
#include <set>

#include "hz/win32_tools.h"
#include "hz/string_sprintf.h"
#include "rconfig/rconfig_mini.h"
#include "app_pcrecpp.h"
#include "storage_detector_helpers.h"


/**
\file
3ware Windows detection:
For 3ware 9xxx only.
Call as: smartctl -i sd[a-z],N
	N is port, a-z is logical drive (unit) provided by controller.
	N is limited to [0, 31] in the code.
	The sd[a-z] device actually exists as \\.\PhysicalDrive[0-N].
		No idea how to check if it's 3ware.
Call as: smarctl -i tw_cli/cx/py
	This runs tw_cli tool and parses the output; controller x, port y.
	tw_cli may be needed for older controllers / drivers.
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
The drives may be duplicated as pdX (with X and N being unrelated).
	This usually happens when Intel RAID drivers are installed, even
	if no RAID configuration is present. For example, on a laptop with
	Intel chipset and Intel drivers installed, we have /dev/csmi0,0 and
	pd0 for the first HDD, and /dev/csmi0,1 for DVD (no pdX entry there).
	This is what --scan-open looks like:
------------------------------------------------------------------
/dev/sda -d ata # /dev/sda, ATA device
/dev/csmi0,0 -d ata # /dev/csmi0,0, ATA device
/dev/csmi0,1 -d ata # /dev/csmi0,1, ATA device
------------------------------------------------------------------
	We filter out pdX devices using serial numbers (unless "-q noserial"
	is given to smartctl), and prefer csmi to pd (since csmi provides more features).


smartctl --scan-open output for win32 with 3ware RAID:
------------------------------------------------------------------
/dev/sda,0 -d ata (opened)
/dev/sda,1 -d ata (opened)
------------------------------------------------------------------
*/


namespace {



/// Run "smartctl --scan-open" and pick the devices which have
/// a port parameter. We don't pick the others because the may
/// conflict with pd* devices, and we like pd* better than sd*.
std::string get_scan_open_multiport_devices(std::vector<StorageDeviceRefPtr>& drives,
		ExecutorFactoryRefPtr ex_factory, std::set<int>& equivalent_pds)
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
// /dev/sde,2 -d ata [ATA] (opened)

	// we only pick the ones with ports
	pcrecpp::RE port_re = app_pcre_re("/^(/dev/[a-z0-9]+),([0-9]+)[ \\t]+-d[ \\t]+([^ \\t\\n]+)/i");
	pcrecpp::RE dev_re = app_pcre_re("/^/dev/sd([a-z])$/");

	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string dev, port_str, type;
		if (port_re.PartialMatch(hz::string_trim_copy(lines.at(i)), &dev, &port_str, &type)) {
			std::string letter;
			if (dev_re.PartialMatch(dev, &letter)) {
				// don't use pd* devices equivalent to these sd* devices.
				equivalent_pds.insert(letter.at(0) - 'a');
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
	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	// Fetch multiport devices using --scan-open.
	std::set<int> used_pds;
	std::string error_msg = get_scan_open_multiport_devices(drives, ex_factory, used_pds);
	bool multiport_found = !drives.empty();

	// Find out their serial numbers
	std::set<std::string> serials;
	for (std::size_t i = 0; i < drives.size(); ++i) {
		std::string local_error = drives.at(i)->fetch_basic_data_and_parse(smartctl_ex);
		if (!local_error.empty()) {
			debug_out_info("app", "Smartctl returned with an error: " << error_msg << "\n");
			// Don't exit, just report it.
		}
		if (!drives.at(i)->get_serial_number().empty()) {
			// add model as well, who knows, there may be duplicates across vendors
			serials.insert(drives.at(i)->get_model_name() + "_" + drives.at(i)->get_serial_number());
		}
	}

	// Scan PhysicalDrive entries
	for (int drive_num = 0; ; ++drive_num) {

		// If the drive was already encountered in --scan-open (with a port number), skip it.
		if (used_pds.count(drive_num) > 0) {
			continue;
		}

		std::string name = hz::string_sprintf("\\\\.\\PhysicalDrive%d", drive_num);

		// If the drive is openable, then it's there. Yes, CreateFile() is open, not create.
		// NOTE: Administrative privileges are required to open it.
		// We don't use any long/unopenable files here, so use the ANSI version.
		HANDLE h = CreateFileA(name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL);

		// The numbers seem to be consecutive, so break on first invalid.
		if (h == INVALID_HANDLE_VALUE)
			break;

		CloseHandle(h);

		StorageDeviceRefPtr drive(new StorageDevice(hz::string_sprintf("pd%d", drive_num)));

		// Sometimes, a single physical drive may be accessible from both "/.//PhysicalDriveN"
		// and "/.//Scsi2" (e.g. pd0 and csmi2,1). Prefer the port-having ones (which is from --scan-open),
		// they contain more information.
		// The only way to detect these duplicates is to compare them using serial numbers.
		if (!serials.empty()) {
			std::string local_error = drive->fetch_basic_data_and_parse(smartctl_ex);
			if (!local_error.empty()) {
				debug_out_info("app", "Smartctl returned with an error: " << error_msg << "\n");
				// Don't exit, just report it.
			}
			// A serial may be empty if "-q noserial" was given to smartctl.
			if (!drive->get_serial_number().empty()
						&& serials.count(drive->get_model_name() + "_" + drive->get_serial_number()) > 0) {
				debug_out_info("app", "Skipping duplicate drive: model: \"" << drive->get_model_name()
						<< "\", S/N: \"" << drive->get_serial_number() << "\".\n");
				continue;
			}
		}

		drives.push_back(drive);
	}


	// If smartctl --scan-open returns no "sd*,port"-style devices,
	// check if 3dm2 is installed and execute "tw_cli show" to get
	// the controllers, then use the tw_cli variant of smartctl.
	if (!multiport_found) {
		std::string inst_path;
		hz::win32_get_registry_value_string(HKEY_USERS, ".DEFAULT\\Software\\3ware\\3DM2", "InstallPath", inst_path);

		if (!inst_path.empty()) {
			std::vector<int> controllers;
			error_msg = tw_cli_get_controllers(ex_factory, controllers);
			// ignore the error message above, it's of no use.
			for (std::size_t i = 0; i < controllers.size(); ++i) {
				// don't specify device, it's ignored in tw_cli mode
				tw_cli_get_drives("", controllers.at(i), drives, ex_factory, true);
			}
		}
	}


	return std::string();
}




#endif  // CONFIG_KERNEL_FAMILY_WINDOWS

/// @}
