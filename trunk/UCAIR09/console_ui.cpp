#include "console_ui.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include "logger.h"
#include "template_engine.h"
#include "user_manager.h"
#include "ucair_server.h"

using namespace std;
using namespace boost;

namespace ucair {

string ConsoleMenuItem::getName(const Request &request) const {
	return "Console";
}

string ConsoleMenuItem::getLink(const Request &request) const {
	return "/console";
}

bool ConsoleMenuItem::isActive(const Request &request) const {
	if (request.url_components.path == "/console") {
		return true;
	}
	return false;
}

bool ConsoleUI::initialize(){
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/console", bind(&ConsoleUI::displayConsole, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/login", bind(&ConsoleUI::userLogIn, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::STATIC, "/open_search", bind(&ConsoleUI::downloadOpenSearch, this, _1, _2));
	getSearchMenu().addMenuItem(shared_ptr<ConsoleMenuItem>(new ConsoleMenuItem));
	return true;
}

void ConsoleUI::displayConsole(Request &request, Reply &reply){
	string active_user_id;
	try {
		User *user = getUserManager().getUser(request, false);
		if (user) {
			active_user_id = user->getUserId();
		}
	}
	catch (RequestHandler::Interrupt &){
		// do not redirect
	}

	bool shutdown_requested = ! request.getFormData("shutdown").empty();

	try {
		templating::TemplateData t_main;
		t_main.set("user_id", active_user_id);
		if (! active_user_id.empty()) {
			getSearchMenu().render(t_main, request);
		}

		t_main.set("shutdown", shutdown_requested ? "true" : "false");

		vector<string> user_ids = getUserManager().getAllUserIds(false);
		BOOST_FOREACH(const string &user_id, user_ids) {
			t_main.addChild("user")
					.set("user_id", user_id)
					.set("active", user_id == active_user_id ? "true" : "false");
		}

		t_main.set("address", getUCAIRServer().getAddress() + ":" + getUCAIRServer().getPort());

		string content = templating::getTemplateEngine().render(t_main, "console.htm");
		reply.content = content;
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}

	if (shutdown_requested) {
		Main::instance().interrupt();
	}
}

void ConsoleUI::userLogIn(Request &request, Reply &reply){
	string user_id = request.getFormData("user");
	if (! user_id.empty()) {
		if (getUserManager().initializeUser(user_id)) {
			reply.setCookie("user", user_id);
			getUCAIRServer().externalRedirect("/search");
		}
	}
	getUCAIRServer().externalRedirect("/console");
}

void ConsoleUI::downloadOpenSearch(Request &request, Reply &reply) {
	try {
		templating::TemplateData t_main;
		t_main.set("address", getUCAIRServer().getAddress() + ":" + getUCAIRServer().getPort());
		reply.content = templating::getTemplateEngine().render(t_main, "ucair_open_search.xml");
		reply.setHeader("Content-Type", "application/opensearchdescription+xml");
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}
}

} // namespace ucair
