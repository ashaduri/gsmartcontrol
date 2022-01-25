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

#include "local_glibmm.h"
#include <gtkmm.h>
#include <vector>
#include <memory>

#include "hz/string_algo.h"  // string_split
#include "hz/string_num.h"
#include "hz/debug.h"
#include "hz/launch_url.h"
#include "hz/fs.h"
#include "rconfig/rconfig.h"
#include "applib/storage_detector.h"
#include "applib/smartctl_ata_text_parser.h"
#include "applib/gui_utils.h"  // gui_show_error_dialog
#include "applib/smartctl_executor.h"  // get_smartctl_binary()
#include "applib/smartctl_executor_gui.h"
#include "applib/app_gtkmm_tools.h"  // app_gtkmm_*
#include "applib/warning_colors.h"  // app_property_get_label_highlight_color
#include "applib/app_pcrecpp.h"  // app_pcre_match

#include "gsc_init.h"  // app_quit()
#include "gsc_about_dialog.h"
#include "gsc_info_window.h"
#include "gsc_preferences_window.h"
#include "gsc_executor_log_window.h"
#include "gsc_executor_error_dialog.h"  // gsc_executor_error_dialog_show

#include "gsc_main_window_iconview.h"
#include "gsc_main_window.h"
#include "gsc_add_device_window.h"
#include "applib/command_executor_factory.h"
#include "gsc_startup_settings.h"



using namespace std::literals;


GscMainWindow::GscMainWindow(BaseObjectType* gtkcobj, Glib::RefPtr<Gtk::Builder> ui)
		: AppBuilderWidget<GscMainWindow, false, Gtk::ApplicationWindow>(gtkcobj, std::move(ui))
{
	// iconview, gtkuimanager stuff (menus), custom labels
	create_widgets();

	// Size
	{
		int def_size_w = rconfig::get_data<int>("gui/main_window/default_size_w");
		int def_size_h = rconfig::get_data<int>("gui/main_window/default_size_h");
		if (def_size_w > 0 && def_size_h > 0) {
			set_default_size(def_size_w, def_size_h);
		}
	}

	// show the window first, scan later
	show();

	// Position (after the window has been shown)
	{
		int pos_x = rconfig::get_data<int>("gui/main_window/default_pos_x");
		int pos_y = rconfig::get_data<int>("gui/main_window/default_pos_y");
		if (pos_x > 0 && pos_y > 0) {  // to avoid situations where positions are not supported
			this->move(pos_x, pos_y);
		}
	}

	while (Gtk::Main::events_pending())  // allow the window to show
		Gtk::Main::iteration();


	// Check if smartctl is executable

	std::string error_msg;
	bool show_output_button = true;

	do {
		std::string smartctl_binary = get_smartctl_binary().u8string();

		// Don't use default options here - they are used when invoked
		// with a device option.
// 		std::string smartctl_def_options = rconfig::get_data<std::string>("system/smartctl_options");

		if (smartctl_binary.empty()) {
			error_msg = _("Smartctl binary is not specified in configuration.");
			show_output_button = false;
			break;
		}

// 		if (!smartctl_def_options.empty())
// 			smartctl_def_options += " ";

		SmartctlExecutorGui ex;
		ex.create_running_dialog(this);
		ex.set_running_msg(_("Checking if smartctl is executable..."));

// 		ex.set_command(Glib::shell_quote(smartctl_binary), smartctl_def_options + "-V");  // --version
		ex.set_command(Glib::shell_quote(smartctl_binary), "-V");  // --version

		if (!ex.execute() || !ex.get_error_msg().empty()) {
			error_msg = ex.get_error_msg();
			break;
		}

		std::string output = ex.get_stdout_str();
		if (output.empty()) {
			error_msg = _("Smartctl returned an empty output.");
			break;
		}

		std::string version, version_full;
		if (!SmartctlAtaTextParser::parse_version(output, version, version_full)) {
			error_msg = _("Smartctl returned invalid output.");
			break;
		}

		{
			// We require this version at runtime to support --get=all.
			const double minimum_req_version = 5.43;
			double version_double = 0;
			if (hz::string_is_numeric_nolocale<double>(version, version_double, false)) {
				if (version_double < minimum_req_version) {
					error_msg = Glib::ustring::compose(_("Smartctl version %1 found, %2 required."), version, hz::number_to_string_nolocale(minimum_req_version));
					break;
				}
			}
		}

	} while (false);

	bool smartctl_valid = error_msg.empty();
	if (!smartctl_valid) {
		gsc_executor_error_dialog_show(_("There was an error while executing smartctl"),
				error_msg + "\n\n<i>" + _("Please specify the correct smartctl binary in Preferences.") + "</i>",
				this, true, show_output_button);
	}

	// Scan
	populate_iconview(smartctl_valid);
}



GscMainWindow::~GscMainWindow()
{
	// This is needed because for some reason, if any icon is selected,
	// on_iconview_selection_changed() is called even after the window is deleted,
	// causing crash on exit.
	// iconview_->clear_all();
	delete iconview_;
}



void GscMainWindow::populate_iconview(bool smartctl_valid)
{
	if (!smartctl_valid) {
		iconview_->set_empty_view_message(GscMainWindowIconView::Message::no_smartctl);
		iconview_->clear_all();  // the message won't be shown without invalidating the region.
		while (Gtk::Main::events_pending())  // give expose event the time it needs
			Gtk::Main::iteration();

	} else if (rconfig::get_data<bool>("gui/scan_on_startup")  // config option
			&& !get_startup_settings().no_scan) {  // command-line option
		rescan_devices();  // scan for devices and fill the iconview

	} else {
		iconview_->set_empty_view_message(GscMainWindowIconView::Message::scan_disabled);
		iconview_->clear_all();  // the message won't be shown without invalidating the region.
		while (Gtk::Main::events_pending())  // give expose event the time it needs
			Gtk::Main::iteration();
	}

	// Add command-line-requested devices and virtual drives.

	if (smartctl_valid) {
		for (auto&& dev_with_type : get_startup_settings().add_devices) {
			if (!dev_with_type.empty()) {
				std::vector<std::string> parts;
				hz::string_split(dev_with_type, "::", parts, false);
				std::string file = (!parts.empty() ? parts.at(0) : std::string());
				std::string type_arg = (parts.size() > 1 ? parts.at(1) : std::string());
				std::string extra_args = (parts.size() > 2 ? parts.at(2) : std::string());
				if (!file.empty()) {
					add_device(file, type_arg, extra_args);
				}
			}
		}
	}

	for (auto&& virt : get_startup_settings().load_virtuals) {
		if (!virt.empty()) {
			add_virtual_drive(virt);
		}
	}

	// update the menus (group sensitiveness, etc...)
	iconview_->update_menu_actions();
	this->update_status_widgets();
}




