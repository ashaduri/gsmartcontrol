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

#ifndef COMMAND_EXECUTOR_ARECA_H
#define COMMAND_EXECUTOR_ARECA_H

#include <ranges>

#include "local_glibmm.h"

#include "async_command_executor.h"
#include "command_executor.h"




/// Executor for cli (Areca utility)
template<class ExecutorPolicy>
class ArecaCliExecutorGeneric : public ExecutorPolicy {
	public:

		/// Constructor
		ArecaCliExecutorGeneric();


	protected:


		/// Exit status translate handler
		static std::string translate_exit_status([[maybe_unused]] [[maybe_unused]] int status);


		/// Import the last error from command executor and clear all errors there
		void import_error() override;


		/// This is called when an error occurs in command executor.
		/// Note: The warnings are already printed via debug_* in cmdex.
		void on_error_warn(hz::ErrorBase* e) override;

};



/// tw_cli executor without GUI support
using ArecaCliExecutor = ArecaCliExecutorGeneric<CommandExecutor>;


/// tw_cli executor with GUI support
using ArecaCliExecutorGui = ArecaCliExecutorGeneric<CommandExecutorGui>;




// ------------------------------------------- Implementation



template<class ExecutorPolicy>
ArecaCliExecutorGeneric<ExecutorPolicy>::ArecaCliExecutorGeneric()
{
	ExecutorPolicy::get_async_executor().set_exit_status_translator(&ArecaCliExecutorGeneric::translate_exit_status);
	this->set_error_header(std::string(_("An error occurred while executing Areca cli:")) + "\n\n");
}



template<class ExecutorPolicy>
std::string ArecaCliExecutorGeneric<ExecutorPolicy>::translate_exit_status([[maybe_unused]] int status)
{
	return std::string();
}



template<class ExecutorPolicy>
void ArecaCliExecutorGeneric<ExecutorPolicy>::import_error()
{
	AsyncCommandExecutor& cmdex = this->get_async_executor();
	AsyncCommandExecutor::error_list_t errors = cmdex.get_errors();  // these are not clones

	hz::ErrorBase* e = nullptr;
	// find the last relevant error.
	for (const auto& error : std::ranges::reverse_view(errors)) {
		// ignore iochannel errors, they may mask the real errors
		if (error->get_type() != "giochannel" && error->get_type() != "custom") {
			e = error->clone();
			break;
		}
	}

	cmdex.clear_errors();  // and clear them

	if (e) {  // if error is present, alert the user
		on_error_warn(e);
	}
}



template<class ExecutorPolicy>
void ArecaCliExecutorGeneric<ExecutorPolicy>::on_error_warn(hz::ErrorBase* e)
{
	if (!e)
		return;

	// import the error only if it's relevant.
	const std::string error_type = e->get_type();

	// ignore giochannel errors - higher level errors will be triggered, and they more user-friendly.
	if (error_type == "giochannel" || error_type == "custom") {
		return;
	}

	this->set_error_msg(e->get_message());
}






#endif

/// @}
