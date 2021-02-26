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

#ifndef EXECUTOR_FACTORY_H
#define EXECUTOR_FACTORY_H

#include <memory>

#include "cmdex_sync.h"


// Forward declaration
namespace Gtk {
	class Window;
}



/// This class allows you to create new executors for different commands,
/// without carrying the GUI/CLI stuff manually.
class ExecutorFactory {
	public:

		/// Executor type for create_executor()
		enum class ExecutorType {
			Smartctl,
			TwCli,
			ArecaCli
		};


		/// Constructor. If \c use_gui is true, specify \c parent for the GUI dialogs.
		explicit ExecutorFactory(bool use_gui, Gtk::Window* parent = nullptr);


		/// Create a new executor instance according to \c type and the constructor parameters.
		std::shared_ptr<CmdexSync> create_executor(ExecutorType type);


	private:

		bool use_gui_ = false;  ///< Whether to construct GUI executors or not.
		Gtk::Window* parent_ = nullptr;  ///< Parent window for dialogs

};



/// A reference-counting pointer to ExecutorFactory
using ExecutorFactoryPtr = std::shared_ptr<ExecutorFactory>;




#endif

/// @}
