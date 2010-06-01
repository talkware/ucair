#include "main.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include "common_util.h"
#include "component.h"
#include "config.h"
#include "log_importer.h"
#include "logger.h"
#include "sqlitepp.h"
#include "test_main.h"
#include "ucair_server.h"
#include "user_manager.h"

using namespace std;
using namespace boost;
namespace po = boost::program_options;

namespace ucair {

extern void addAllComponents();

Main* Main::_instance = NULL;

Main::Main(int _argc, char *_argv[]): argc(_argc), argv(_argv), started(false) {
	_instance = this;
	sqlite::initialize();
}

Main::~Main(){
	BOOST_REVERSE_FOREACH(Component *component, components){
		delete component;
	}
	sqlite::shutdown();
}

void Main::start(){
	addAllComponents();

	if (! getOptions(argc, argv)){
		exit(1);
	}

	list<Component*> initialized_components;
	BOOST_FOREACH(Component *component, components){
		if (! component->initialize()){
			getLogger().fatal(util::getClassName(*component) + " failed to initialize");
			BOOST_REVERSE_FOREACH(Component *initialized_component, initialized_components) {
				initialized_component->finalize();
			}
			exit(1);
		}
		initialized_components.push_front(component);
	}

	// Initialize default user.
	if (! getUserManager().initializeUser()) {
		exit(1);
	}

	started = true;
	if (start_mode == "ucair_server"){
		getUCAIRServer().start();
	}
	else {
		if (start_mode == "test") {
			testMain();
		}
		else if (start_mode == "log_importer") {
			getComponent<LogImporter>().run();
		}
		stop();
	}
}

void Main::interrupt(){
	if (start_mode == "ucair_server") {
		io_service.post(bind(&Main::stop, this));
	}
}

void Main::stop() {
	if (! started) {
		return;
	}

	if (start_mode == "ucair_server") {
		getUCAIRServer().stop();
	}

	vector<string> all_user_ids = getUserManager().getAllUserIds();
	BOOST_FOREACH(const string &user_id, all_user_ids) {
		getUserManager().finalizeUser(user_id);
	}

	BOOST_REVERSE_FOREACH(Component *component, components){
		if (! component->finalize()){
			getLogger().fatal(util::getClassName(*component) + " failed to finalize");
		}
	}

	started = false;
}

bool Main::getOptions(int argc, char *argv[]){
	try{
		po::options_description all_options;
		all_options.add_options()
			("help", "print help message")
			("config_file,f", po::value<string>(), "config file to read");

		po::variables_map vm;
		po::store(parse_command_line(argc, argv, all_options), vm);

		if (vm.count("help")){
			cout << all_options << endl;
			return false;
		}

		string file_name = "config.ini";
		if (vm.count("config_file")){
			file_name = vm["config_file"].as<string>();
		}
		ifstream fin(file_name.c_str());
		if (! fin){
			getLogger().fatal("Failed to open config file: " + file_name);
			return false;
		}
		util::readParams(fin, config);
	}
	catch (po::error &e){
		getLogger().fatal(e.what());
		return false;
	}

	try {
		start_mode = util::getParam<string>(config, "start_mode");
	}
	catch (runtime_error &e) {
		getLogger().fatal(e.what());
		return false;
	}

	return true;
}

} // namespace ucair