// pass enum elements here
#define APP_ACTION_NAME(a) #a


bool GscMainWindow::create_widgets()
{
	// --------------------------------- Icon View

	get_ui()->get_widget_derived("drive_iconview", iconview_);  // fill our iconview and do the rest
	DBG_ASSERT_RETURN(iconview_, false);

	iconview_->set_main_window(this);


	// --------------------------------- Actions

	actiongroup_main_ = Gio::SimpleActionGroup::create();
	actiongroup_device_ = Gio::SimpleActionGroup::create();

	// Create actions:

	actiongroup_main_->add_action("quit",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_quit));

	// FIXME tooltips

	// _("View detailed information")
	actiongroup_device_->add_action("view_details",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_view_details));

	// _("Toggle SMART status. The status will be preserved at least until reboot (unless you toggle it again).")
	toggle_enable_smart_action_ = actiongroup_device_->add_action_bool("enable_smart",
			sigc::mem_fun(*this, &GscMainWindow::on_action_enable_smart_toggled));
	// FIXME
	// lookup_widget<Gtk::CheckButton*>("status_smart_enabled_check")->set_related_action(action);

	// _("Toggle Automatic Offline Data Collection which will update \"offline\" SMART attributes every four hours")
	toggle_enable_aodc_action_ = actiongroup_device_->add_action_bool("enable_aodc",
			sigc::mem_fun(*this, &GscMainWindow::on_action_enable_aodc_toggled));
	// FIXME
	// lookup_widget<Gtk::CheckButton*>("status_aodc_enabled_check")->set_related_action(action);

	// _("Re-read basic SMART data")
	actiongroup_device_->add_action("reread_device_data",
			sigc::mem_fun(*this, &GscMainWindow::on_action_reread_device_data));

	// _("Perform various self-tests on the drive")
	actiongroup_device_->add_action("perform_tests",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_perform_tests));

	// _("Remove previously added device")
	actiongroup_device_->add_action("remove_device",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_remove_device));

	// _("Remove previously loaded virtual device")
	actiongroup_device_->add_action("remove_virtual_device",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_remove_virtual_device));


	// _("Manually add device to device list")
	actiongroup_main_->add_action("add_device",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_add_device));

	// _("Load smartctl output from a text file as a read-only virtual device")
	actiongroup_main_->add_action("load_virtual",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_load_virtual));

	// _("Re-scan device list")
	actiongroup_main_->add_action("rescan_devices",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_rescan_devices));


	actiongroup_main_->add_action("executor_log",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_executor_log));

	actiongroup_main_->add_action("update_drivedb",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_update_drivedb));

	actiongroup_main_->add_action("preferences",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_preferences));


	actiongroup_main_->add_action("online_documentation",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_online_documentation));

	actiongroup_main_->add_action("support",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_support));

	actiongroup_main_->add_action("about",
			sigc::bind(sigc::mem_fun(*this, &GscMainWindow::on_action_activated), action_about));

	insert_action_group("main", actiongroup_main_);
	insert_action_group("device", actiongroup_device_);


	// Lay out the actions in a menubar and context menu
	Glib::ustring ui_info =
