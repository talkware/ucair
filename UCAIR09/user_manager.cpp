#include "user_manager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr.hpp>
#include "common_util.h"
#include "config.h"
#include "logger.h"
#include "ucair_server.h"
#include "ucair_util.h"

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

namespace ucair {

bool UserManager::initialize(){
	Main& main = Main::instance();
	profiles_dir = util::getParam<string>(main.getConfig(), "profiles_dir");
	default_user_id = util::getParam<string>(main.getConfig(), "default_user");

	util::PrototypedFactory::registerPrototype(ClickResultEvent::type, shared_ptr<ClickResultEvent>(new ClickResultEvent));
	util::PrototypedFactory::registerPrototype(ViewSearchPageEvent::type, shared_ptr<ViewSearchPageEvent>(new ViewSearchPageEvent));
	return true;
}

bool UserManager::initializeUser(const string &user_id_){
	string user_id = user_id_;
	if (user_id.empty()) {
		user_id = default_user_id;
	}
	if (users.find(user_id) != users.end()){
		// Don't initialize twice.
		return true;
	}

	try {
		createUserFiles(user_id);
	}
	catch (const fs::filesystem_error &e) {
		getLogger().fatal(string("Failed to create user files: ") + e.what());
		return false;
	}

	fs::path path(getProfileDir(user_id));
	path = path / "pref.ini";
	ifstream fin(path.string().c_str());
	if (! fin){
		getLogger().fatal("User pref file not found: " + path.string());
		return false;
	}

	shared_ptr<User> user(new User(user_id));
	users.insert(make_pair(user_id, user));

	util::readParams(fin, user->getConfig());

	Main& main = Main::instance();
	list<Component*> initialized_components;
	BOOST_FOREACH(Component *component, main.getAllComponents()){
		if (! component->initializeUser(*user)){
			getLogger().fatal(util::getClassName(*component) + " failed to initialize user " + user_id);
			BOOST_REVERSE_FOREACH(Component *initialized_component, initialized_components) {
				initialized_component->finalizeUser(*user);
			}
			users.erase(user_id);
			return false;
		}
		initialized_components.push_front(component);
	}

	return true;
}

bool UserManager::finalizeUser(const string &user_id){
	User *user = getUser(user_id);
	if (! user) {
		return false;
	}

	Main& main = Main::instance();
	list<Component*> &components = main.getAllComponents();
	BOOST_REVERSE_FOREACH(Component *component, components){
		if (! component->finalizeUser(*user)){
			getLogger().fatal(util::getClassName(*component) + " failed to finalize user " + user_id);
			return false;
		}
	}

	users.erase(user_id);

	return true;
}

string UserManager::getProfileDir(const string &user_id) const {
	fs::path path(profiles_dir);
	path = path / user_id;
	return path.string();
}

User* UserManager::getUser(const string &user_id) const {
	map<string, shared_ptr<User> >::const_iterator itr = users.find(user_id);
	if (itr == users.end()){
		return NULL;
	}
	return itr->second.get();
}

vector<string> UserManager::getAllUserIds(bool logged_on_only) const {
	vector<string> v;
	if (logged_on_only) {
		for (map<string, shared_ptr<User> >::const_iterator itr = users.begin(); itr != users.end(); ++ itr){
			v.push_back(itr->first);
		}
	}
	else {
		fs::path path(profiles_dir);
		if (fs::exists(path)) {
			fs::directory_iterator end_itr;
			for (fs::directory_iterator itr(path); itr != end_itr; ++ itr) {
				if (fs::is_directory(itr->status())) {
					v.push_back(itr->leaf());
				}
			}
		}
	}

	sort(v.begin(), v.end());
	return v;
}

bool UserManager::existsUser(const string &user_id) const {
	fs::path path(getProfileDir(user_id));
	return fs::exists(path);
}

User* UserManager::getUserBySearchId(const string &search_id) const {
	map<string, string>::const_iterator itr = search2user.find(search_id);
	if (itr != search2user.end()){
		return getUser(itr->second);
	}
	return NULL;
}

User* UserManager::getUser(const Request &request, bool log_on_automatically) {
	string user_id = request.getCookie("user");
	User *user = getUser(user_id);
	if (! user) {
		// Automatically log on the user if possible.
		if (log_on_automatically && existsUser(user_id) && initializeUser(user_id)) {
			user = getUser(user_id);
		}
		if (! user) {
			// User needs to be created.
			getUCAIRServer().externalRedirect("/console");
		}
	}
	return user;
}

void UserManager::createUserFiles(const string &user_id) const {
	fs::path profile_path(getProfileDir(user_id));
	if (! fs::exists(profile_path)) {
		fs::create_directory(profile_path);
	}
	fs::path pref_path = profile_path / "pref.ini";
	if (! fs::exists(pref_path)) {
		fs::copy_file(fs::path("default_pref.ini"), pref_path);
	}
}

} // namespace ucair
