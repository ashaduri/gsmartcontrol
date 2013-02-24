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

#include "hz/hz_config.h"  // CONFIG_*

#if defined CONFIG_KERNEL_FAMILY_WINDOWS


#include <windows.h>  // CreateFileA(), CloseHandle(), etc...
#include <glibmm.h>
#include <set>

#include "hz/win32_tools.h"
#include "hz/string_sprintf.h"
#include "hz/fs_path.h"
#include "rconfig/rconfig_mini.h"
#include "app_pcrecpp.h"
#include "storage_detector_win32.h"
#include "storage_detector_helpers.h"



/**
\file
<pre>
3ware Windows detection:
For 3ware 9xxx only.
Call as: smartctl -i sd[a-z],N
	N is port, a-z is logical drive (unit) provided by controller.
	N is limited to [0, 31] in the code.
	The sd[a-z] device actually exists as \\\\.\\PhysicalDrive[0-N].
		No idea how to check if it's 3ware.
Call as: smarctl -i tw_cli/cx/py
	This runs tw_cli tool and parses the output; controller x, port y.
	tw_cli may be needed for older controllers / drivers.
	In tw_cli mode only limited information-gathering is supported.
tw_cli (part of 3DM2) is automatically added to system PATH,
	no need to look for it.
3DM2 install can be detected by checking:
	HKEY_USERS\\.DEFAULT\\Software\\3ware\\3DM2, InstallPath
Another option for detection (whether it's 3ware) would be getting
	\\\\.\\PhysicalDrive0 properties, like smartctl does.
Newer (added after 5.39.1) smartctl supports --scan-open, which will give us:
	/dev/sda,0 -d ata (opened)
	/dev/sda,1 -d ata (opened)
-d 3ware is not needed under Windows. We should treat sda as pd0
	and remove pd0 from PhysicalDrive-detected list.
Running smartctl on sda gives almost the same result as on sda,0.


Intel Matrix RAID (since smartmontools SVN version on 2011-02-04):
Call as: "/dev/csmi[0-9],N" where N is the port behind the logical
	scsi controller "\\\\.\\Scsi[0-9]:".
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
</pre>
*/