R"(<interface>
	<menu id='main_menubar'>
		<submenu>
			<attribute name='label' translatable='yes'>_File</attribute>
			<section>
				<item>
					<attribute name='label' translatable='yes'>_Quit</attribute>
					<attribute name='action'>main.quit</attribute>
					<attribute name='accel'>&lt;Primary&gt;n</attribute>
					<attribute name="icon">application-exit</attribute>
				</item>
			</section>
		</submenu>
		<submenu>
			<attribute name='label' translatable='yes'>_Device</attribute>
			<section>
				<item>
					<attribute name='label' translatable='yes'>_View details</attribute>
					<attribute name='action'>device.view_details</attribute>
					<attribute name='accel'>&lt;Primary&gt;v</attribute>
					<attribute name="icon">dialog-information</attribute>
				</item>
			</section>
			<section>
				<item>
					<attribute name='label' translatable='yes'>Enable SMART</attribute>
					<attribute name='action'>device.enable_smart</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Enable Auto Offline Data Collection</attribute>
					<attribute name='action'>device.enable_aodc</attribute>
				</item>
			</section>
			<section>
				<item>
					<attribute name='label' translatable='yes'>Re-read Data</attribute>
					<attribute name='action'>device.reread_device_data</attribute>
					<attribute name='accel'>&lt;Primary&gt;e</attribute>
					<attribute name="icon">view-refresh</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Perform _Tests...</attribute>
					<attribute name='action'>device.perform_tests</attribute>
					<attribute name='accel'>&lt;Primary&gt;t</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Re_move Added Device</attribute>
					<attribute name='action'>device.remove_device</attribute>
					<attribute name='accel'>&lt;Primary&gt;w</attribute>
					<attribute name="icon">list-remove</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Re_move Virtual Device</attribute>
					<attribute name='action'>device.remove_virtual_device</attribute>
					<attribute name='accel'>Delete</attribute>
					<attribute name="icon">list-remove</attribute>
				</item>
			</section>
			<section>
				<item>
					<attribute name='label' translatable='yes'>_Add Device...</attribute>
					<attribute name='action'>main.add_device</attribute>
					<attribute name='accel'>&lt;Primary&gt;d</attribute>
					<attribute name="icon">document-open</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>_Load Smartctl Output as Virtual Device...</attribute>
					<attribute name='action'>main.load_virtual</attribute>
					<attribute name='accel'>&lt;Primary&gt;o</attribute>
					<attribute name="icon">document-open</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>_Re-scan Device List</attribute>
					<attribute name='action'>main.rescan_devices</attribute>
					<attribute name='accel'>&lt;Primary&gt;r</attribute>
					<attribute name="icon">view-refresh</attribute>
				</item>
			</section>
		</submenu>
		<submenu>
			<attribute name='label' translatable='yes'>_Options</attribute>
			<section>
				<item>
					<attribute name='label' translatable='yes'>View Execution Log</attribute>
					<attribute name='action'>main.executor_log</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Update Drive Database</attribute>
					<attribute name='action'>main.update_drivedb</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Preferences</attribute>
					<attribute name='action'>main.preferences</attribute>
					<attribute name='accel'>&lt;Alt&gt;p</attribute>
					<attribute name="icon">preferences-system</attribute>
				</item>
			</section>
		</submenu>
		<submenu>
			<attribute name='label' translatable='yes'>_Help</attribute>
			<section>
				<item>
					<attribute name='label' translatable='yes'>Help</attribute>
					<attribute name='action'>main.online_documentation</attribute>
					<attribute name='accel'>F1</attribute>
					<attribute name="icon">help-browser</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>Support</attribute>
					<attribute name='action'>main.support</attribute>
				</item>
				<item>
					<attribute name='label' translatable='yes'>About GSmartControl...</attribute>
					<attribute name='action'>main.about</attribute>
					<attribute name="icon">help-about</attribute>
				</item>
			</section>
		</submenu>
	</menu>

	<menu id='device_popup'>
		<section>
			<item>
				<attribute name='label' translatable='yes'>_View details</attribute>
				<attribute name='action'>device.view_details</attribute>
				<attribute name='accel'>&lt;Primary&gt;v</attribute>
				<attribute name="icon">dialog-information</attribute>
			</item>
		</section>
		<section>
			<item>
				<attribute name='label' translatable='yes'>Enable SMART</attribute>
				<attribute name='action'>device.enable_smart</attribute>
			</item>
			<item>
				<attribute name='label' translatable='yes'>Enable Auto Offline Data Collection</attribute>
				<attribute name='action'>device.enable_aodc</attribute>
			</item>
		</section>
		<section>
			<item>
				<attribute name='label' translatable='yes'>Re-read Data</attribute>
				<attribute name='action'>device.reread_device_data</attribute>
				<attribute name='accel'>&lt;Primary&gt;e</attribute>
				<attribute name="icon">view-refresh</attribute>
			</item>
			<item>
				<attribute name='label' translatable='yes'>Perform _Tests...</attribute>
				<attribute name='action'>device.perform_tests</attribute>
				<attribute name='accel'>&lt;Primary&gt;t</attribute>
			</item>
			<item>
				<attribute name='label' translatable='yes'>Re_move Added Device</attribute>
				<attribute name='action'>device.remove_device</attribute>
				<attribute name='accel'>&lt;Primary&gt;w</attribute>
				<attribute name="icon">list-remove</attribute>
			</item>
			<item>
				<attribute name='label' translatable='yes'>Re_move Virtual Device</attribute>
				<attribute name='action'>device.remove_virtual_device</attribute>
				<attribute name='accel'>Delete</attribute>
				<attribute name="icon">list-remove</attribute>
			</item>
		</section>
		<section>
			<item>
				<attribute name='label' translatable='yes'>_Add Device...</attribute>
				<attribute name='action'>main.add_device</attribute>
				<attribute name='accel'>&lt;Primary&gt;d</attribute>
				<attribute name="icon">document-open</attribute>
			</item>
			<item>
				<attribute name='label' translatable='yes'>_Load Smartctl Output as Virtual Device...</attribute>
				<attribute name='action'>main.load_virtual</attribute>
				<attribute name='accel'>&lt;Primary&gt;o</attribute>
				<attribute name="icon">document-open</attribute>
			</item>
			<item>
				<attribute name='label' translatable='yes'>_Re-scan Device List</attribute>
				<attribute name='action'>main.rescan_devices</attribute>
				<attribute name='accel'>&lt;Primary&gt;r</attribute>
				<attribute name="icon">view-refresh</attribute>
			</item>
		</section>
	</menu>
</interface>
)";

	ui_builder_ = Gtk::Builder::create();

	// add accelerator group to our window so that they work
	// FIXME
	// add_accel_group(ui_builder_->get_accel_group());


	try {
		ui_builder_->add_from_string(ui_info);
	}
	catch(Glib::Error& ex)
	{
		debug_out_error("app", DBG_FUNC_MSG << "UI creation failed: " << ex.what() << "\n");
		return false;
	}


	// FIXME
	// add some more accelerators (in addition to existing ones)
	// Gtk::Widget* rescan_item = ui_builder_->get_widget("/main_menubar/device_menu/" APP_ACTION_NAME(action_rescan_devices));
	// if (rescan_item)
	// 	rescan_item->add_accelerator("activate", get_accel_group(), GDK_KEY_F5, Gdk::ModifierType(0), Gtk::AccelFlags(0));


	// look after the created widgets
	// FIXME
	// auto* menubar_vbox = lookup_widget<Gtk::Box*>("menubar_vbox");
	// Gtk::Widget* menubar = ui_builder_->get_widget("/main_menubar");
	// if (menubar_vbox && menubar) {
	// 	menubar_vbox->pack_start(*menubar, Gtk::PACK_EXPAND_WIDGET);
	// 	menubar->set_hexpand(true);
// 		menubar->set_halign(Gtk::ALIGN_START);
// 	}


	// Set tooltips on menu items - gtk does that only on toolbar items.
	// FIXME
	// Glib::ustring tooltip_text;
	// std::vector<Glib::RefPtr<Gtk::ActionGroup> > groups = ui_builder_->get_action_groups();
	// for (auto& group : groups) {
	// 	std::vector<Glib::RefPtr<Gtk::Action> > actions = group->get_actions();
	// 	for (auto& group_action : actions) {
	// 		std::vector<Gtk::Widget*> widgets = group_action->get_proxies();
	// 		if (!(tooltip_text = group_action->property_tooltip()).empty()) {
	// 			for (auto& widget : widgets) {
	// 				app_gtkmm_set_widget_tooltip(*widget, tooltip_text, true);
	// 			}
	// 		}
	// 	}
	// }


	// ----------------------------------------- Labels

	// create and add labels
	auto* name_label_box = lookup_widget<Gtk::Box*>("status_name_label_hbox");
	name_label_ = Gtk::manage(new Gtk::Label(_("No drive selected"), Gtk::ALIGN_START));
	name_label_->set_line_wrap(true);
	name_label_->set_selectable(true);
	name_label_->show();
	name_label_box->pack_start(*name_label_, true, true);

	auto* health_label_box = lookup_widget<Gtk::Box*>("status_health_label_hbox");
	health_label_ = Gtk::manage(new Gtk::Label(_("No drive selected"), Gtk::ALIGN_START));
	health_label_->set_line_wrap(true);
	health_label_->set_selectable(true);
	health_label_->show();
	health_label_box->pack_start(*health_label_, true, true);

	auto* family_label_box = lookup_widget<Gtk::Box*>("status_family_label_hbox");
	family_label_ = Gtk::manage(new Gtk::Label(_("No drive selected"), Gtk::ALIGN_START));
	family_label_->set_line_wrap(true);
	family_label_->set_selectable(true);
	family_label_->show();
	family_label_box->pack_start(*family_label_, true, true);

	return true;
}



