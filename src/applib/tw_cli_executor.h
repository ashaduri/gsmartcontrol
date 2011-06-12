/**************************************************************************
 Copyright:
      (C) 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef TW_CLI_EXECUTOR_H
#define TW_CLI_EXECUTOR_H

#include "cmdex.h"
#include "cmdex_sync.h"



/// Executor for tw_cli (3ware utility)
template<class ExecutorSync>
class TwCliExecutorGeneric : public ExecutorSync {

	public:

		TwCliExecutorGeneric(const std::string& cmd, const std::string& cmdargs)
			: ExecutorSync(cmd, cmdargs)
		{
			this->construct();
		}


		TwCliExecutorGeneric()
		{
			this->construct();
		}


		virtual ~TwCliExecutorGeneric()
		{ }



	protected:

		void construct()
		{
			ExecutorSync::cmdex_.set_exit_status_translator(&TwCliExecutorGeneric::translate_exit_status, NULL);
			this->set_error_header("An error occurred while executing tw_cli:\n\n");
		}



		static std::string translate_exit_status(int status, void* user_data)
		{
			return std::string();
		}



		// import the last error from cmdex_ and clear all errors there
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


		// The warnings are already printed via debug_* in cmdex.
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




typedef TwCliExecutorGeneric<CmdexSync> TwCliExecutor;

typedef hz::intrusive_ptr<TwCliExecutor> TwCliExecutorRefPtr;



typedef TwCliExecutorGeneric<CmdexSyncGui> TwCliExecutorGui;

typedef hz::intrusive_ptr<TwCliExecutorGui> TwCliExecutorGuiRefPtr;







#endif
