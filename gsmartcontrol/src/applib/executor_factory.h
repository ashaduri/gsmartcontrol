/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef EXECUTOR_FACTORY_H
#define EXECUTOR_FACTORY_H

#include "cmdex_sync.h"
#include "hz/intrusive_ptr.h"


// Forward declaration
namespace Gtk {
	class Window;
}



/// This class allows you to create new executors for different commands,
/// without carrying the GUI/CLI stuff manually.
class ExecutorFactory : public hz::intrusive_ptr_referenced {
	public:

		/// Executor type for create_executor()
		enum Type {
			ExecutorSmartctl,
			ExecutorTwCli
		};


		/// Constructor. If \c use_gui is true, specify \c parent for the GUI dialogs.
		ExecutorFactory(bool use_gui, Gtk::Window* parent = 0);


		/// Create a new executor instance according to \c type and the constructor parameters.
		hz::intrusive_ptr<CmdexSync> create_executor(Type type);


	private:

		bool use_gui_;  ///< Whether to construct GUI executors or not.
		Gtk::Window* parent_;  ///< Parent window for dialogs

};



typedef hz::intrusive_ptr<ExecutorFactory> ExecutorFactoryRefPtr;




#endif