namespace {



/// Run "smartctl --scan-open" and pick the devices which have
/// a port parameter. We don't pick the others because the may
/// conflict with pd* devices, and we like pd* better than sd*.
std::string get_scan_open_multiport_devices(std::vector<StorageDeviceRefPtr>& drives,
		ExecutorFactoryRefPtr ex_factory, std::set<int>& equivalent_pds)
{
	debug_out_info("app", "Getting multi-port devices through smartctl --scan-open...\n");

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
	const pcrecpp::RE port_re = app_pcre_re("/^(/dev/[a-z0-9]+),([0-9]+)[ \\t]+-d[ \\t]+([^ \\t\\n]+)/i");
	const pcrecpp::RE dev_re = app_pcre_re("/^/dev/sd([a-z])$/");

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




/// Find and execute areca cli with specified options, return its output through \c output.
/// \return error message
inline std::string execute_areca_cli(ExecutorFactoryRefPtr ex_factory, const std::string& cli_binary,
		const std::string& command_options, std::string& output)
{
	hz::intrusive_ptr<CmdexSync> executor = ex_factory->create_executor(ExecutorFactory::ExecutorArecaCli);

	executor->set_command(Glib::shell_quote(cli_binary), command_options);

	if (!executor->execute() || !executor->get_error_msg().empty()) {
		debug_out_warn("app", DBG_FUNC_MSG << "Error while executing Areca cli binary.\n");
	}

	// any_to_unix is needed for windows
	output = hz::string_trim_copy(hz::string_any_to_unix_copy(executor->get_stdout_str()));
	if (output.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "Areca cli returned an empty output.\n");
		return "Areca CLI returned an empty output.";
	}

	return std::string();
}



/** <pre>
Get the drives on Areca controller using Areca cli tool.
Note that the drives are inserted in the order they are detected.

There are 3 formats of "disk info" output:

1. No expanders:
------------------------------------------------------------
  # Ch# ModelName                       Capacity  Usage
===============================================================================
  1  1  INTEL SSDSA2M160G2GC             160.0GB  System
  2  2  INTEL SSDSA2M160G2GC             160.0GB  System
  3  3  INTEL SSDSA2M160G2GC             160.0GB  System
  4  4  INTEL SSDSA2M160G2GC             160.0GB  System
  5  5  Hitachi HDS724040ALE640         4000.8GB  Storage
  6  6  Hitachi HDS724040ALE640         4000.8GB  Storage
  7  7  Hitachi HDS724040ALE640         4000.8GB  Storage
  8  8  Hitachi HDS724040ALE640         4000.8GB  Storage
  9  9  Hitachi HDS724040ALE640         4000.8GB  Storage
 10 10  Hitachi HDS724040ALE640         4000.8GB  Storage
 11 11  Hitachi HDS724040ALE640         4000.8GB  Backup
 12 12  Hitachi HDS724040ALE640         4000.8GB  Backup
===============================================================================
GuiErrMsg<0x00>: Success.
------------------------------------------------------------

2. No expanders (this output comes from CLI documentation, possibly an old format):
------------------------------------------------------------
 #   ModelName        Serial#          FirmRev     Capacity  State
===============================================================================
 1   ST3250620NS      5QE1CP8S         3.AEE        250.1GB  RaidSet Member(1)
 2   ST3250620NS      5QE1CP8S         3.AEE        250.1GB  RaidSet Member(1)
....(snip)....
12   ST3250620NS      5QE1CP8S         3.AEE        250.1GB  RaidSet Member(1)
===============================================================================
GuiErrMsg<0x00>: Success.
------------------------------------------------------------

3. Expanders:
------------------------------------------------------------
  # Enc# Slot#   ModelName                        Capacity  Usage
===============================================================================
  1  01  Slot#1  N.A.                                0.0GB  N.A.
  2  01  Slot#2  N.A.                                0.0GB  N.A.
  3  01  Slot#3  N.A.                                0.0GB  N.A.
  4  01  Slot#4  N.A.                                0.0GB  N.A.
  5  01  Slot#5  N.A.                                0.0GB  N.A.
  6  01  Slot#6  N.A.                                0.0GB  N.A.
  7  01  Slot#7  N.A.                                0.0GB  N.A.
  8  01  Slot#8  N.A.                                0.0GB  N.A.
  9  02  SLOT 01 N.A.                                0.0GB  N.A.
 10  02  SLOT 02 N.A.                                0.0GB  N.A.
 11  02  SLOT 03 N.A.                                0.0GB  N.A.
 12  02  SLOT 04 N.A.                                0.0GB  N.A.
 13  02  SLOT 05 N.A.                                0.0GB  N.A.
 14  02  SLOT 06 N.A.                                0.0GB  N.A.
 15  02  SLOT 07 N.A.                                0.0GB  N.A.
 16  02  SLOT 08 N.A.                                0.0GB  N.A.
 17  02  SLOT 09 N.A.                                0.0GB  N.A.
 18  02  SLOT 10 N.A.                                0.0GB  N.A.
 19  02  SLOT 11 N.A.                                0.0GB  N.A.
 20  02  SLOT 12 N.A.                                0.0GB  N.A.
 21  02  SLOT 13 N.A.                                0.0GB  N.A.
 22  02  SLOT 14 ST910021AS                        100.0GB  Free
 23  02  SLOT 15 WDC WD3200BEVT-75A23T0            320.1GB  HotSpare[Global]
 24  02  SLOT 16 N.A.                                0.0GB  N.A.
 25  02  SLOT 17 Hitachi HDS724040ALE640          4000.8GB  Raid Set # 000
 26  02  SLOT 18 ST31500341AS                     1500.3GB  Raid Set # 000
 27  02  SLOT 19 ST3320620AS                       320.1GB  Raid Set # 000
 28  02  SLOT 20 ST31500341AS                     1500.3GB  Raid Set # 000
 29  02  SLOT 21 ST3500320AS                       500.1GB  Raid Set # 000
 30  02  SLOT 22 Hitachi HDS724040ALE640          4000.8GB  Raid Set # 000
 31  02  SLOT 23 Hitachi HDS724040ALE640          4000.8GB  Raid Set # 000
 32  02  SLOT 24 Hitachi HDS724040ALE640          4000.8GB  Raid Set # 000
 33  02  EXTP 01 N.A.                                0.0GB  N.A.
 34  02  EXTP 02 N.A.                                0.0GB  N.A.
 35  02  EXTP 03 N.A.                                0.0GB  N.A.
 36  02  EXTP 04 N.A.                                0.0GB  N.A.
===============================================================================
GuiErrMsg<0x00>: Success.
------------------------------------------------------------
</pre> */
inline std::string areca_cli_get_drives(const std::string& cli_binary, const std::string& dev, int controller,
		std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	debug_out_info("app", "Getting available Areca drives (ports) for controller " << controller << " through Areca CLI...\n");

	// TODO Support controller number.
	// So far it seems the only way to pass the controller number to CLI is to use
	// the interactive mode.

	std::string output;
	std::string error = execute_areca_cli(ex_factory, cli_binary, "disk info", output);
	if (!error.empty()) {
		return error;
	}

	// split to lines
	std::vector<std::string> lines;
	hz::string_split(output, '\n', lines, true);

	enum FormatType {
		FormatUnknown,
		FormatNoEnc1,
		FormatNoEnc2,
		FormatEnc
	};

	pcrecpp::RE noenc1_header_re = app_pcre_re("/^\\s*#\\s+Ch#/mi");
	pcrecpp::RE noenc2_header_re = app_pcre_re("/^\\s*#\\s+ModelName/mi");
	pcrecpp::RE exp_header_re = app_pcre_re("/^\\s*#\\s+Enc#/mi");

	FormatType format_type = FormatUnknown;
	for (std::size_t i = 0; i < lines.size(); ++i) {
		if (noenc1_header_re.PartialMatch(lines.at(i))) {
			format_type = FormatNoEnc1;
			break;
		} else if (noenc2_header_re.PartialMatch(lines.at(i))) {
			format_type = FormatNoEnc2;
			break;
		} else if (exp_header_re.PartialMatch(lines.at(i))) {
			format_type = FormatEnc;
			break;
		}
	}
	if (format_type == FormatUnknown) {
		debug_out_warn("app", "Could not read Areca CLI output: No valid header found.\n");
		return "Could not read Areca CLI output: No valid header found.";
	}

	// Note: These may not match the full model, but just the first part is sufficient for comparison with "N.A.".
	pcrecpp::RE noexp1_port_re = app_pcre_re("/^\\s*[0-9]+\\s+([0-9]+)\\s+([^\\s]+)/mi");  // matches port, model.
	pcrecpp::RE noexp2_port_re = app_pcre_re("/^\\s*([0-9]+)\\s+([^\\s]+)/mi");  // matches port, model.
	pcrecpp::RE exp_port_re = app_pcre_re("/^\\s*[0-9]+\\s+([0-9]+)\\s+(?:Slot#|SLOT\\s+)([0-9]+)\\s+([^\\s]+)/mi");  // matches enclosure, port, model.

	bool has_enclosure = (format_type == FormatEnc);
	if (has_enclosure) {
		debug_out_dump("app", "Areca controller seems to have enclosures.\n");
	} else {
		debug_out_dump("app", "Areca controller doesn't have any enclosures.\n");
	}

	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string port_str, model_str;
		if (has_enclosure) {
			std::string enclosure_str;
			if (exp_port_re.PartialMatch(hz::string_trim_copy(lines.at(i)), &enclosure_str, &port_str, &model_str)) {
				if (model_str != "N.A.") {
					int port = -1, enclosure = -1;
					hz::string_is_numeric(port_str, port);
					hz::string_is_numeric(enclosure_str, enclosure);
					drives.push_back(StorageDeviceRefPtr(new StorageDevice(dev,
							"areca," + hz::number_to_string(port) + "/" + hz::number_to_string(enclosure))));
					debug_out_info("app", "Added Areca drive " << drives.back()->get_device_with_type() << ".\n");
				}
			}
		} else {  // no enclosures
			pcrecpp::RE port_re = (format_type == FormatNoEnc1 ? noexp1_port_re : noexp2_port_re);
			if (port_re.PartialMatch(hz::string_trim_copy(lines.at(i)), &port_str, &model_str)) {
				if (model_str != "N.A.") {
					int port = -1;
					hz::string_is_numeric(port_str, port);
					drives.push_back(StorageDeviceRefPtr(new StorageDevice(dev, "areca," + hz::number_to_string(port))));
					debug_out_info("app", "Added Areca drive " << drives.back()->get_device_with_type() << ".\n");
				}
			}
		}
	}