namespace {

	/// Return true if the user agrees to quit
	inline bool ask_about_quit_on_test(Gtk::Window& parent)
	{
		int status = 0;
		{
			Gtk::MessageDialog dialog(parent,
					"\n"s + _("One of the drives is performing a test. Do you really want to quit?")
					+ "\n\n<small>" + _("The test will continue to run in the background, but you won't be"
					" able to monitor it using GSmartControl.") + "</small>",
					true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
			status = dialog.run();
		}
		return (status == Gtk::RESPONSE_YES);
	}
	
}




// NOTE: Do NOT bind the Glib::RefPtr<Gtk::Action> parameter to this function.
// Doing so causes valgrind errors on window destroy (and Send Report dialogs on win32).
// Probably a gtkmm bug. Use action maps or Gtk::Action* instead.
void GscMainWindow::on_action_activated(GscMainWindow::action_t action_type)
{
	if (!this->action_handling_enabled_)  // check if we should do something
		return;

	if (action_map_.find(action_type) == action_map_.end()) {
		debug_out_error("app", DBG_FUNC_MSG << "Invalid action activated: " << static_cast<int>(action_type) << ".\n");
		return;
	}

	Glib::RefPtr<Gio::Action> action = action_map_[action_type];
	if (!action) {
		debug_out_error("app", DBG_FUNC_MSG << "Action is NULL for action type " << static_cast<int>(action_type) << ".\n");
		return;
	}

	std::string action_name = action->get_name();

	// Do NOT output action->get_name() directly, it dies with unhandled conversion error
	// exception if used with operator <<.
	debug_out_info("app", DBG_FUNC_MSG << "Action activated: \"" + action_name << "\"\n");


	switch (action_type) {
		case action_quit:
			quit_requested();
			break;

		case action_view_details:
			if (iconview_)
				this->show_device_info_window(iconview_->get_selected_drive());
			break;

		case action_reread_device_data:
			on_action_reread_device_data();
			break;

		case action_perform_tests:
			if (iconview_) {
				auto win = this->show_device_info_window(iconview_->get_selected_drive());
				if (win)  // won't be created if test is already running
					win->show_tests();
			}
			break;

		case action_remove_device:
			if (iconview_) {
				StorageDevicePtr drive = iconview_->get_selected_drive();
				if (drive && drive->get_is_manually_added() && !drive->get_test_is_active())
					iconview_->remove_selected_drive();
			}
			break;

		case action_remove_virtual_device:
			if (iconview_) {
				StorageDevicePtr drive = iconview_->get_selected_drive();
				if (drive && drive->get_is_virtual())
					iconview_->remove_selected_drive();
			}
			break;

		case action_add_device:
			this->show_add_device_chooser();
			break;


		case action_load_virtual:
			this->show_load_virtual_file_chooser();
			break;

		case action_rescan_devices:
			rescan_devices();
			break;

		case action_executor_log:
		{
			// this one will only hide on close.
			auto win = GscExecutorLogWindow::create();  // probably already created
			// win->set_transient_for(*this);  // don't do this - it will make it always-on-top of this.
			win->show_last();  // show the window and select last entry
			break;
		}

		case action_update_drivedb:
		{
			run_update_drivedb();
			break;
		}

		case action_preferences:
		{
			auto win = GscPreferencesWindow::create();  // destroyed on close
			win->set_transient_for(*this);  // for "destroy with parent", always-on-top
			win->set_main_window(this);
			win->set_modal(true);
			win->show();
			break;
		}

		case action_online_documentation:
		{
			hz::launch_url(Gtk::Window::gobj(), "https://gsmartcontrol.sourceforge.io/documentation.html");
			break;
		}

		case action_support:
		{
			hz::launch_url(Gtk::Window::gobj(), "https://gsmartcontrol.sourceforge.io/support.html");
			break;
		}

		case action_about:
		{
			auto dialog = GscAboutDialog::create();  // destroyed on close
			dialog->set_transient_for(*this);  // for "destroy with parent"
			dialog->show();
			break;
		}

//		default:
//			debug_out_error("app", DBG_FUNC_MSG << "Unknown action: \"" << action_name << "\"\n");
//			break;
	}
}



void GscMainWindow::on_action_enable_smart_toggled()
{
	if (!iconview_)
		return;

	StorageDevicePtr drive = iconview_->get_selected_drive();

	// we should be protected from these by disabled actions, but still...
	if (!drive || drive->get_is_virtual() || drive->get_test_is_active())
		return;

	StorageDevice::Status status = drive->get_smart_status();
	if (status == StorageDevice::Status::unsupported)  // this shouldn't happen
		return;

	bool toggle_active = false;
	toggle_enable_smart_action_->get_state(toggle_active);

	if ( (toggle_active && status == StorageDevice::Status::disabled)
			|| (!toggle_active && status == StorageDevice::Status::enabled) ) {

		std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
		ex->create_running_dialog(this);

		std::string error_msg = drive->set_smart_enabled(toggle_active, ex);  // run it with GUI support

		if (!error_msg.empty()) {
			std::string error_header = (toggle_active ? _("Cannot enable SMART") : _("Cannot disable SMART"));
			gsc_executor_error_dialog_show(error_header, error_msg, this);
		}

		on_action_reread_device_data();  // reread if changed
	}
}



void GscMainWindow::on_action_enable_aodc_toggled()
{
	if (!iconview_)
		return;

	StorageDevicePtr drive = iconview_->get_selected_drive();

	// we should be protected from these by disabled actions, but still...
	if (!drive || drive->get_is_virtual() || drive->get_test_is_active())
		return;

	StorageDevice::Status status = drive->get_aodc_status();
	if (status == StorageDevice::Status::unsupported)  // this shouldn't happen
		return;

	if (status == StorageDevice::Status::unknown) {
		// it's supported, but we don't know if it's enabled or not. ask the user.

		int response = 0;
		{  // the dialog hides at the end of scope
			Gtk::MessageDialog dialog(*this, "\n"s + _("Automatic Offline Data Collection status could not be determined.\n"
					"\n<big>Do you want to enable or disable it?</big>") + "\n",
					true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);  // markup, modal

			Gtk::Button dismiss_button(_("Dis_miss"), true);
			// FIXME
			// dismiss_button.set_image(*Gtk::manage(new Gtk::Image("gtk-cancel", Gtk::ICON_SIZE_BUTTON)));
			dismiss_button.show_all();
			dialog.add_action_widget(dismiss_button, Gtk::RESPONSE_CANCEL);

			Gtk::Button disable_button(_("_Disable"), true);
			// FIXME
			// disable_button.set_image(*Gtk::manage(new Gtk::Image("gtk-no", Gtk::ICON_SIZE_BUTTON)));
			disable_button.show_all();
			dialog.add_action_widget(disable_button, Gtk::RESPONSE_NO);

			Gtk::Button enable_button(_("_Enable"), true);
			// FIXME
			// enable_button.set_image(*Gtk::manage(new Gtk::Image("gtk-yes", Gtk::ICON_SIZE_BUTTON)));
			enable_button.set_can_default(true);
			enable_button.show_all();
			dialog.add_action_widget(enable_button, Gtk::RESPONSE_YES);
			enable_button.grab_default();  // make it the default widget

			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

			response = dialog.run();  // blocks until the dialog is closed
		}

		bool enable_aodc = false;

		switch (response) {
			case Gtk::RESPONSE_YES:
				enable_aodc = true;
				break;
			case Gtk::RESPONSE_NO:
				enable_aodc = false;
				break;
			case Gtk::RESPONSE_CANCEL: case Gtk::RESPONSE_DELETE_EVENT:
				// nothing, the dialog is closed already
				return;
			default:
				debug_out_error("app", DBG_FUNC_MSG << "Unknown dialog response code: " << response << ".\n");
				return;
		}

		std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
		ex->create_running_dialog(this);

		std::string error_msg = drive->set_aodc_enabled(enable_aodc, ex);  // run it with GUI support

		if (!error_msg.empty()) {
			std::string error_header = (enable_aodc ? _("Cannot enable Automatic Offline Data Collection")
					: _("Cannot disable Automatic Offline Data Collection"));
			gsc_executor_error_dialog_show(error_header, error_msg, this);

		} else {  // tell the user, because there's no other feedback
			gui_show_info_dialog((enable_aodc ? _("Automatic Offline Data Collection enabled.")
					: _("Automatic Offline Data Collection disabled.")), this);
		}

		return;
	}


	bool toggle_active = false;
	toggle_enable_aodc_action_->get_state(toggle_active);

	if ( (toggle_active && status == StorageDevice::Status::disabled)
			|| (!toggle_active && status == StorageDevice::Status::enabled) ) {

		std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
		ex->create_running_dialog(this);

		std::string error_msg = drive->set_aodc_enabled(toggle_active, ex);  // run it with GUI support

		if (!error_msg.empty()) {
			std::string error_header = (toggle_active ? _("Cannot enable Automatic Offline Data Collection")
					: _("Cannot disable Automatic Offline Data Collection"));
			gsc_executor_error_dialog_show(error_header, error_msg, this);
		}

		on_action_reread_device_data();  // reread if changed
	}
}



void GscMainWindow::on_action_reread_device_data()
{
	if (!iconview_)
		return;

	StorageDevicePtr drive = iconview_->get_selected_drive();

	if (!drive->get_is_virtual() && !drive->get_test_is_active()) {  // disallow on virtual and testing
		std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
		ex->create_running_dialog(this);

		// note: this will clear the non-basic properties!
		std::string error_msg = drive->fetch_basic_data_and_parse(ex);  // run it with GUI support

		// the icon will be updated through drive's signal_changed callback.
		if (!error_msg.empty()) {
			gsc_executor_error_dialog_show(_("Cannot retrieve SMART data"), error_msg, this);
		}
	}
}



std::unique_ptr<Gtk::Menu> GscMainWindow::get_popup_menu(const StorageDevicePtr& drive)
{
	if (!ui_builder_)
		return nullptr;

	Gtk::Menu* menu = nullptr;
	if (drive) {
		ui_builder_->get_widget("/device_popup", menu);
	} else {
		ui_builder_->get_widget("/empty_area_popup", menu);
	}
	std::unique_ptr<Gtk::Menu> umenu(menu);
	return umenu;
}



void GscMainWindow::set_drive_menu_status(const StorageDevicePtr& drive)
{
	// disable any action handling until we're out of here, else we'll get some
	// bogus toggle actions, etc...
	this->action_handling_enabled_ = false;

	do {  // for quick skipping

		// if no drive is selected or if a test is being run on selected drive, disallow.
		if (!drive || drive->get_test_is_active()) {
			for (const auto& action_name : actiongroup_device_->list_actions()) {
				if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action(action_name))) {
					action->set_enabled(false);
				}
			}
			break;  // nothing else to do here
		}

