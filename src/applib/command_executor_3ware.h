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

#ifndef COMMAND_EXECUTOR_3WARE_H
#define COMMAND_EXECUTOR_3WARE_H

#include "local_glibmm.h"

#include "async_command_executor.h"
#include "command_executor.h"




/// Executor for tw_cli (3ware utility)
template<class ExecutorPolicy>
class TwCliExecutorGeneric : public ExecutorPolicy {
	public:

		/// Constructor
		TwCliExecutorGeneric();


	protected:


		/// Exit status translate handler
		static std::string translate_exit_status(int status);


		/// Import the last error from command executor and clear all errors there
		void import_error() override;


		/// This is called when an error occurs in command executor.
		/// Note: The warnings are already printed via debug_* in cmdex.
		void on_error_warn(hz::ErrorBase* e) override;

};



/// tw_cli executor without GUI support
using TwCliExecutor = TwCliExecutorGeneric<CommandExecutor>;


/// tw_cli executor with GUI support
using TwCliExecutorGui = TwCliExecutorGeneric<CommandExecutorGui>;




// ------------------------------------------- Implementation



template<class ExecutorPolicy>
TwCliExecutorGeneric<ExecutorPolicy>::TwCliExecutorGeneric()
{
	ExecutorPolicy::get_async_executor().set_exit_status_translator(&TwCliExecutorGeneric::translate_exit_status);
	this->set_error_header(std::string(_("An error occurred while executing tw_cli:")) + "\n\n");
}



template<class ExecutorPolicy>
std::string TwCliExecutorGeneric<ExecutorPolicy>::translate_exit_status([[maybe_unused]] int status)
{
	return {};
}



template<class ExecutorPolicy>
void TwCliExecutorGeneric<ExecutorPolicy>::import_error()
{
	AsyncCommandExecutor& cmdex = this->get_async_executor();
	AsyncCommandExecutor::error_list_t errors = cmdex.get_errors();  // these are not clones

	hz::ErrorBase* e = nullptr;
	// find the last relevant error.
	for (auto iter = errors.crbegin(); iter != errors.crend(); ++iter) {
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



template<class ExecutorPolicy>
void TwCliExecutorGeneric<ExecutorPolicy>::on_error_warn(hz::ErrorBase* e)
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






#endif

/// @}
