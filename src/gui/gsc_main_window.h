/******************************************************************************
License: GNU General Public License v3.0 only
Copyright:
	(C) 2008 - 2021 Alexander Shaduri <ashaduri@gmail.com>
******************************************************************************/
/// \file
/// \author Alexander Shaduri
/// \ingroup gsc
/// \weakgroup gsc
/// @{

#ifndef GSC_MAIN_WINDOW_H
#define GSC_MAIN_WINDOW_H

#include <map>
#include <gtkmm.h>

#include "applib/app_builder_widget.h"
#include "applib/storage_device.h"



class GscMainWindowIconView;  // defined in gsc_main_window_iconview.h

class GscInfoWindow;  // declared in gsc_info_window.h



/// The main window.
/// Use create() / destroy() with this class instead of new / delete!
class GscMainWindow : public AppBuilderWidget<GscMainWindow, false> {
	public:

		friend class GscMainWindowIconView;  // It needs our privates

		// name of ui file (without .ui extension) for AppBuilderWidget
		static inline const std::string_view ui_name = "gsc_main_window";


		/// Constructor, GtkBuilder needs this.
		GscMainWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui);

		/// Destructor.
		virtual ~GscMainWindow();


		/// Scan for devices and fill the iconview
		void rescan_devices(bool startup);


		/// Add manually added devices to the icon view.
		void add_startup_manual_devices();


		/// Execute update-smart-drivedb
		void run_update_drivedb();


		/// Add device file to icon list, interactively showing errors if any.
		bool add_device_interactive(const std::string& file, const std::string& type_arg, const std::vector<std::string>& extra_args);

		/// Add device file to icon list silently, ignoring errors.
		bool add_device_silent(const std::string& file, const std::string& type_arg, const std::vector<std::string>& extra_args);


	private:

		/// Add device file to icon list
		bool add_device(bool silent, const std::string& file, const std::string& type_arg, const std::vector<std::string>& extra_args);


	public:

		/// Read smartctl data from file, add it as a virtual drive to icon list
		bool add_virtual_drive(const std::string& file);


		/// If at least one drive is having a test performed, return true.
		bool testing_active() const;


		/// Show the info window for the drive
		std::shared_ptr<GscInfoWindow> show_device_info_window(const StorageDevicePtr& drive);

		/// Show "Preferences updated, please rescan" message
		void show_prefs_updated_message();


	protected:

		/// Action type
		enum action_t {
			action_quit,

			action_view_details,
			action_enable_smart,
			action_reread_device_data,
			action_perform_tests,
			action_remove_device,
			action_remove_virtual_device,
			action_add_device,
			action_load_virtual,
			action_rescan_devices,

			action_executor_log,
			action_update_drivedb,
			action_preferences,

			action_online_documentation,
			action_support,
			action_about
		};


		/// Enable/disable items in Drive menu, set toggles in menu items
		void set_drive_menu_status(const StorageDevicePtr& drive);

		/// Get popup menu for a drive
		[[nodiscard]] Gtk::Menu* get_popup_menu(const StorageDevicePtr& drive);

		/// Update status widgets (status area, etc.)
		void update_status_widgets();


		/// Create the widgets - iconview, gtkuimanager stuff (menus), custom labels
		bool create_widgets();

		/// scan and populate iconview widget with drive icons
		void populate_iconview_on_startup(bool smartctl_valid);

		/// Show "Add Device" window
		void show_add_device_chooser();

		/// Show "Load Virtual File" dialog
		void show_load_virtual_file_chooser();


		/// Check smartctl version and set default parser format accordingly.
		/// An error dialog is shown if there is an error with smartctl.
		bool check_smartctl_version_and_set_format();


		/// Called when quit has been requested (by delete event or Quit action)
		void quit_requested();


		// -------------------- callbacks

		// ---------- overriden virtual methods

		/// Quit the application on delete event (by default it calls hide()).
		/// If some test is running, show a question dialog first.
		/// Reimplemented from Gtk::Window.
		bool on_delete_event(GdkEventAny* e) override;


		// ---------- other callbacks


// 		void on_action_activated(Glib::RefPtr<Gtk::Action> action, action_t action_type);

		/// Action activate callback
		void on_action_activated(action_t action_type);

		/// Action callback
		void on_action_enable_smart_toggled(Gtk::ToggleAction* action);

		/// Action callback
		void on_action_reread_device_data();


	private:

		GscMainWindowIconView* iconview_ = nullptr;  ///< The main icon view
		std::vector<StorageDevicePtr> drives_;  ///< Scanned drives

		Glib::RefPtr<Gtk::UIManager> ui_manager_;  ///< UI manager
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_main_;  ///< Action group
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_device_;  ///< Action group
		bool action_handling_enabled_ = true;  ///< Whether action handling is enabled or not
		std::map<action_t, Glib::RefPtr<Gtk::Action> > action_map_;  ///< Used by on_action_activated().

		Gtk::Label* name_label_ = nullptr;  ///< A UI label
		Gtk::Label* health_label_ = nullptr;  ///< A UI label
		Gtk::Label* family_label_ = nullptr;  ///< A UI label

		bool scanning_ = false;  ///< If the scanning is in process or not

};






#endif

/// @}