		// make everything sensitive, then disable one by one
		for (const auto& action_name : actiongroup_device_->list_actions()) {
			if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action(action_name))) {
				action->set_enabled(true);
			}
		}

		bool is_virtual = (drive && drive->get_is_virtual());

		StorageDevice::Status smart_status = StorageDevice::Status::unsupported;
		StorageDevice::Status aodc_status = StorageDevice::Status::unsupported;

		if (drive && !is_virtual) {
			smart_status = drive->get_smart_status();
			aodc_status = drive->get_aodc_status();
		}


		// Sensitivity and visibility manipulation.
		// Do this first, then do the enable / disable stuff.
		{
			if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action("perform_tests"))) {
				action->set_enabled(smart_status == StorageDevice::Status::enabled);
			}
			// TODO
			// if ((action = actiongroup_device_->get_action(APP_ACTION_NAME(action_reread_device_data))))
			// 	action->set_visible(drive && !is_virtual);
			// if ((action = actiongroup_device_->get_action(APP_ACTION_NAME(action_remove_device)))) {
			// 	action->set_visible(drive && drive->get_is_manually_added());
			// if ((action = actiongroup_device_->get_action(APP_ACTION_NAME(action_remove_virtual_device))))
			// 	action->set_visible(drive && is_virtual);

			if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action("enable_smart"))) {
				action->set_enabled(smart_status != StorageDevice::Status::unsupported);
			}
			if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action("enable_aodc"))) {
				action->set_enabled(aodc_status != StorageDevice::Status::unsupported);
			}
		}


		// smart toggle status
		if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action("enable_smart"))) {
			action->change_state(smart_status == StorageDevice::Status::enabled);
		}


		// aodc toggle status
		if (auto action = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(actiongroup_device_->lookup_action("enable_smart"))) {
			action->change_state(smart_status == StorageDevice::Status::enabled);

			Gtk::CheckMenuItem* dev_odc_item = nullptr;
			ui_builder_->get_widget("/main_menubar/device_menu/" APP_ACTION_NAME(action_enable_aodc), dev_odc_item);

			Gtk::CheckMenuItem* popup_odc_item = nullptr;
			ui_builder_->get_widget("/device_popup/" APP_ACTION_NAME(action_enable_aodc), dev_odc_item);

			auto* status_aodc_check = lookup_widget<Gtk::CheckButton*>("status_aodc_enabled_check");

			// true if supported, but unknown whether it's enabled or not.
			if (dev_odc_item)
				dev_odc_item->set_inconsistent(aodc_status == StorageDevice::Status::unknown);
			if (popup_odc_item)
				popup_odc_item->set_inconsistent(aodc_status == StorageDevice::Status::unknown);
			if (status_aodc_check)
				status_aodc_check->set_inconsistent(aodc_status == StorageDevice::Status::unknown);

			// for unknown it doesn't really matter what state it's in.
			action->change_state(aodc_status == StorageDevice::Status::enabled);
		}

	} while (false);


	// re-enable action handling
	this->action_handling_enabled_ = true;
}



