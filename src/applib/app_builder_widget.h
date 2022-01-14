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

#ifndef APP_BUILDER_WIDGET_H
#define APP_BUILDER_WIDGET_H

#include "local_glibmm.h"
#include <string>
#include <gtkmm.h>
#include <memory>

#include "hz/debug.h"
#include "hz/data_file.h"

#include "window_instance_manager.h"
#include "gui_utils.h"  // gui_show_error_dialog



/// Connect member function (callback) to signal \ref signal_name on widget
/// \ref ui_element, where \ref ui_element is the widget's gtkbuilder name.
/// This allows easy attaching of gtkbuilder widget signals to member functions.
#define APP_BUILDER_CONNECT(ui_element, signal_name, callback) \
	if (true) { \
		if (!(ui_element)) \
			 this->lookup_widget(#ui_element, ui_element); \
		if (ui_element) { \
			(ui_element)->signal_ ## signal_name ().connect(sigc::mem_fun(*this, &std::remove_reference_t<decltype(*this)>::callback)); \
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
class AppBuilderWidget : public WidgetType, public WindowInstanceManager<Child, MultiInstance> {
	public:

		friend class Gtk::Builder;  // allow construction via GtkBuilder
		// friend class WindowInstanceManager<Child, MultiInstance>;  // allow construction through instance class


		/// Disallow
		AppBuilderWidget(const AppBuilderWidget& other) = delete;

		/// Disallow
		AppBuilderWidget(AppBuilderWidget&& other) = delete;

		/// Disallow
		AppBuilderWidget& operator=(const AppBuilderWidget& other) = delete;

		/// Disallow
		AppBuilderWidget& operator=(AppBuilderWidget&& other) = delete;

		/// Default
		~AppBuilderWidget() = default;



		/// Create an instance of this class, returning an existing instance if not MultiInstance.
		/// A glade file in "ui" data domain is loaded with Child::ui_name filename base and is available as
		/// `get_ui()` in child object.
		/// \return nullptr if widget could not be loaded.
		static std::shared_ptr<Child> create();


		/// Get UI resource
		Glib::RefPtr<Gtk::Builder> get_ui();


		/// Find a widget in UI and return it.
		/// \return nullptr if widget was not found.
		Gtk::Widget* lookup_widget(const Glib::ustring& name);


		/// Find a widget in UI and return it.
		/// \return nullptr if widget was not found.
		template<typename WidgetPtr>
		WidgetPtr lookup_widget(const Glib::ustring& name);


		/// Find a widget in UI and return it in \ref w.
		/// \return false if widget was not found.
		template<typename Widget>
		bool lookup_widget(const Glib::ustring& name, Widget*& w);


	protected:


		/// Protected constructor, use `create()` instead.
		/// GtkBuilder needs this constructor in a child.
		/// BaseObjectType is a C type, defined in specific Gtk:: widget class.
		AppBuilderWidget(typename WidgetType::BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);


	private:

		Glib::RefPtr<Gtk::Builder> ui_;  ///< UI resource

};





// ------------------------------------------- Implementation


template<class Child, bool MultiInstance, class WidgetType>
std::shared_ptr<Child> AppBuilderWidget<Child, MultiInstance, WidgetType>::create()
{
	if constexpr(!MultiInstance) {  // for single-instance objects
		if (auto inst = WindowInstanceManager<Child, MultiInstance>::instance()) {
			return inst;
		}
	}

	std::string error_msg;

	auto ui_path = hz::data_file_find("ui", std::string(Child::ui_name) + ".glade");
	try {
		auto ui = Gtk::Builder::create_from_file(ui_path.u8string());  // may throw

		Child* raw_obj = nullptr;
		ui->get_widget_derived({Child::ui_name.data(), Child::ui_name.size()}, raw_obj);  // Calls Child's constructor
		if (!raw_obj) {
			debug_out_fatal("app", "Fatal error: Cannot get root widget from UI-resource-created hierarchy.\n");
			gui_show_error_dialog(_("Fatal error: Cannot get root widget from UI-resource-created hierarchy."));
			return nullptr;
		}

		// Store the instance so it does not get destroyed
		return WindowInstanceManager<Child, MultiInstance>::store_instance(raw_obj);
	}
	catch (Glib::Exception& ex) {
		error_msg = ex.what();
	}

	if (!error_msg.empty()) {
		debug_out_fatal("app", "Fatal error: Cannot create UI-resource widgets: " << error_msg << "\n");
		gui_show_error_dialog(Glib::ustring::compose(_("Fatal error: Cannot create UI-resource widgets: %1"), error_msg));
	}
	return nullptr;
}



template<class Child, bool MultiInstance, class WidgetType>
Glib::RefPtr<Gtk::Builder> AppBuilderWidget<Child, MultiInstance, WidgetType>::get_ui()
{
	return ui_;
}



template<class Child, bool MultiInstance, class WidgetType>
template<typename WidgetPtr>
WidgetPtr AppBuilderWidget<Child, MultiInstance, WidgetType>::lookup_widget(const Glib::ustring& name)
{
	WidgetPtr w = nullptr;
	lookup_widget(name, w);
	return w;
}



template<class Child, bool MultiInstance, class WidgetType>
Gtk::Widget* AppBuilderWidget<Child, MultiInstance, WidgetType>::lookup_widget(const Glib::ustring& name)
{
	return lookup_widget<Gtk::Widget*>(name);
}



template<class Child, bool MultiInstance, class WidgetType>
template<typename Widget>
bool AppBuilderWidget<Child, MultiInstance, WidgetType>::lookup_widget(const Glib::ustring& name, Widget*& w)
{
	ui_->get_widget(name, w);
	return w != nullptr;
}



template<class Child, bool MultiInstance, class WidgetType>
AppBuilderWidget<Child, MultiInstance, WidgetType>::AppBuilderWidget(typename WidgetType::BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
		: WidgetType(gtkcobj), ui_(std::move(ui))
{
	// An example of Child's constructor:

	// Manually connect signals:
	// this->signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_main_window_delete));

	// Automatically connect signals of GtkBuilder-created objects to member functions:
	// Gtk::ToolButton* rescan_devices_toolbutton = 0;
	// APP_BUILDER_AUTO_CONNECT(rescan_devices_toolbutton, clicked);

	// show();
}




#endif

/// @}
