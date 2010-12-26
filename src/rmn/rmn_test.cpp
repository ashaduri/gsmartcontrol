/**************************************************************************
 Copyright:
      (C) 2003 - 2008  Irakli Elizbarashvili <ielizbar 'at' gmail.com>
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: Public Domain
***************************************************************************/

#define INTRUSIVE_PTR_RUNTIME_CHECKS
#define RMN_RESOURCE_NODE_DEBUG

// disable libdebug, we don't link to it
#undef HZ_USE_LIBDEBUG
#define HZ_USE_LIBDEBUG 0
// enable libdebug emulation through std::cerr
#undef HZ_EMULATE_LIBDEBUG
#define HZ_EMULATE_LIBDEBUG 1

// The first header should be then one we're testing, to avoid missing
// header pitfalls.
#include "rmn.h"

#include <iostream>
#include <string>
#include <cstdlib>



// enable printing of std::string inside any_type.
// ANY_TYPE_SET_PRINTABLE(std::string, true);



int main()
{

	typedef rmn::resource_node< rmn::ResourceDataAny<rmn::ResourceSyncPolicyNone> > resource_node;
	typedef resource_node::node_ptr resource_node_ptr;


	resource_node_ptr root(new resource_node);
	root->set_name("/");
// 	std::cerr << root;

	{

		// create some nodes...
		resource_node_ptr app(new resource_node);
		app->set_name("app");
		root->add_child(app);

		resource_node_ptr sys(new resource_node);
		sys->set_name("sys");
		root->add_child(sys);

		resource_node_ptr plugins(new resource_node);
		plugins->set_name("plugins");
		app->add_child(plugins);


		plugins->create_child("plug1");
		plugins->create_child("plug2", std::string("plug2_data"));
		plugins->create_child("plug3", 6);

		plugins->build_nodes("video/gui");
		plugins->build_nodes("/app/video2/gui");  // should be denied
		plugins->build_nodes("/app/video3/gui", true);  // should be allowed, side-construction is enabled.

		plugins->create_child("plug4", "plug4_data");  // this will store it as std::string!

// 		std::cerr << "Data: " << plugins->get_child_node("plug3")->get_data<int>();

// 		std::cerr << root;
	}


	std::cerr << "--- begin dump root:\n";
	std::cerr << root;
	std::cerr << "--- end dump root\n";




	std::cerr << "--- build path test: -----------------\n";

	std::cerr << "root->build_nodes(\"/app/video/gui\"); - 2 times;" << "\n";
	root->build_nodes("/app/video/gui");
	root->build_nodes("/app/video/gui");


	std::cerr << "--- /app/fingor/gui, /app/fingor/state, /app/biocalc/gui, /sys/conf/video ... " << "\n";

	root->build_nodes("/app/fingor/gui");
	root->build_nodes("/app/fingor/state");
	root->build_nodes("/app/biocalc/gui");
	root->build_nodes("sys/conf/video");
	root->build_nodes("sys/conf/fingor");
	root->build_nodes("/sys/conf/biocalc");
	std::cerr << "--- root dump:" << "\n";
	std::cerr << root;

/*
	std::cerr << "--- get_path test: -------\n";

	std::cerr << "sys->get_path():\t" ;
	std::cerr << sys->get_path() << "\n";
	std::cerr << "plugins->get_path():\t" ;
	std::cerr << plugins->get_path() << "\n";
	std::cerr << "plug2->get_path():\t" ;
	std::cerr << plug2->get_path() << "\n";
*/

	std::cerr << "--- find_node test: -------\n";
	resource_node_ptr n;
	std::cerr << ((n = root->find_node("/sys/conf/fingor")) ? n->get_name() : "NULL") << "\n";
	std::cerr << ((n = root->find_node("/sys/conf/fingor/nonexistent")) ? n->get_name() : "NULL") << "\n";
	std::cerr << ((n = root->find_node("state")) ? n->get_name() : "NULL") << "\n";
	std::cerr << ((n = root->find_node("sys")) ? n->get_name() : "NULL") << "\n";
	std::cerr << ((n = root->find_node("/state")) ? n->get_name() : "NULL") << "\n";
	std::cerr << ((n = root->find_node("video/")) ? n->get_name() : "NULL") << "\n";

/*
	std::cerr << "--- root dump:" << "\n";
	std::cerr << root;
*/


	// this throws exception
// 	root->get_data<int>();  // empty
	root->find_node("/app/plugins/plug3")->get_data<std::string>();  // int -> string




	return EXIT_SUCCESS;
}




