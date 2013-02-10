/**************************************************************************
 Copyright:
      (C) 2011 - 2012  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include <glibmm.h>

#include "executor_factory.h"
#include "storage_device.h"
#include "rconfig/rconfig_mini.h"
#include "app_pcrecpp.h"



/// Find and execute tw_cli with specified options, return its output through \c output.
/// \return error message
inline std::string execute_tw_cli(ExecutorFactoryRefPtr ex_factory, const std::string& command_options, std::string& output)
{
	hz::intrusive_ptr<CmdexSync> executor = ex_factory->create_executor(ExecutorFactory::ExecutorTwCli);

	std::string binary;
	rconfig::get_data("system/tw_cli_binary", binary);

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



/// Get number of ports using tw_cli. Return -1 on error.
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
				hz::string_is_numeric(port_str, port);
				if (port != -1) {
					if (use_tw_cli_dev) {  // use "tw_cli/cx/py" device
						drives.push_back(StorageDeviceRefPtr(new StorageDevice("tw_cli/c"
								+ hz::number_to_string(controller) + "/p" + hz::number_to_string(port))));
					} else {
						drives.push_back(StorageDeviceRefPtr(new StorageDevice(dev, "3ware," + hz::number_to_string(port))));
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
			hz::string_is_numeric(controller_str, controller);
			if (controller != -1) {
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







#endif

/// @}
