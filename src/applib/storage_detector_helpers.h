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

#ifndef STORAGE_DETECTOR_HELPERS_H
#define STORAGE_DETECTOR_HELPERS_H

#include <string>
#include <vector>

#include "local_glibmm.h"  // Glib::shell_quote(), compose

#include "build_config.h"
#include "command_executor_factory.h"
#include "storage_device.h"
#include "rconfig/rconfig.h"
#include "app_pcrecpp.h"
#include "hz/string_num.h"



/// Find and execute tw_cli with specified options, return its output through \c output.
/// \return error message
inline std::string execute_tw_cli(const ExecutorFactoryPtr& ex_factory, const std::string& command_options, std::string& output)
{
	std::shared_ptr<CommandExecutor> executor = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::TwCli);

	auto binary = rconfig::get_data<std::string>("system/tw_cli_binary");

	if (binary.empty()) {
		debug_out_error("app", DBG_FUNC_MSG << "tw_cli binary is not set in config.\n");
		return Glib::ustring::compose(_("%1 binary is not specified in configuration."), "tw_cli");
	}

	std::vector<std::string> binaries;  // binaries to try
	// Note: tw_cli is automatically added to PATH in windows, no need to look for it.
	binaries.push_back(binary);
#ifdef CONFIG_KERNEL_LINUX
	// tw_cli may be named tw_cli.x86 or tw_cli.x86_64 in linux
	binaries.push_back(binary + ".x86_64");  // try this first
	binaries.push_back(binary + ".x86");
#endif

	for (const auto& bin : binaries) {
		executor->set_command(Glib::shell_quote(bin), command_options);

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
		return _("tw_cli returned an empty output.");
	}

	return std::string();
}



/// Get the drives on a 3ware controller using tw_cli.
/// Note that the drives are inserted in the order they are detected.
inline std::string tw_cli_get_drives(const std::string& dev, int controller,
		std::vector<StorageDevicePtr>& drives, const ExecutorFactoryPtr& ex_factory, bool use_tw_cli_dev)
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
	pcrecpp::RE port_re = app_pcre_re(R"(/^p([0-9]+)[ \t]+([^\t\n]+)/mi)");
	for (const auto& line : lines) {
		std::string port_str, status;
		if (port_re.PartialMatch(hz::string_trim_copy(line), &port_str, &status)) {
			if (status != "NOT-PRESENT") {
				int port = -1;
				if (hz::string_is_numeric_nolocale(port_str, port)) {
					if (use_tw_cli_dev) {  // use "tw_cli/cx/py" device
						drives.emplace_back(std::make_shared<StorageDevice>("tw_cli/c"
								+ hz::number_to_string_nolocale(controller) + "/p" + hz::number_to_string_nolocale(port)));
					} else {
						drives.emplace_back(std::make_shared<StorageDevice>(dev, "3ware," + hz::number_to_string_nolocale(port)));
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
inline std::string tw_cli_get_controllers(const ExecutorFactoryPtr& ex_factory, std::vector<int>& controllers)
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
	for (const auto& line : lines) {
		std::string controller_str;
		if (controller_re.PartialMatch(hz::string_trim_copy(line), &controller_str)) {
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
	  int from, int to, std::vector<StorageDevicePtr>& drives, const ExecutorFactoryPtr& ex_factory, std::string& last_output)
{
	std::shared_ptr<CommandExecutor> smartctl_ex = ex_factory->create_executor(CommandExecutorFactory::ExecutorType::Smartctl);

	for (int i = from; i <= to; ++i) {
		std::string type_arg = hz::string_sprintf(type.c_str(), i);
		auto drive = std::make_shared<StorageDevice>(dev, type_arg);

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
