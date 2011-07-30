/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
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
#include <gtkmm/window.h>

#include "hz/hz_config.h"  // feature test macros (ENABLE_*)

#if defined ENABLE_LIBGLADE && ENABLE_LIBGLADE
	#include <libglademm.h>
#elif defined ENABLE_GTKBUILDER && ENABLE_GTKBUILDER
	#include <gtkmm/builder.h>
#endif

// Old glibmm versions had exceptions but didn't define this at all.
// New ones define it to 0 if there are no glibmm exceptions.
#if !defined GLIBMM_EXCEPTIONS_ENABLED || GLIBMM_EXCEPTIONS_ENABLED
	#include <memory>  // std::auto_ptr
#endif

#include "app_gtkmm_features.h"  // APP_GTKMM_OLD_TOOLTIPS

#if defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS
	#include <gtk/gtk.h>  // gtk_tooltips_*
#endif

#include "hz/debug.h"
#include "hz/instance_manager.h"
#include "hz/down_cast.h"
#include "hz/res_data.h"

#include "gui_utils.h"  // gui_show_error_dialog



/// \def APP_UI_RES_DATA_INIT(res_name)
/// Use these in window class definitions to declare ui resources.
/// E.g. APP_UI_RES_DATA_INIT(main_window) will search for
/// main_window.glade or main_window.ui (depending on whether
/// you're using libglade or gtkbuilder) in data file search paths.
/// Or, if you're using compiled-in buffers, it will make them available.
#if defined ENABLE_LIBGLADE && ENABLE_LIBGLADE

	#define APP_UI_RES_DATA_INIT(res_name) \
		HZ_RES_DATA_INIT_NAMED(res_name##_ui, #res_name ".glade", UIResDataBase); \
		struct UIResData : public UIResDataBase { \
			UIResData() \
			{ \
				UIResDataBase::root_name = #res_name;  /* we need original name here, not with _ui */ \
			} \
		}


#elif defined ENABLE_GTKBUILDER && ENABLE_GTKBUILDER

	#define APP_UI_RES_DATA_INIT(res_name) \
		HZ_RES_DATA_INIT_NAMED(res_name##_ui, #res_name ".ui", UIResDataBase); \
		struct UIResData : public UIResDataBase { \
			UIResData() \
			{ \
				UIResDataBase::root_name = #res_name;  /* we need original name here, not with _ui */ \
			} \
		}


#else  // none, just avoid errors

	#define APP_UI_RES_DATA_INIT(res_name) \
		HZ_RES_DATA_DUMMY_INIT_NAMED(res_name##_ui, 0, UIResDataBase); \
		struct UIResData : public UIResDataBase { \
			UIResData() \
			{ \
				UIResDataBase::root_name = #res_name;  /* we need original name here, not with _ui */ \
			} \
		}

#endif




/// \typedef app_ui_res_ref_t
/// A reference-counting pointer to application UI resource

/// \fn bool app_ui_res_create_from(app_ui_res_ref_t& ref, const unsigned char* buf, unsigned int buf_size, std::string& error_msg)
/// Create application UI resource from a static buffer.

#if defined ENABLE_LIBGLADE && ENABLE_LIBGLADE

	typedef Glib::RefPtr<Gnome::Glade::Xml> app_ui_res_ref_t;


	inline bool app_ui_res_create_from(app_ui_res_ref_t& ref,
			const unsigned char* buf, unsigned int buf_size, std::string& error_msg)
	{
		if (!buf_size || !buf || !buf[0]) {
			error_msg = "Cannot load data buffers.";
			return false;
		}

	#if !defined GLIBMM_EXCEPTIONS_ENABLED || GLIBMM_EXCEPTIONS_ENABLED
		try {
			// Glib::RefPtr<Gnome::Glade::Xml> ref = Gnome::Glade::Xml::create("main_window.glade");
			ref = Gnome::Glade::Xml::create_from_buffer(reinterpret_cast<const char*>(buf),
					static_cast<int>(buf_size));
		}
		catch (Gnome::Glade::XmlError& ex) {
			error_msg = ex.what();
			return false;
		}

	#else

		std::auto_ptr<Gnome::Glade::XmlError> error;
		ref = Gnome::Glade::Xml::create_from_buffer(reinterpret_cast<const char*>(buf),
				static_cast<int>(buf_size),"", "", error);

		if (error.get()) {
			error_msg = error->what();
			return false;
		}
	#endif

		return true;
	}


#elif defined ENABLE_GTKBUILDER && ENABLE_GTKBUILDER

	typedef Glib::RefPtr<Gtk::Builder> app_ui_res_ref_t;


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


#else  // none, just avoid compilation errors

	/// A dummy structure that can be used to emulate a (non-functional) resource.
	struct AppUIResDummy {
		template<typename T>
		void get_widget_derived(const Glib::ustring& name, T*& widget)
		{ }
		template<typename T>
		void get_widget(const Glib::ustring& name, T*& widget)
		{ }
	};

	typedef AppUIResDummy* app_ui_res_ref_t;

	inline bool app_ui_res_create_from(app_ui_res_ref_t& ref,
			const unsigned char* buf, unsigned int buf_size, std::string& error_msg)
	{
		error_msg = "Neither Gtk::Builder (gtkmm >= 2.12) nor libglademm were present during compilation."
				" Recompile when either dependency is resolved.";
		return false;
	}

#endif




// These allow easy attaching of glade widget signals to member functions

/// Connect member function (callback) to signal \c signal_name on widget
/// \c ui_element, where \c ui_element is the widget's glade/gtkbuilder name.
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
/// widget's glade/gtkbuilder name.
#define APP_UI_RES_AUTO_CONNECT(ui_element, signal_name) \
	APP_UI_RES_CONNECT(ui_element, signal_name, on_ ## ui_element ## _ ## signal_name)





/// Inherit this when using Glade-enabled windows (or any other glade-enabled objects).
/// \c Child is the child class that inherits all the functionality of having instance lifetime
/// management and other benefits.
/// If \c MultiInstance is false, create() will return the same instance each time.
template<class Child, bool MultiInstance, class WidgetType = Gtk::Window>
class AppUIResWidget : public WidgetType, public hz::InstanceManager<Child, MultiInstance> {
	public:

		/// Instance class type, which is also the parent class.
		typedef hz::InstanceManager<Child, MultiInstance> instance_class;

#if defined ENABLE_LIBGLADE && ENABLE_LIBGLADE
		friend class Gnome::Glade::Xml;  // allow construction through libglade
#elif defined ENABLE_GTKBUILDER && ENABLE_GTKBUILDER
		friend class Gtk::Builder;  // allow construction through gtkbuilder
#endif
		friend class hz::InstanceManager<Child, MultiInstance>;  // allow construction through instance class


		/// Override parent hz::InstanceManager's function because of non-trivial constructor
		static Child* create()
		{
			if (hz::InstanceManager<Child, MultiInstance>::has_single_instance())  // for single-instance objects
				return hz::InstanceManager<Child, MultiInstance>::get_single_instance();

			std::string error;
#if defined ENABLE_LIBGLADE && ENABLE_LIBGLADE
			app_ui_res_ref_t ui;
#elif defined ENABLE_GTKBUILDER && ENABLE_GTKBUILDER
			app_ui_res_ref_t ui = Gtk::Builder::create();
#else  // none, just avoid compilation errors
			app_ui_res_ref_t ui = 0;
#endif
			const typename Child::UIResData data;  // this holds the glade data

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
#if defined ENABLE_LIBGLADE && ENABLE_LIBGLADE
			return static_cast<Glib::Object*>(lookup_widget(name));  // all glade objects are widgets, it's an upcast
#elif defined ENABLE_GTKBUILDER && ENABLE_GTKBUILDER
			return ref_ui_->get_object(name).operator->();  // silly RefPtr doesn't have get().
#else  // none, just avoid compilation errors
			return 0;
#endif
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
			return (obj = hz::down_cast<ObjectPtr>(lookup_object(name)));  // up, then down
		}



	protected:

		typedef Child self_type;  ///< This is needed by APP_GLADE_ macros
		typedef WidgetType widget_type;  ///< This is needed by APP_GLADE_ macros


		// protected constructor / destructor, use create() / destroy() instead of new / delete.

		// Glade needs this constructor in a child.
		// BaseObjectType is a C type, defined in specific Gtk:: widget class.
		AppUIResWidget(typename WidgetType::BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
				: WidgetType(gtkcobj), ref_ui_(ref_ui)
		{
			// if gtkmm is older than 2.12, use the old tooltips API
#if defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS
			// attach it to the window
			this->create_tooltips_data(this);
#endif


			// manually connecting signals:
			// this->signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_main_window_delete));

			// signals of glade-created objects:
			// Gtk::ToolButton* rescan_devices_toolbutton = 0;
			// APP_UI_RES_AUTO_CONNECT(rescan_devices_toolbutton, clicked);

			// show();
		}


		/// Virtual destructor
		virtual ~AppUIResWidget()
		{ }


#if defined APP_GTKMM_OLD_TOOLTIPS && APP_GTKMM_OLD_TOOLTIPS
		// differentiate Window and its children from other widgets through overloading

		void create_tooltips_data(Gtk::Window* window)
		{
			if (window) {
				// gtkmm tooltips api is somewhat broken (no create?), so use gtk
				if (!window->get_data("window_tooltips")) {
					window->set_data("window_tooltips", gtk_tooltips_new());
				}
			}
		}

		void create_tooltips_data(Gtk::Object* o)
		{ }
#endif



	protected:

// 		void on_rescan_devices_toolbutton_clicked()
// 		{
// 		}


	private:

		app_ui_res_ref_t ref_ui_;  ///< UI resource

};






#endif

/// @}