	return std::string();
}



/** <pre>
Areca Windows:
Call as:
	- smartctl -i -d areca,N /dev/arcmsr0  (N is [1,24]).
	- smartctl -i -d areca,N/E /dev/arcmsr0  (N is [1,128], E is [1,8]).
The models that have "ix" (case-insensitive) in their model names
have the enclosures and require the N,E syntax.
Note: Using N/E syntax with non-enclosure cards seems to work
regardless of the value of E.

Detection:
	Check the registry keys for Areca management software and
	the CLI utility. One of them has to be installed.
Check if CLI is installed. If it is, run it and parse the output. This should
	give us the populated port list (non-enclosure), and the populated port
	and expander list (enclosure models).
	There are two problems with CLI:
	If no controller is present, it crashes.
		We try the following first:
			smartctl -i -d areca,1 /dev/arcmsr0
		If it contains "arcmsr0: No Areca controller found" (Windows), or
			"Smartctl open device: /dev/arcmsr0 [areca_disk#01_enc#01] failed: No such device" (Linux),
			then there's no controller there.
	I don't see how it reports different controllers (there's only one list).
		We ignore this for now.
If CLI is not installed, do the brute-force way:
	For arcmsr0, 1, ..., 8 (until we see "No Areca controller found"):
		Scan -d areca,[1-24] /dev/arcmsrN
		If at least one drive was found, it's a no-enclosure controller.
		If no drives were found, it's probably an enclosure controller, try
		-d areca,[1-128]/[1-8] /dev/arcmsrN
			It's 2-3 drives a second on an empty port, so some limits are set in config.
</pre> */
inline std::string detect_drives_win32_areca(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	debug_out_info("app", DBG_FUNC_MSG << "Detecting drives behind Areca controller(s)...\n");

	int scan_controllers = rconfig::get_data<int32_t>("system/win32_areca_scan_controllers");
	if (scan_controllers == 0) {  // disabled
		debug_out_info("app", "Areca controller scanning is disabled through config.\n");
		return std::string();
	}

	// Check if Areca tools are installed

	std::string cli_inst_path;
	hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CLI", "InstallPath", cli_inst_path);
	if (cli_inst_path.empty()) {
		hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE,
				"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CLI", "InstallPath", cli_inst_path);
	}

	if (scan_controllers == 2) {  // auto
		std::string http_inst_path;
		hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE,
				"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\archttp", "InstallPath", http_inst_path);
		if (http_inst_path.empty()) {
			hz::win32_get_registry_value_string(HKEY_LOCAL_MACHINE,
					"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\archttp", "InstallPath", http_inst_path);
		}
		if (cli_inst_path.empty() && http_inst_path.empty()) {
			debug_out_info("app", "No Areca software found. Install Areca CLI or set \"system/win32_areca_scan_controllers\" config key to 1 to force scanning.\n");
			return std::string();
		}
		if (http_inst_path.empty()) {
			debug_out_dump("app", "Areca HTTP installation not found.\n");
		} else {
			debug_out_dump("app", "Areca HTTP installation found at: \"" << http_inst_path << "\".\n");
		}
	}
	if (cli_inst_path.empty()) {
		debug_out_dump("app", "Areca CLI installation not found.\n");
	} else {
		debug_out_dump("app", "Areca CLI installation found at: \"" << cli_inst_path << "\".\n");
	}

	bool cli_found = !cli_inst_path.empty();

	int use_cli = rconfig::get_data<int32_t>("system/win32_areca_use_cli");
	bool scan_detect = (use_cli != 1);  // Whether to detect using sequential port scanning. Only do that if CLI is not forced.
	if (use_cli == 2) {  // auto
		use_cli = cli_found;
	}

	std::string cli_binary;
	if (use_cli == 1) {
		cli_binary = rconfig::get_data<std::string>("system/areca_cli_binary");
		if (!hz::FsPath(cli_binary).is_absolute() && !cli_inst_path.empty()) {
			cli_binary = hz::FsPath(cli_inst_path).append(cli_binary).str();
		}
		if (hz::FsPath(cli_binary).is_absolute() && !hz::FsPath(cli_binary).exists()) {
			use_cli = 0;
			debug_out_dump("app", "Areca CLI binary \"" << cli_binary << "\" not found.\n");
		}
	}

	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	// --- CLI mode

	// Since CLI may segfault if there are no drives, test the controller presence first.
	// It doesn't matter if we use areca,N or areca,N/E - we will still get a different
	// error if there's no controller.
	if (use_cli) {
		debug_out_dump("app", "Testing Areca controller presence using smartctl...\n");

		StorageDeviceRefPtr drive(new StorageDevice("/dev/arcmsr0", "areca,1"));
		std::string error_msg = drive->fetch_basic_data_and_parse(smartctl_ex);
		std::string output = drive->get_info_output();
		if (app_pcre_match("/No Areca controller found/mi", output)
				|| app_pcre_match("/Smartctl open device: .* failed: No such device/mi", output) ) {
			use_cli = 0;
			scan_detect = false;
			debug_out_dump("app", "Areca controller not found.\n");
		}
	}

	if (use_cli) {
		debug_out_info("app", "Scanning Areca drives using CLI...\n");
		int cli_max_controllers = 1;  // TODO controller # with CLI.
		for (int controller_no = 0; controller_no < cli_max_controllers; ++controller_no) {
			std::string error_msg = areca_cli_get_drives(cli_binary, "/dev/arcmsr" + hz::number_to_string(controller_no), controller_no, drives, ex_factory);
			// If we get an error on controller 0, fall back to no-cli detection.
			if (!error_msg.empty() && controller_no == 0) {
				use_cli = 0;
				debug_out_warn("app", "Areca scan using CLI failed.\n");
				if (scan_detect) {
					debug_out_dump("app", "Trying manual Areca scan because of a CLI error.\n");
				}
			}
		}
	}

	// If CLI scanning failed (and it was not forced), or if there was no CLI, scan manually.
	if (use_cli == 0 && scan_detect) {
		debug_out_info("app", "Manually scanning Areca controllers and ports...\n");

		int max_controllers = rconfig::get_data<int32_t>("system/win32_areca_max_controllers");
		int max_noenc_ports = rconfig::get_data<int32_t>("system/win32_areca_neonc_max_scan_port");
		max_noenc_ports = std::max(1, std::min(24, max_noenc_ports));  // 1-24 sanity check
		int max_enc_ports = rconfig::get_data<int32_t>("system/win32_areca_enc_max_scan_port");
		max_enc_ports = std::max(1, std::min(128, max_enc_ports));  // 1-128 sanity check
		int max_enclosures = rconfig::get_data<int32_t>("system/win32_areca_enc_max_enclosure");
		max_enclosures = std::max(1, std::min(8, max_enclosures));  // 1-8 sanity check

		for (int controller_no = 0; controller_no < max_controllers; ++controller_no) {
			std::string dev = std::string("/dev/arcmsr") + hz::number_to_string(controller_no);

			// First, scan using the areca,N format.
			debug_out_dump("app", "Starting brute-force port scan (no-enclosure) on 1-" << max_noenc_ports << " ports, device \"" << dev
					<< "\". Change the maximum by setting \"system/win32_areca_neonc_max_scan_port\" config key.\n");

			std::size_t old_drive_count = drives.size();
			std::string last_output;
			std::string error_msg = smartctl_scan_drives_sequentially(dev, "areca,%d", 1, max_noenc_ports, drives, ex_factory, last_output);
			// If the scan stopped because of no controller, stop it all.
			if (!error_msg.empty() && (app_pcre_match("/No Areca controller found/mi", last_output)
					|| app_pcre_match("/Smartctl open device: .* failed: No such device/mi", last_output)) ) {
				debug_out_dump("app", "Areca controller " << controller_no << " not present, stopping sequential scan.\n");
				break;
			}

			// If no drives were added (but the controller is present), it may be due to
			// expander syntax requirement. Try that.
			if (drives.size() == old_drive_count) {
				for (int enclosure_no = 1; enclosure_no < max_enclosures; ++enclosure_no) {
					debug_out_dump("app", "Starting brute-force port scan (enclosure #" << enclosure_no << ") on 1-" << max_enc_ports << " ports, device \"" << dev
							<< "\". Change the maximums by setting \"system/win32_areca_onc_max_scan_port\" and \"system/win32_areca_enc_max_enclosure\" config keys.\n");
					error_msg = smartctl_scan_drives_sequentially(dev, "areca,%d/" + hz::number_to_string(enclosure_no), 1, max_enc_ports, drives, ex_factory, last_output);
				}
			}

			debug_out_dump("app", "Brute-force port scan finished.\n");
		}
	}

	debug_out_info("app", DBG_FUNC_MSG << "Finished scanning Areca controllers.\n");

	return std::string();
}



}