// update statusbar with selected drive info
void GscMainWindow::update_status_widgets()
{
	if (!iconview_)
		return;

// 	Gtk::Label* name_label = this->lookup_widget<Gtk::Label*>("status_name_label");
// 	Gtk::Label* health_label = this->lookup_widget<Gtk::Label*>("status_health_label");
// 	Gtk::Label* family_label = this->lookup_widget<Gtk::Label*>("status_family_label");
// 	Gtk::Statusbar* statusbar = this->lookup_widget<Gtk::Statusbar*>("window_statusbar");

	StorageDevicePtr drive = iconview_->get_selected_drive();
	if (!drive) {
		if (name_label_)
			name_label_->set_text(_("No drive selected"));
		if (health_label_)
			health_label_->set_text(_("No drive selected"));
		if (family_label_)
			family_label_->set_text(_("No drive selected"));
// 		if (statusbar)
// 			statusbar->pop();
		return;
	}

	/// Translators: %1 is filename
	std::string device = Glib::Markup::escape_text(drive->get_is_virtual()
			? Glib::ustring::compose(_("Virtual: %1"), drive->get_virtual_filename()) : Glib::ustring(drive->get_device_with_type()));
	std::string size = Glib::Markup::escape_text(drive->get_device_size_str());
	std::string model = Glib::Markup::escape_text(drive->get_model_name().empty()
			? std::string(_("Unknown model")) : drive->get_model_name());
	std::string family = Glib::Markup::escape_text(drive->get_family_name().empty()
			? C_("model_family", "Unknown") : drive->get_family_name());
	std::string family_fallback = Glib::Markup::escape_text(drive->get_family_name().empty() ? model : drive->get_family_name());
	std::string drive_letters_str = Glib::Markup::escape_text(drive->format_drive_letters(false));

	std::string info_str = device
			+ (drive_letters_str.empty() ? "" : (" (<b>" + drive_letters_str + "</b>)"))
			+ (size.empty() ? "" : (", " + size))
			+ (model.empty() ? "" : (", " + model));
	if (name_label_) {
		name_label_->set_markup(info_str);
		app_gtkmm_set_widget_tooltip(*name_label_, info_str, false);  // in case it doesn't fit
	}

	AtaStorageProperty health_prop = drive->get_health_property();

	if (health_label_) {
		if (health_prop.generic_name == "overall_health") {
			health_label_->set_text(health_prop.format_value());
			std::string fg;
			if (app_property_get_label_highlight_color(health_prop.warning, fg)) {
				health_label_->set_markup("<span color=\"" + fg + "\">"+ Glib::Markup::escape_text(health_label_->get_text()) + "</span>");
			}
			// don't set description tooltip - we already have the basic one.
			// unless it's failing.
			// app_gtkmm_set_widget_tooltip(*health_label, health_prop.get_description(), true);

			if (health_prop.warning != WarningLevel::none) {
				std::string tooltip_str = storage_property_get_warning_reason(health_prop)
						+ "\n\n" + _("View details for more information.");
				app_gtkmm_set_widget_tooltip(*health_label_, tooltip_str, true);
			}

		} else {
			health_label_->set_text(C_("health_status", "Unknown"));
		}
	}

	if (family_label_) {
		family_label_->set_text(family);
		app_gtkmm_set_widget_tooltip(*family_label_, family, false);  // in case it doesn't fit
	}

// 	std::string status_str = " " + device + (size.empty() ? "" : (", " + size)) + (family_fallback.empty() ? "" : (", " + family_fallback));
// 	if (statusbar) {
// 		statusbar->pop();
// 		statusbar->push(status_str);
// 	}
}



