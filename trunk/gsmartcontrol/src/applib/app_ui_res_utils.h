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

#ifndef APP_UI_RES_UTILS_H
#define APP_UI_RES_UTILS_H

#include <string>
#include <gtkmm.h>

#include "hz/hz_config.h"  // feature test macros (ENABLE_*)

// Old glibmm versions had exceptions but didn't define this at all.
// New ones define it to 0 if there are no glibmm exceptions.
#if !defined GLIBMM_EXCEPTIONS_ENABLED || GLIBMM_EXCEPTIONS_ENABLED
	#include <memory>  // std::auto_ptr
#endif

#include "hz/debug.h"
#include "hz/instance_manager.h"
#include "hz/res_data.h"

#include "gui_utils.h"  // gui_show_error_dialog



/// \def APP_UI_RES_DATA_INIT(res_name)
/// Use these in window class definitions to declare ui resources.
/// E.g. APP_UI_RES_DATA_INIT(main_window) will search for
/// main_window.ui in data file search paths.
/// Or, if you're using compiled-in buffers, it will make them available.
#define APP_UI_RES_DATA_INIT(res_name) \
	HZ_RES_DATA_INIT_NAMED(res_name##_ui, #res_name ".ui", UIResDataBase); \
	struct UIResData : public UIResDataBase { \
		UIResData() \
		{ \
			UIResDataBase::root_name = #res_name;  /* we need original name here, not with _ui */ \
		} \
	}



/// \typedef app_ui_res_ref_t
/// A reference-counting pointer to application UI resource
typedef Glib::RefPtr<Gtk::Builder> app_ui_res_ref_t;


/// \fn bool app_ui_res_create_from(app_ui_res_ref_t& ref, const unsigned char* buf, unsigned int buf_size, std::string& error_msg)
/// Create application UI resource from a static buffer.
inline bool app_ui_res_create_from(app_ui_res_ref_t& ref,
		const unsigned char* buf, unsigned int buf_size, std::string& error_msg)
{
	if (!buf_size || !buf || !buf[0]) {
		error_msg = "Cannot load data buffers.";
		return false;
	}

#if !defined GLIBMM_EXCEPTIONS_ENABLED || GLIBMM_EXCEPTIONS_ENABLED
	try {
		// ref->add_from_file("main_window.ui");
		ref->add_from_string(reinterpret_cast<const char*>(buf), static_cast<gsize>(buf_size));
	}
	catch (Gtk::BuilderError& ex) {  // the docs say Glib::MarkupError, but examples say otherwise.
		error_msg = ex.what();
		return false;
	}

#else

	std::auto_ptr<Glib::BuilderError> error;
	ref->add_from_string(reinterpret_cast<const char*>(buf), static_cast<gsize>(buf_size),"", "", error);

	if (error.get()) {
		error_msg = error->what();
		return false;
	}
#endif

	return true;
}




// These allow easy attaching of gtkbuilder widget signals to member functions

/// Connect member function (callback) to signal \c signal_name on widget
/// \c ui_element, where \c ui_element is the widget's gtkbuilder name.
#define APP_UI_RES_CONNECT(ui_element, signal_name, callback) \
	if (true) { \
		if (!ui_element) \
			 this->lookup_object(#ui_element, ui_element); \
		if (ui_element) { \
			ui_element->signal_ ## signal_name ().connect(sigc::mem_fun(*this, &self_type::callback)); \
		} \
	} else (void)0


/// Connect member function (callback) with a name of \c on_<widget_name>_<signal_name>
/// to signal \c signal_name on widget \c ui_element, where \c ui_element is the
/// widget's gtkbuilder name.
#define APP_UI_RES_AUTO_CONNECT(ui_element, signal_name) \
	APP_UI_RES_CONNECT(ui_element, signal_name, on_ ## ui_element ## _ ## signal_name)





/// Inherit this when using GtkBuilder-enabled windows (or any other GtkBuilder-enabled objects).
/// \c Child is the child class that inherits all the functionality of having instance lifetime
/// management and other benefits.
/// If \c MultiInstance is false, create() will return the same instance each time.
template<class Child, bool MultiInstance, class WidgetType = Gtk::Window>
class AppUIResWidget : public WidgetType, public hz::InstanceManager<Child, MultiInstance> {
	public:

		/// Instance class type, which is also the parent class.
		typedef hz::InstanceManager<Child, MultiInstance> instance_class;
		friend class Gtk::Builder;  // allow construction through gtkbuilder
		friend class hz::InstanceManager<Child, MultiInstance>;  // allow construction through instance class


		/// Override parent hz::InstanceManager's function because of non-trivial constructor
		static Child* create()
		{
			if (hz::InstanceManager<Child, MultiInstance>::has_single_instance())  // for single-instance objects
				return hz::InstanceManager<Child, MultiInstance>::get_single_instance();

			std::string error;
			app_ui_res_ref_t ui = Gtk::Builder::create();
			const typename Child::UIResData data;  // this holds the GtkBuilder data

			// this does the actual object construction
			bool success = app_ui_res_create_from(ui, data.buf, data.size, error);

			if (!success) {
				std::string msg = "Fatal error: Cannot create UI-resource widgets: " + error;
				debug_out_fatal("app", msg << "\n");
				gui_show_error_dialog(msg);
				return 0;
			}

			Child* o = 0;
			ui->get_widget_derived(data.root_name, o);

			if (!o) {
				std::string msg = "Fatal error: Cannot get root widget from UI-resource-created hierarchy.";
				debug_out_fatal("app", msg << "\n");
				gui_show_error_dialog(msg);
				return 0;
			}

			o->obj_create();

			hz::InstanceManager<Child, MultiInstance>::set_single_instance(o);  // for single-instance objects

			return o;
		}


		/// Get UI resource
		app_ui_res_ref_t get_ui()
		{
			return ref_ui_;
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
			WidgetPtr w = 0;
			return lookup_widget(name, w);
		}


		/// Find a widget in UI and return it.
		template<typename WidgetPtr>
		WidgetPtr lookup_widget(const Glib::ustring& name, WidgetPtr& w)
		{
			ref_ui_->get_widget(name, w);
			return w;
		}


		/// Find an object in UI and return it.
		Glib::Object* lookup_object(const Glib::ustring& name)
		{
			return ref_ui_->get_object(name).operator->();  // silly RefPtr doesn't have get().
		}


		/// Find an object in UI and return it.
		template<typename ObjectPtr>
		ObjectPtr lookup_object(const Glib::ustring& name)
		{
			ObjectPtr obj = 0;
			return lookup_object(name, obj);
		}


		/// Find an object in UI and return it.
		template<typename ObjectPtr>
		ObjectPtr lookup_object(const Glib::ustring& name, ObjectPtr& obj)
		{
			return (obj = dynamic_cast<ObjectPtr>(lookup_object(name)));  // up, then down
		}



	protected:

		typedef Child self_type;  ///< This is needed by APP_UI_ macros
		typedef WidgetType widget_type;  ///< This is needed by APP_UI_ macros


		// protected constructor / destructor, use create() / destroy() instead of new / delete.

		/// GtkBuilder needs this constructor in a child.
		/// BaseObjectType is a C type, defined in specific Gtk:: widget class.
		AppUIResWidget(typename WidgetType::BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
				: WidgetType(gtkcobj), ref_ui_(ref_ui)
		{
			// manually connecting signals:
			// this->signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_main_window_delete));

			// signals of GtkBuilder-created objects:
			// Gtk::ToolButton* rescan_devices_toolbutton = 0;
			// APP_UI_RES_AUTO_CONNECT(rescan_devices_toolbutton, clicked);

			// show();
		}


		/// Virtual destructor
		virtual ~AppUIResWidget() = default;


	private:

		app_ui_res_ref_t ref_ui_;  ///< UI resource

};






#endif

/// @}
