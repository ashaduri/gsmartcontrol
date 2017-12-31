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

#ifndef CLI_EXECUTORS_H
#define CLI_EXECUTORS_H

#include "cmdex.h"
#include "cmdex_sync.h"



/// Executor for tw_cli (3ware utility)
template<class ExecutorSync>
class TwCliExecutorGeneric : public ExecutorSync {
	public:

		/// Constructor
		TwCliExecutorGeneric(const std::string& cmd, const std::string& cmdargs)
			: ExecutorSync(cmd, cmdargs)
		{
			this->construct();
		}


		/// Constructor
		TwCliExecutorGeneric()
		{
			this->construct();
		}


		/// Virtual destructor
		virtual ~TwCliExecutorGeneric() = default;


	protected:

		/// Called from constructors
		void construct()
		{
			ExecutorSync::get_command_executor().set_exit_status_translator(&TwCliExecutorGeneric::translate_exit_status, NULL);
			this->set_error_header("An error occurred while executing tw_cli:\n\n");
		}


		/// Exit status translate handler
		static std::string translate_exit_status(int status, void* user_data)
		{
			return std::string();
		}


		/// Import the last error from command executor and clear all errors there
		virtual void import_error()
		{
			Cmdex& cmdex = this->get_command_executor();
			Cmdex::error_list_t errors = cmdex.get_errors();  // these are not clones

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

			cmdex.clear_errors();  // and clear them

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

			// ignore giochannel errors - higher level errors will be triggered, and they more user-friendly.
			if (error_type == "giochannel" || error_type == "custom") {
				return;
			}

			this->set_error_msg(e->get_message());
		}

};




/// tw_cli executor without GUI support
using TwCliExecutor = TwCliExecutorGeneric<CmdexSync>;

/// A reference-counting pointer to TwCliExecutor
using TwCliExecutorRefPtr = hz::intrusive_ptr<TwCliExecutor>;



/// tw_cli executor with GUI support
using TwCliExecutorGui = TwCliExecutorGeneric<CmdexSyncGui>;

/// A reference-counting pointer to TwCliExecutorGui
using TwCliExecutorGuiRefPtr = hz::intrusive_ptr<TwCliExecutorGui>;





/// Executor for cli (Areca utility)
template<class ExecutorSync>
class ArecaCliExecutorGeneric : public ExecutorSync {
	public:

		/// Constructor
		ArecaCliExecutorGeneric(const std::string& cmd, const std::string& cmdargs)
			: ExecutorSync(cmd, cmdargs)
		{
			this->construct();
		}


		/// Constructor
		ArecaCliExecutorGeneric()
		{
			this->construct();
		}


		/// Virtual destructor
		virtual ~ArecaCliExecutorGeneric() = default;


	protected:

		/// Called from constructors
		void construct()
		{
			ExecutorSync::get_command_executor().set_exit_status_translator(&ArecaCliExecutorGeneric::translate_exit_status, NULL);
			this->set_error_header("An error occurred while executing Areca cli:\n\n");
		}


		/// Exit status translate handler
		static std::string translate_exit_status(int status, void* user_data)
		{
			return std::string();
		}


		/// Import the last error from command executor and clear all errors there
		virtual void import_error()
		{
			Cmdex& cmdex = this->get_command_executor();
			Cmdex::error_list_t errors = cmdex.get_errors();  // these are not clones

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

			cmdex.clear_errors();  // and clear them

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

			// ignore giochannel errors - higher level errors will be triggered, and they more user-friendly.
			if (error_type == "giochannel" || error_type == "custom") {
				return;
			}

			this->set_error_msg(e->get_message());
		}

};




/// tw_cli executor without GUI support
using ArecaCliExecutor = ArecaCliExecutorGeneric<CmdexSync>;

/// A reference-counting pointer to ArecaCliExecutor
using ArecaCliExecutorRefPtr = hz::intrusive_ptr<ArecaCliExecutor>;



/// tw_cli executor with GUI support
using ArecaCliExecutorGui = ArecaCliExecutorGeneric<CmdexSyncGui>;

/// A reference-counting pointer to ArecaCliExecutorGui
using ArecaCliExecutorGuiRefPtr = hz::intrusive_ptr<ArecaCliExecutorGui>;






#endif

/// @}