void GscMainWindow::rescan_devices()
{
	// ignore double-scan (may happen because we use gtk loop iterations here).
	if (this->scanning_)
		return;

	// don't manipulate window sensitiveness here - it breaks things
	// (cursors, gtk errors pop out, etc...)

	// if at least one drive is having a test performed, disallow.
	if (this->testing_active()) {
		int status = 0;
		{
			Gtk::MessageDialog dialog(*this,
					"\n"s + _("This operation may abort any running tests. Do you wish to continue?"),
					true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
			status = dialog.run();
		}
		if (status != Gtk::RESPONSE_YES) {
			return;
		}
	}

	this->scanning_ = true;

// 	std::string match_str = rconfig::get_data<std::string>("system/device_match_patterns");
	auto blacklist_str = rconfig::get_data<std::string>("system/device_blacklist_patterns");

// 	std::vector<std::string> match_patterns;
	std::vector<std::string> blacklist_patterns;
// 	hz::string_split(match_str, ';', match_patterns, true);
	hz::string_split(blacklist_str, ';', blacklist_patterns, true);

	iconview_->set_empty_view_message(GscMainWindowIconView::Message::scanning);

	iconview_->clear_all();  // clear previous icons, invalidate region to update the message.
	while (Gtk::Main::events_pending())  // give expose event the time it needs
		Gtk::Main::iteration();

	this->drives_.clear();

	// populate the icon area with drive icons
	StorageDetector sd;
// 	sd.add_match_patterns(match_patterns);
	sd.add_blacklist_patterns(blacklist_patterns);


	auto ex_factory = std::make_shared<CommandExecutorFactory>(true, this);  // run it with GUI support

	std::string error_msg = sd.detect_and_fetch_basic_data(drives_, ex_factory);

	bool error = false;

	// Catch permission errors.
	// executor errors and outputs, not reported through error_message.
	std::vector<std::string> fetch_outputs = sd.get_fetch_data_error_outputs();
	for (const auto& fetch_output : fetch_outputs) {
		// debug_out_error("app", DBG_FUNC_MSG << fetch_outputs[i] << "\n");
		if (app_pcre_match("/Smartctl open device.+Permission denied/mi", fetch_output)) {
			gsc_executor_error_dialog_show(_("An error occurred while scanning the system"),
					_("It seems that smartctl doesn't have enough permissions to access devices.\n"
					"<small>See \"Permission Problems\" section of the documentation, accessible through the Help menu.</small>"), this, true, true);
			error = true;
			break;
		}
	}

	if (!error && !error_msg.empty()) {  // generic scan error. smartctl errors are not reported during scan at all.
		// we don't show output button here
		gsc_executor_error_dialog_show(_("An error occurred while scanning the system"),
				error_msg, this, false, false);
		// error = true;

	// add them anyway, in case the error was only on one drive.
	} else { // if (!error) {
		// add them to iconview
		for (auto& drive : drives_) {
			if (rconfig::get_data<bool>("gui/show_smart_capable_only")) {
				if (drive->get_smart_status() != StorageDevice::Status::unsupported)
					iconview_->add_entry(drive);
			} else {
				iconview_->add_entry(drive);
			}
		}
	}

	// in case there are no drives in the system.
	if (iconview_->get_num_icons() == 0)
		iconview_->set_empty_view_message(GscMainWindowIconView::Message::no_drives_found);

	this->scanning_ = false;
}



void GscMainWindow::run_update_drivedb()
{
	auto smartctl_binary = get_smartctl_binary();

	if (smartctl_binary.empty()) {
		gui_show_error_dialog(_("Error Updating Drive Database"), _("Smartctl binary is not specified in configuration."), this);
		return;
	}

	hz::fs::path update_binary_path = hz::fs::u8path("update-smart-drivedb");
	if (smartctl_binary.is_absolute()) {
		update_binary_path = smartctl_binary.parent_path() / update_binary_path;
	}
	std::string update_binary = Glib::shell_quote(update_binary_path.u8string());

	if constexpr(!BuildEnv::is_kernel_family_windows()) {  // X11
		update_binary = "xterm -hold -e " + update_binary;
	}

	try {
		Glib::spawn_command_line_async(update_binary);
	}
	catch(Glib::Error& e) {
		gui_show_error_dialog(_("Error Updating Drive Database"), e.what(), this);
	}
}



bool GscMainWindow::add_device(const std::string& file, const std::string& type_arg, const std::string& extra_args)
{
	// win32 doesn't have device files, so skip the check in Windows.
	if constexpr(!BuildEnv::is_kernel_family_windows()) {
		std::error_code ec;
		if (!hz::fs::exists(hz::fs::u8path(file), ec)) {
			gui_show_error_dialog(_("Cannot add device"),
					(ec.message().empty() ? Glib::ustring::compose(_("Device \"%1\" doesn't exist."), file).raw() : ec.message()), this);
			return false;
		}
	}

	auto drive = std::make_shared<StorageDevice>(file);
	drive->set_type_argument(type_arg);
	drive->set_extra_arguments(extra_args);
	drive->set_is_manually_added(true);

	auto ex_factory = std::make_shared<CommandExecutorFactory>(true, this);  // pass this as dialog parent

	std::vector<StorageDevicePtr> tmp_drives;
	tmp_drives.push_back(drive);

	StorageDetector sd;
	std::string error_msg = sd.fetch_basic_data(tmp_drives, ex_factory, true);  // return its first error
	if (!error_msg.empty()) {
		gsc_executor_error_dialog_show(_("An error occurred while adding the device"), error_msg, this);

	} else {
		this->drives_.push_back(drive);
		this->iconview_->add_entry(drive, true);  // add it, scroll and select it.
	}

	return true;
}



bool GscMainWindow::add_virtual_drive(const std::string& file)
{
	std::string output;
	const int max_size = 10*1024*1024;  // 10M
	auto ec = hz::fs_file_get_contents(hz::fs::u8path(file), output, max_size);
	if (ec) {
		debug_out_warn("app", "Cannot open virtual drive file \"" << file << "\": " << ec.message() << "\n");
		gui_show_error_dialog(_("Cannot load data file"), ec.message(), this);
		return false;
	}

	// we have to use smart pointers here, because a pointer may be invalidated
	// on vector reallocation
	auto drive = std::make_shared<StorageDevice>(file, true);

	drive->set_full_output(output);
	drive->set_info_output(output);  // info can be parsed from full output string too.

	std::string error_msg = drive->parse_data();  // this will set the type and add the properties
	if (!error_msg.empty()) {
		gui_show_error_dialog(_("Cannot interpret SMART data"), error_msg, this);
		return false;
	}

	this->drives_.push_back(drive);

	this->iconview_->add_entry(drives_.back(), true);  // add it, scroll and select it.

	return true;
}




bool GscMainWindow::testing_active() const
{
	return std::any_of(drives_.cbegin(), drives_.cend(),
	[](const auto& drive)
	{
		return drive && drive->get_test_is_active();
	});
}



std::shared_ptr<GscInfoWindow> GscMainWindow::show_device_info_window(const StorageDevicePtr& drive)
{
	if (!drive) {
		return nullptr;
	}

	// if a test is being run on it, disallow.
	if (drive->get_test_is_active()) {
		gui_show_warn_dialog(_("Please wait until the test is finished on this drive."), this);
		return nullptr;
	}

	// ask to enable SMART if it's supported but disabled
	if (!drive->get_is_virtual() && (drive->get_smart_status() == StorageDevice::Status::disabled)) {

		int status = 0;

		// scope hides the dialog, without it two dialogs may be shown (this and)
		// the error one, and we don't want that.
		{
			Gtk::MessageDialog dialog(*this,
					"\n"s + _("This drive has SMART disabled. Do you want to enable it?") + "\n\n"
					+ "<small>" + _("SMART will stay enabled at least until you reboot your computer.") + "\n"
					+ _("See \"Enable SMART Permanently\" section of the documentation, accessible through the Help menu.") + "</small>",
					true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

			status = dialog.run();
		}

		if (status == Gtk::RESPONSE_YES) {
			std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
			ex->create_running_dialog(this, Glib::ustring::compose(_("Running {command} on %1..."), drive->get_device_with_type()));
			std::string error_msg = drive->set_smart_enabled(true, ex);  // run it with GUI support

			if (!error_msg.empty()) {
				gsc_executor_error_dialog_show(_("Cannot enable SMART"), error_msg, this);
			}
		}
	}


	// Virtual drives are parsed at load time.
	// Parse non-virtual, smart-supporting drives here.
	if (!drive->get_is_virtual() && drive->get_smart_status() != StorageDevice::Status::unsupported) {
		std::shared_ptr<SmartctlExecutorGui> ex(new SmartctlExecutorGui());
		ex->create_running_dialog(this, Glib::ustring::compose(_("Running {command} on %1..."), drive->get_device_with_type()));
		std::string error_msg = drive->fetch_data_and_parse(ex);  // run it with GUI support

		if (!error_msg.empty()) {
			gsc_executor_error_dialog_show(_("Cannot retrieve SMART data"), error_msg, this);
			return nullptr;
		}
	}


	// If the drive output wasn't fully parsed (happens with e.g. scsi and
	// usb devices), only very basic info is available and there's no point
	// in showing this window. - for both virtual and non-virtual.
	if (drive->get_parse_status() == StorageDevice::ParseStatus::none) {
		gsc_no_info_dialog_show(_("No additional information is available for this drive."),
				"", this, false, drive->get_info_output(), _("Smartctl Output"), drive->get_save_filename());
		return nullptr;
	}


	auto win = GscInfoWindow::create();  // self-destroyed

	win->set_drive(drive);
	win->fill_ui_with_info(false);  // already scanned. "refresh" will scan it again in the info window.

	// win->set_transient_for(*this);  // for "destroy with parent", always-on-top

	win->show();

	return win;
}



void GscMainWindow::show_prefs_updated_message()
{
	iconview_->set_empty_view_message(GscMainWindowIconView::Message::please_rescan);
	iconview_->clear_all();  // the message won't be shown without invalidating the region.
	while (Gtk::Main::events_pending())  // give expose event the time it needs
		Gtk::Main::iteration();
}




void GscMainWindow::show_add_device_chooser()
{
	auto window = GscAddDeviceWindow::create();
	window->set_main_window(this);
	window->set_transient_for(*this);
	window->show();
}



void GscMainWindow::show_load_virtual_file_chooser()
{
	static std::string last_dir;
	if (last_dir.empty()) {
		last_dir = rconfig::get_data<std::string>("gui/drive_data_open_save_dir");
	}
	int result = 0;

	Glib::RefPtr<Gtk::FileFilter> specific_filter = Gtk::FileFilter::create();
	specific_filter->set_name(_("Text Files"));
	specific_filter->add_pattern("*.txt");

	Glib::RefPtr<Gtk::FileFilter> all_filter = Gtk::FileFilter::create();
	all_filter->set_name(_("All Files"));
	all_filter->add_pattern("*");

#if GTK_CHECK_VERSION(3, 20, 0)
	std::unique_ptr<GtkFileChooserNative, decltype(&g_object_unref)> dialog(
			gtk_file_chooser_native_new(
				_("Load Data From..."), Gtk::Window::gobj(),
				GTK_FILE_CHOOSER_ACTION_OPEN, nullptr, nullptr),
			&g_object_unref);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), specific_filter->gobj());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog.get()), all_filter->gobj());

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog.get()), TRUE);

	if (!last_dir.empty()) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog.get()), last_dir.c_str());
	}

	result = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog.get()));

