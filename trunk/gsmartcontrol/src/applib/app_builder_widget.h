/**************************************************************************
 Copyright:
      (C) 2008 - 2018  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup applib
/// \weakgroup applib
/// @{

#ifndef APP_BUILDER_WIDGET_H
#define APP_BUILDER_WIDGET_H

#include <string>
#include <type_traits>
#include <gtkmm.h>

#include "hz/debug.h"
#include "hz/instance_manager.h"
#include "hz/data_file.h"

#include "gui_utils.h"  // gui_show_error_dialog



// These allow easy attaching of gtkbuilder widget signals to member functions

/// Connect member function (callback) to signal \c signal_name on widget
/// \c ui_element, where \c ui_element is the widget's gtkbuilder name.
#define APP_BUILDER_CONNECT(ui_element, signal_name, callback) \
	if (true) { \
		if (!(ui_element)) \
			 this->lookup_widget(#ui_element, ui_element); \
		if (ui_element) { \
			(ui_element)->signal_ ## signal_name ().connect(sigc::mem_fun(*this, &std::remove_pointer_t<decltype(this)>::callback)); \
		} \
	} else (void)0


/// Connect member function (callback) with a name of \c on_<widget_name>_<signal_name>
/// to signal \c signal_name on widget \c ui_element, where \c ui_element is the
/// widget's gtkbuilder name.
#define APP_BUILDER_AUTO_CONNECT(ui_element, signal_name) \
	APP_BUILDER_CONNECT(ui_element, signal_name, on_ ## ui_element ## _ ## signal_name)



/// Inherit this when using GtkBuilder-enabled windows (or any other GtkBuilder-enabled objects).
/// \c Child is the child class that inherits all the functionality of having instance lifetime
/// management and other benefits.
/// If \c MultiInstance is false, create() will return the same instance each time.
template<class Child, bool MultiInstance, class WidgetType = Gtk::Window>
class AppBuilderWidget : public WidgetType, public hz::InstanceManager<Child, MultiInstance> {
	public:

		/// Instance class type, which is also the parent class.
		friend class Gtk::Builder;  // allow construction through gtkbuilder
		friend class hz::InstanceManager<Child, MultiInstance>;  // allow construction through instance class


		/// Override parent hz::InstanceManager's function because of non-trivial constructor
		static Child* create()
		{
			if constexpr(!MultiInstance) {  // for single-instance objects
				if (hz::InstanceManager<Child, MultiInstance>::get_single_instance()) {
					return hz::InstanceManager<Child, MultiInstance>::get_single_instance();
				}
			}

			std::string error_msg;

			auto ui_path = hz::data_file_find("ui", Child::ui_name + ".glade");
			try {
				auto ui = Gtk::Builder::create_from_file(ui_path.u8string());  // may throw

				Child* o = nullptr;
				ui->get_widget_derived(Child::ui_name, o);  // Calls Child's constructor

				if (!o) {
					std::string msg = "Fatal error: Cannot get root widget from UI-resource-created hierarchy.";
					debug_out_fatal("app", msg << "\n");
					gui_show_error_dialog(msg);
					return nullptr;
				}

				if constexpr(!MultiInstance) {
					hz::InstanceManager<Child, MultiInstance>::set_single_instance(o);  // for single-instance objects
				}
				return o;
			}
			catch (Glib::Exception& ex) {
				error_msg = ex.what();
			}

			if (!error_msg.empty()) {
				std::string msg = "Fatal error: Cannot create UI-resource widgets: " + error_msg;
				debug_out_fatal("app", msg << "\n");
				gui_show_error_dialog(msg);
			}
			return nullptr;
		}


		/// Get UI resource
		Glib::RefPtr<Gtk::Builder> get_ui()
		{
			return ui_;
		}


		/// Find a widget in UI and return it.
		Gtk::Widget* lookup_widget(const Glib::ustring& name)
		{
			return lookup_widget<Gtk::Widget*>(name);
		}


		/// Find a widget in UI and return it.
		template<typename WidgetPtr>
		WidgetPtr lookup_widget(const Glib::ustring& name)
		{
			WidgetPtr w = nullptr;
			return lookup_widget(name, w);
		}


		/// Find a widget in UI and return it.
		template<typename Widget>
		Widget* lookup_widget(const Glib::ustring& name, Widget*& w)
		{
			ui_->get_widget(name, w);
			return w;
		}


	protected:

		// protected constructor / destructor, use create() / destroy() instead of new / delete.

		/// GtkBuilder needs this constructor in a child.
		/// BaseObjectType is a C type, defined in specific Gtk:: widget class.
		AppBuilderWidget(typename WidgetType::BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
				: WidgetType(gtkcobj), ui_(std::move(ui))
		{
			// manually connecting signals:
			// this->signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_main_window_delete));

			// signals of GtkBuilder-created objects:
			// Gtk::ToolButton* rescan_devices_toolbutton = 0;
			// APP_BUILDER_AUTO_CONNECT(rescan_devices_toolbutton, clicked);

			// show();
		}


		/// Virtual destructor
		~AppBuilderWidget() = default;


	private:

		Glib::RefPtr<Gtk::Builder> ui_;  ///< UI resource

};






#endif

/// @}
