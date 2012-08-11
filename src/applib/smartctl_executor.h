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

#ifndef SMARTCTL_EXECUTOR_H
#define SMARTCTL_EXECUTOR_H

#include "cmdex.h"
#include "cmdex_sync.h"



/// Smartctl executor template.
template<class ExecutorSync>
class SmartctlExecutorGeneric : public ExecutorSync {
	public:

		/// Constructor
		SmartctlExecutorGeneric(const std::string& cmd, const std::string& cmdargs)
			: ExecutorSync(cmd, cmdargs)
		{
			this->construct();
		}


		/// Constructor
		SmartctlExecutorGeneric()
		{
			this->construct();
		}


		/// Virtual destructor
		virtual ~SmartctlExecutorGeneric()
		{ }



	protected:

		/// Called by constructors
		void construct()
		{
			ExecutorSync::get_command_executor().set_exit_status_translator(&SmartctlExecutorGeneric::translate_exit_status, NULL);
			this->set_error_header("An error occurred while executing smartctl:\n\n");
		}


		enum {
			exit_cant_parse = 1 << 0,  ///< Smartctl error code bit
			exit_open_failed = 1 << 1,  ///< Smartctl error code bit
			exit_smart_failed = 1 << 2,  ///< Smartctl error code bit
			exit_disk_failing = 1 << 3,  ///< Smartctl error code bit
			exit_prefail_threshold = 1 << 4,  ///< Smartctl error code bit
			exit_threshold_in_past = 1 << 5,  ///< Smartctl error code bit
			exit_error_log = 1 << 6,  ///< Smartctl error code bit
			exit_self_test_log = 1 << 7  ///< Smartctl error code bit
		};


		/// Translate smartctl error code to a readable message
		static std::string translate_exit_status(int status, void* user_data)
		{
			const char* table[] = {
				"Command line did not parse.",
				"Device open failed, or device did not return an IDENTIFY DEVICE structure.",
				"Some SMART command to the disk failed, or there was a checksum error in a SMART data structure",
				"SMART status check returned \"DISK FAILING\"",
				"SMART status check returned \"DISK OK\" but some prefail Attributes are less than threshold.",
				"SMART status check returned \"DISK OK\" but we found that some (usage or prefail) Attributes have been less than threshold at some time in the past.",
				"The device error log contains records of errors.",
				"The device self-test log contains records of errors."
			};

			std::string str;

			// check every bit
			for (unsigned int i = 0; i <= 7; i++) {
				if (status & (1 << i)) {
					if (!str.empty())
						str += "\n";  // new-line-separate each entry
					str += table[i];
				}
			}

			return str;
		}


		/// Import the last error from command executor and clear all errors there
		virtual void import_error()
		{
			Cmdex& cmdex = this->get_command_executor();
			cmdex.errors_lock();

			Cmdex::error_list_t errors = cmdex.get_errors(false);  // these are not clones

			hz::ErrorBase* e = 0;
			// find the last relevant error.
			// note: const_reverse_iterator doesn't work on gcc 3, so don't do it.
			for (Cmdex::error_list_t::reverse_iterator iter = errors.rbegin(); iter != errors.rend(); ++iter) {
				// ignore iochannel errors, they may mask the real errors
				if ((*iter)->get_type() != "giochannel" && (*iter)->get_type() != "custom") {
					e = (*iter)->clone();
					break;
				}
			}

			cmdex.clear_errors(false);  // and clear them

			cmdex.errors_unlock();

			if (e) {  // if error is present, alert the user
				on_error_warn(e);
			}
		}



		/// This is called when an error occurs in command executor.
		/// Note: The warnings are already printed via debug_* in cmdex.
		virtual void on_error_warn(hz::ErrorBase* e)
		{
			if (!e)
				return;

			// import the error only if it's relevant.
			std::string error_type = e->get_type();
			// accept all errors by default, except:

			// Treat most exit codes as non-errors.
			if (error_type == "exit") {
				int exit_code = 0;
				e->get_code(exit_code);
				// ignore everyone except these
				if ( !((exit_code & exit_cant_parse) || (exit_code & exit_open_failed)) )
					return;

				// ignore giochannel errors - higher level errors will be triggered, and they more user-friendly.
			} else if (error_type == "giochannel" || error_type == "custom") {
				return;
			}

			this->set_error_msg(e->get_message());
		}

};




/// Smartctl executor without GUI support
typedef SmartctlExecutorGeneric<CmdexSync> SmartctlExecutor;

/// A reference-counting pointer to SmartctlExecutor
typedef hz::intrusive_ptr<SmartctlExecutor> SmartctlExecutorRefPtr;



/// Get smartctl binary (from config, etc...). Returns an empty string if not found.
std::string get_smartctl_binary();


/// Execute smartctl on device \c device.
/// \return error message on error, empty string on success.
std::string execute_smartctl(const std::string& device, const std::string& device_opts,
		const std::string& command_options,
		hz::intrusive_ptr<CmdexSync> smartctl_ex, std::string& smartctl_output);





#endif

/// @}
