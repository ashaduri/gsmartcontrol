/**************************************************************************
 Copyright:
      (C) 2008 - 2011  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_unlicense.txt
***************************************************************************/

#include <iostream>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <gtkmm/main.h>

#include "app_ui_res_utils.h"
// #include "hz/instance_manager.h"



class AppUiResTestWindow;

// class AppUiResTestWindow : public Gtk::Window
// class AppUiResTestWindow : public hz::InstanceManager<AppUiResTestWindow, false>, public Gtk::Window
class AppUiResTestWindow : public AppUIResWidget<AppUiResTestWindow, false>
{
	public:

		// name of glade/ui file without a .glade/.ui extension and quotes
		APP_UI_RES_DATA_INIT(app_ui_res_test_window);


		enum action_t {
			action_quit
		};

// 		AppUiResTestWindow()
		AppUiResTestWindow(BaseObjectType* gtkcobj, const app_ui_res_ref_t& ref_ui)
				: AppUIResWidget<AppUiResTestWindow, false>(gtkcobj, ref_ui)
		{
			Gtk::Box* vbox = Gtk::manage(new Gtk::VBox(false, 5));
			add(*vbox);

			Gtk::Button* button = Gtk::manage(new Gtk::Button("Clicky"));
			vbox->pack_start(*button, Gtk::PACK_SHRINK);
// 			button->signal_clicked().connect(sigc::mem_fun(*this, &AppUiResTestWindow::on_button_clicked));
			button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AppUiResTestWindow::on_button_clicked2), action_quit));


			Gtk::Label* label = Gtk::manage(new Gtk::Label("test"));
			vbox->pack_start(*label, Gtk::PACK_SHRINK);

			this->signal_delete_event().connect(sigc::mem_fun(*this, &AppUiResTestWindow::on_delete_event));

			show_all();
		}

		virtual ~AppUiResTestWindow()
		{ }


	private:

		void on_button_clicked()
		{
			std::cerr << "AppUiResTestWindow::on_button_clicked()\n";
		}

		void on_button_clicked2(action_t action_type)
		{
			std::cerr << "AppUiResTestWindow::on_button_clicked2()\n";
		}

		bool on_delete_event(GdkEventAny* e)
		{
			Gtk::Main::quit();
			return true;  // action handled
		}

};



int main(int argc, char *argv[])
{
    Gtk::Main m(&argc, &argv);

//     AppUiResTestWindow* app = new AppUiResTestWindow();
	AppUiResTestWindow::create();

    m.run();

// 	delete app;
	AppUiResTestWindow::destroy();

    return(0);
}