#else
	Gtk::FileChooserDialog dialog(*this, _("Load Data From..."),
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	// Add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

	dialog.add_filter(specific_filter);
	dialog.add_filter(all_filter);

	dialog.set_select_multiple(true);

	if (!last_dir.empty())
		dialog.set_current_folder(last_dir);

	// Show the dialog and wait for a user response
	result = dialog.run();  // the main cycle blocks here
#endif

	// Handle the response
	switch (result) {
		case Gtk::RESPONSE_ACCEPT:
		{
			std::vector<std::string> files;

#if GTK_CHECK_VERSION(3, 20, 0)
			std::unique_ptr<GSList, decltype(&g_slist_free)> file_slist(gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog.get())),
					&g_slist_free);
			GSList* iterator = file_slist.get();
			while(iterator) {
				files.push_back(app_ustring_from_gchar((gchar*)iterator->data));
				iterator = g_slist_next(iterator);
			}
#else
			files = dialog.get_filenames();  // in fs encoding
#endif
			if (!files.empty()) {
				last_dir = hz::fs::u8path(files.front()).parent_path().u8string();
			}
			rconfig::set_data("gui/drive_data_open_save_dir", last_dir);
			for (const auto& file : files) {
				std::error_code ec;
				if (!hz::fs::is_directory(hz::fs::u8path(file), ec)) {  // file chooser returns selected directories as well, ignore them.
					this->add_virtual_drive(file);
				}
			}
			break;
		}

		case Gtk::RESPONSE_CANCEL: case Gtk::RESPONSE_DELETE_EVENT:
			// nothing, the dialog is closed already
			break;

		default:
			debug_out_error("app", DBG_FUNC_MSG << "Unknown dialog response code: " << result << ".\n");
			break;
	}
}



void GscMainWindow::quit_requested()
{
	// if at least one drive is having a test performed, disallow.
	if (this->testing_active()) {
		if (!ask_about_quit_on_test(*this)) {
			return;
		}
	}

	// window size / pos
	{
		int window_w = 0, window_h = 0;
		get_size(window_w, window_h);
		rconfig::set_data("gui/main_window/default_size_w", window_w);
		rconfig::set_data("gui/main_window/default_size_h", window_h);

		int pos_x = 0, pos_y = 0;
		get_position(pos_x, pos_y);
		rconfig::set_data("gui/main_window/default_pos_x", pos_x);
		rconfig::set_data("gui/main_window/default_pos_y", pos_y);
	}

	app_quit();  // ends the main loop
}



// by default, delete_event calls hide().
bool GscMainWindow::on_delete_event([[maybe_unused]] GdkEventAny* e)
{
	quit_requested();
	return true;  // event handled
}






/// @}
