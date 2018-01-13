/**************************************************************************
 Copyright:
      (C) 2011 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef STORAGE_DETECTOR_HELPERS_H
#define STORAGE_DETECTOR_HELPERS_H

#include <string>
#include <vector>

// TODO Remove this in gtkmm4.
#include <bits/stdc++.h>  // to avoid throw() macro errors.
#define throw(a)  // glibmm uses dynamic exception specifications, remove them.
#include <glibmm.h>  // Glib::shell_quote()
#undef throw

#include "executor_factory.h"
#include "storage_device.h"
#include "rconfig/config.h"
#include "app_pcrecpp.h"
#include "hz/string_num.h"



/// Find and execute tw_cli with specified options, return its output through \c output.
/// \return error message
inline std::string execute_tw_cli(ExecutorFactoryRefPtr ex_factory, const std::string& command_options, std::string& output)
{
	hz::intrusive_ptr<CmdexSync> executor = ex_factory->create_executor(ExecutorFactory::ExecutorTwCli);

	std::string binary = rconfig::get_data<std::string>("system/tw_cli_binary");

	if (binary.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "tw_cli binary is not set in config.\n");
		return "tw_cli binary is not specified in configuration.";
	}

	std::vector<std::string> binaries;  // binaries to try
	// Note: tw_cli is automatically added to PATH in windows, no need to look for it.
	binaries.push_back(binary);
#ifdef CONFIG_KERNEL_LINUX
	// tw_cli may be named tw_cli.x86 or tw_cli.x86_64 in linux
	binaries.push_back(binary + ".x86_64");  // try this first
	binaries.push_back(binary + ".x86");
#endif

	for (std::size_t i = 0; i < binaries.size(); ++i) {
		executor->set_command(Glib::shell_quote(binaries.at(i)), command_options);

		if (!executor->execute() || !executor->get_error_msg().empty()) {
			debug_out_warn("app", DBG_FUNC_MSG << "Error while executing tw_cli binary.\n");
		} else {
			break;  // found it
		}
	}

	// any_to_unix is needed for windows
	output = hz::string_trim_copy(hz::string_any_to_unix_copy(executor->get_stdout_str()));
	if (output.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "tw_cli returned an empty output.\n");
		return "tw_cli returned an empty output.";
	}

	return std::string();
}



/// Get the drives on a 3ware controller using tw_cli.
/// Note that the drives are inserted in the order they are detected.
inline std::string tw_cli_get_drives(const std::string& dev, int controller,
		std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory, bool use_tw_cli_dev)
{
	debug_out_info("app", "Getting available 3ware drives (ports) for controller " << controller << " through tw_cli...\n");

	std::string output;
	std::string error = execute_tw_cli(ex_factory, hz::string_sprintf("/c%d show all", controller), output);
	if (!error.empty()) {
		return error;
	}

	// split to lines
	std::vector<std::string> lines;
	hz::string_split(output, '\n', lines, true);

	// Note that the ports may be printed in any order. We sort the drives themselves in the end.
	pcrecpp::RE port_re = app_pcre_re("/^p([0-9]+)[ \\t]+([^\\t\\n]+)/mi");
	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string port_str, status;
		if (port_re.PartialMatch(hz::string_trim_copy(lines.at(i)), &port_str, &status)) {
			if (status != "NOT-PRESENT") {
				int port = -1;
				if (hz::string_is_numeric_nolocale(port_str, port)) {
					if (use_tw_cli_dev) {  // use "tw_cli/cx/py" device
						drives.push_back(StorageDeviceRefPtr(new StorageDevice("tw_cli/c"
								+ hz::number_to_string_nolocale(controller) + "/p" + hz::number_to_string_nolocale(port))));
					} else {
						drives.push_back(StorageDeviceRefPtr(new StorageDevice(dev, "3ware," + hz::number_to_string_nolocale(port))));
					}
					debug_out_info("app", "Added 3ware drive " << drives.back()->get_device_with_type() << ".\n");
				}
			}
		}
	}

	return std::string();
}



/// Return 3ware SCSI host numbers (same as /c switch to tw_cli).
/// \return error string on error
inline std::string tw_cli_get_controllers(ExecutorFactoryRefPtr ex_factory, std::vector<int>& controllers)
{
	debug_out_info("app", "Getting available 3ware controllers through tw_cli...\n");

	std::string output;
	std::string error = execute_tw_cli(ex_factory, "show", output);
	if (!error.empty()) {
		return error;
	}

	// split to lines
	std::vector<std::string> lines;
	hz::string_split(output, '\n', lines, true);

	pcrecpp::RE controller_re = app_pcre_re("/^c([0-9]+)[ \\t]+/mi");
	for (std::size_t i = 0; i < lines.size(); ++i) {
		std::string controller_str;
		if (controller_re.PartialMatch(hz::string_trim_copy(lines.at(i)), &controller_str)) {
			int controller = -1;
			if (hz::string_is_numeric_nolocale(controller_str, controller)) {
				debug_out_info("app", "Found 3ware controller " << controller << ".\n");
				controllers.push_back(controller);
			}
		}
	}

	// Sort them. This affects only the further detection order, since the drives
	// are sorted in the end anyway.
	std::sort(controllers.begin(), controllers.end());

	return std::string();
}



/// Get number of ports by sequentially running smartctl on each port, until
/// one of the gives an error. \c type contains a printf-formatted string with %d.
/// \return an error message on error.
inline std::string smartctl_scan_drives_sequentially(const std::string& dev, const std::string& type,
	  int from, int to, std::vector<StorageDeviceRefPtr>& drives, ExecutorFactoryRefPtr ex_factory, std::string& last_output)
{
	hz::intrusive_ptr<CmdexSync> smartctl_ex = ex_factory->create_executor(ExecutorFactory::ExecutorSmartctl);

	for (int i = from; i <= to; ++i) {
		std::string type_arg = hz::string_sprintf(type.c_str(), i);
		StorageDeviceRefPtr drive(new StorageDevice(dev, type_arg));

		// This will generate an error if smartctl doesn't return 0, which is what happens
		// with non-populated ports.
		// Sometimes the output contains:
		// "Read Device Identity failed: Input/output error"
		// or
		// "Read Device Identity failed: empty IDENTIFY data"
		std::string error_msg = drive->fetch_basic_data_and_parse(smartctl_ex);
		last_output = drive->get_info_output();

		// If we've reached smartctl port limit (older versions may have smaller limits), abort.
		if (app_pcre_match("/VALID ARGUMENTS ARE/mi", last_output)) {
			break;
		}

		// If we couldn't open the device, it means there is no such controller at specified device
		// and scanning the ports is useless.
		if (app_pcre_match("/No .* controller found/mi", last_output)
				|| app_pcre_match("/Smartctl open device: .* failed: No such device/mi", last_output) ) {
			break;
		}

		if (!error_msg.empty()) {
			debug_out_info("app", "Smartctl returned with an error: " << error_msg << "\n");
			debug_out_dump("app", "Skipping drive " << drive->get_device_with_type() << " due to smartctl error.\n");
		} else {
			drives.push_back(drive);
			debug_out_info("app", "Added drive " << drive->get_device_with_type() << ".\n");
		}
	}

	return std::string();
}







#endif

/// @}