// smartctl accepts various variants, the most straight being pdN,
// (or /dev/pdN, /dev/ being optional) where N comes from
// "\\.\PhysicalDriveN" (winnt only).
// http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx
std::string detect_drives_win32(std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory)
{
	std::vector<std::string> error_msgs;
	std::string error_msg;

	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	// Fetch multiport devices using --scan-open.

	std::set<int> used_pds;
	error_msg = get_scan_open_multiport_devices(drives, ex_factory, used_pds);
	if (!error_msg.empty()) {
		error_msgs.push_back(error_msg);
	}

	bool multiport_found = !drives.empty();
	bool areca_open_found = false;  // whether areca devices were found at --scan-open time.

	// Find out their serial numbers and whether there are Arecas there.
	std::set<std::string> serials;
	for (std::size_t i = 0; i < drives.size(); ++i) {
		std::string local_error = drives.at(i)->fetch_basic_data_and_parse(smartctl_ex);
		if (!local_error.empty()) {
			debug_out_info("app", "Smartctl returned with an error: " << local_error << "\n");
			// Don't exit, just report it.
		}
		if (!drives.at(i)->get_serial_number().empty()) {
			// add model as well, who knows, there may be duplicates across vendors
			serials.insert(drives.at(i)->get_model_name() + "_" + drives.at(i)->get_serial_number());
		}

		// See if there are any areca devices. This is not implemented yet by smartctl (as of 6.0),
		// but if it ever is, we can skip our own detection below.
		std::string type_arg = drives.at(i)->get_type_argument();
		if (type_arg.find("areca") != std::string::npos) {
			areca_open_found = true;
		}
	}


	// Scan PhysicalDrive entries

	debug_out_info("app", "Starting sequential scan of \\\\.\\PhysicalDriveN devices...\n");

	for (int drive_num = 0; ; ++drive_num) {

		// If the drive was already encountered in --scan-open (with a port number), skip it.
		if (used_pds.count(drive_num) > 0) {
			debug_out_dump("app", "pd" << drive_num << " already encountered in --scan-open output (as sd*), skipping.\n");
			continue;
		}

		std::string phys_name = hz::string_sprintf("\\\\.\\PhysicalDrive%d", drive_num);

		// If the drive is openable, then it's there. Yes, CreateFile() is open, not create.
		// NOTE: Administrative privileges are required to open it.
		// We don't use any long/unopenable files here, so use the ANSI version.
		HANDLE h = CreateFileA(phys_name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL);

		// The numbers seem to be consecutive, so break on first invalid.
		if (h == INVALID_HANDLE_VALUE) {
			debug_out_dump("app", "Could not open \"" << phys_name << "\", stopping sequential scan.\n");
			break;
		}
		CloseHandle(h);

		debug_out_dump("app", "Successfully opened \"" << phys_name << "\".\n");

		StorageDeviceRefPtr drive(new StorageDevice(hz::string_sprintf("pd%d", drive_num)));

		// Sometimes, a single physical drive may be accessible from both "/.//PhysicalDriveN"
		// and "/.//Scsi2" (e.g. pd0 and csmi2,1). Prefer the port-having ones (which is from --scan-open),
		// they contain more information.
		// The only way to detect these duplicates is to compare them using serial numbers.
		if (!serials.empty()) {
			std::string local_error = drive->fetch_basic_data_and_parse(smartctl_ex);
			if (!local_error.empty()) {
				debug_out_info("app", "Smartctl returned with an error: " << local_error << "\n");
				// Don't exit, just report it.
			}
			// A serial may be empty if "-q noserial" was given to smartctl.
			if (!drive->get_serial_number().empty()
						&& serials.count(drive->get_model_name() + "_" + drive->get_serial_number()) > 0) {
				debug_out_info("app", "Skipping drive due to duplicate S/N: model: \"" << drive->get_model_name()
						<< "\", S/N: \"" << drive->get_serial_number() << "\".\n");
				continue;
			}
		}

		debug_out_info("app", "Added drive " << drive->get_device_with_type() << ".\n");
		drives.push_back(drive);
	}


	// If smartctl --scan-open returns no "sd*,port"-style devices,
	// check if 3dm2 is installed and execute "tw_cli show" to get
	// the controllers, then use the "tw_cli/cN/pN"-style drives with smartctl. This may
	// happen with older smartctl which doesn't support --scan-open, or with
	// drivers that don't allow proper SMART commands.
	if (!multiport_found) {
		debug_out_info("app", "Checking for additional 3ware devices...\n");
		std::string inst_path;
		hz::win32_get_registry_value_string(HKEY_USERS, ".DEFAULT\\Software\\3ware\\3DM2", "InstallPath", inst_path);

		if (!inst_path.empty()) {
			debug_out_dump("app", "3ware 3DM2 found at\"" << inst_path << "\".\n");
			std::vector<int> controllers;
			error_msg = tw_cli_get_controllers(ex_factory, controllers);
			// ignore the error message above, it's of no use.
			for (std::size_t i = 0; i < controllers.size(); ++i) {
				// don't specify device, it's ignored in tw_cli mode
				tw_cli_get_drives("", controllers.at(i), drives, ex_factory, true);
			}
		} else {
			debug_out_info("app", "3ware 3DM2 not installed.\n");
		}
	}


	if (!areca_open_found) {
		detect_drives_win32_areca(drives, ex_factory);
		if (!error_msg.empty()) {
			error_msgs.push_back(error_msg);
		}
	}

	return hz::string_join(error_msgs, "\n");
}




#endif  // CONFIG_KERNEL_FAMILY_WINDOWS

/// @}
