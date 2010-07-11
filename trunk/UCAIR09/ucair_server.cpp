#include "ucair_server.h"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "common_util.h"
#include "config.h"
#include "error.h"
#include "logger.h"

using namespace std;
using namespace boost;

namespace ucair {

static const int INTERNAL_REDIRECT_CODE = 1;
static const int EXTERNAL_REDIRECT_CODE = 2;

Request::Request(http::server::Request &req_):
	method(req_.method),
	url(req_.url),
	http_version_major(req_.http_version_major),
	http_version_minor(req_.http_version_minor),
	headers(req_.headers),
	content(req_.content),
	url_components(url),
	req(req_)
{
}

string Request::getHeader(const std::string &name) const {
	multimap<string, string>::const_iterator itr = headers.find(name);
	if (itr == headers.end()){
		return "";
	}
	return itr->second;
}

string Request::getFormData(const std::string &name) const {
	multimap<string, string>::const_iterator itr = form_data.find(name);
	if (itr == form_data.end()){
		return "";
	}
	return itr->second;
}

vector<string> Request::getFormDataMultiValued(const std::string &name) const {
	vector<string> result;
	typedef multimap<string, string>::const_iterator Itr;
	pair<Itr, Itr> range = form_data.equal_range(name);
	for (Itr itr = range.first; itr != range.second; ++ itr) {
		result.push_back(itr->second);
	}
	return result;
}

string Request::getCookie(const std::string &name) const {
	map<string, string>::const_iterator itr = cookie_data.find(name);
	if (itr != cookie_data.end()) {
		return itr->second;
	}
	return "";
}

Reply::Reply(http::server::Reply &rep_):
	status(rep_.status),
	headers(rep_.headers),
	content(rep_.content),
	doc_type(xml::util::NO_DOC_TYPE),
	rep(rep_)
{
	dom_root.setSelfDestroy();
}

void Reply::reset(){
	status = rep.status;
	headers = rep.headers;
	content.clear();
	dom_root.destroy();
	doc_type = xml::util::NO_DOC_TYPE;
	internal_redirect_path.clear();
	external_redirect_url.clear();
}

void Reply::setHeader(const string &name, const string &value, bool allow_dupliate) {
	if (allow_dupliate) {
		headers.insert(make_pair(name, value));
	}
	else {
		multimap<string, string>::iterator itr = headers.find(name);
		if (itr == headers.end()) {
			headers.insert(make_pair(name, value));
		}
		else {
			itr->second = value;
		}
	}
}

void Reply::setCookie(const string &name, const string &value) {
	cookie_data[name] = value;
}

UCAIRServer::UCAIRServer() :
	idle_signal(Main::instance().io_service) {
}

bool UCAIRServer::initialize(){
	Main &main = Main::instance();
	server_address = util::getParam<string>(main.getConfig(), "httpd_address");
	server_port = util::getParam<string>(main.getConfig(), "httpd_port");
	string doc_type = util::getParam<string>(main.getConfig(), "default_doc_type");
	if (doc_type == "html_4.01_loose"){
		default_doc_type = xml::util::HTML_4_01_LOOSE;
	}
	else if (doc_type == "html_4.01_strict"){
		default_doc_type = xml::util::HTML_4_01_STRICT;
	}
	else if (doc_type == "xhtml_1.0_transitional"){
		default_doc_type = xml::util::XHTML_1_0_TRANSITIONAL;
	}
	else if (doc_type == "xhtml_1.0_strict"){
		default_doc_type = xml::util::XHTML_1_0_STRICT;
	}
	else{
		default_doc_type = xml::util::NO_DOC_TYPE;
	}

	server.reset(new http::server::Server(Main::instance().io_service, server_address, server_port, bind(&UCAIRServer::dispatchRequest, this, _1, _2)));
	return true;
}

void UCAIRServer::stop(){
	getLogger().info("Stopping UCAIR server");
	server->stop(true);
}

void UCAIRServer::start() {
	getLogger().info("Starting UCAIR server");
	server->run();
}

void UCAIRServer::registerHandler(const RequestHandler::Classification &classification, const string &prefix, const RequestHandler::Callback &callback){
	handlers.push_back(RequestHandler());
	handlers.back().classification = classification;
	handlers.back().prefix = prefix;
	handlers.back().callback = callback;
}

void UCAIRServer::dispatchRequest(http::server::Request &req, http::server::Reply &rep){
	Request request(req);
	Reply reply(rep);

	getLogger().info(request.method + " " + request.url);

	// Only handle GET and POST
	if (request.method != "GET" && request.method != "POST"){
		reply.status = reply_status::not_implemented;
	}
	else{
		string path = request.url_components.path;

		// Limits the number of redirects to prevent infinite loop.
		for (int redirect_try = 0; redirect_try < 3; ++ redirect_try){
			RequestHandler *selected_handler = NULL;
			// Find the handler with longest prefix
			BOOST_FOREACH(RequestHandler &handler, handlers){
				if (starts_with(path, handler.prefix)){
					if (! selected_handler || handler.prefix.length() > selected_handler->prefix.length()){
						selected_handler = &handler;
					}
				}
			}
			if (! selected_handler){
				// No handler can handle this request path.
				reply.status = reply_status::not_found;
				break;
			}

			if (redirect_try == 0){
				// If CGI, parse form data.
				if (selected_handler->classification == RequestHandler::CGI_HTML || selected_handler->classification == RequestHandler::CGI_OTHER){
					reply.status = parseFormData(request);
					if (reply.status != reply_status::ok){
						break;
					}
				}
				parseCookieData(request);
			}

			try{
				reply.doc_type = default_doc_type;
				try {
					selected_handler->callback(request, reply);
				}
				catch (Error &e) {
					getLogger().error(e.what());
					if (const string* error_msg = boost::get_error_info<ErrorMsg>(e)){
						getLogger().error(*error_msg);
					}
					err(reply_status::internal_server_error, e.what());
				}
				catch (std::exception &e){
					getLogger().error(e.what());
					err(reply_status::internal_server_error, e.what());
				}
			}
			catch (RequestHandler::Interrupt &x){
				// If it is thrown by UCAIRServer::err (with HTTP status code and error message)
				if (const pair<int, string>* interrupt_info = get_error_info<RequestHandler::InterruptInfo>(x)){
					int status = interrupt_info->first;
					if (status == INTERNAL_REDIRECT_CODE) {
						reply.internal_redirect_path = interrupt_info->second;
					}
					else if (status == EXTERNAL_REDIRECT_CODE) {
						reply.external_redirect_url = interrupt_info->second;
					}
					else {
						const string &error_message = interrupt_info->second;
						reply.reset();
						reply.status = status;
						string status_str = reply_status::toString(status);
						static const char* html_format = "\
<html>\
<head>\
<title>%1%</title>\
</head>\
<body>\
<h1>%2% %1%</h1>\
<p>%3%</p>\
</body>\
</html>";
						reply.content = str(format(html_format) % xml::util::quote(status_str) % status % xml::util::quote(error_message));
					}
				}
			}

			if (! reply.internal_redirect_path.empty()){
				// internal redirect
				path = reply.internal_redirect_path;
				reply.reset(); // then goes to the next redirect_try
			}
			else if (! reply.external_redirect_url.empty()){
				// external redirect
				string temp = reply.external_redirect_url;
				reply.reset(); // this will clear reply.external_redirect_url
				reply.status = reply_status::moved_temporarily;
				reply.setHeader("Location", temp);
				break;
			}
			else{
				// Handler completes normally.
				if (selected_handler->classification == RequestHandler::CGI_HTML){
					makeHTML(reply);
				}
				break;
			}
		}
	}

	// Sets the date header.
	posix_time::ptime now = posix_time::microsec_clock::universal_time();
	reply.setHeader("Date", util::timeToString(now, util::internet_time_format));

	// Set cookies
	for (map<string, string>::const_iterator itr = reply.cookie_data.begin(); itr != reply.cookie_data.end(); ++ itr) {
		string name = itr->first;
		string value = itr->second;
		reply.setHeader("Set-Cookie", str(format("%s=%s; expires=Wed, 01-Jan-2020 00:00:00 GMT; path=/") % name % value), true);
	}

	// Inform watchers of handler completion.
	request_handled_signal(request, reply);
	idle_signal.waitFor(posix_time::seconds(1)); // TODO: use config value

	getLogger().info(str(format("%d %s") % reply.status % http::server::reply_status::toString(reply.status)));

	// Update wrapped http::server::reply
	rep.status = reply.status;
	swap(rep.headers, reply.headers); // save copying
	swap(rep.content, reply.content);
}

int UCAIRServer::parseFormData(Request &request){
	if (request.method == "GET"){
		http::util::URLComponents::parseQuery(request.url_components.query, request.form_data);
	}
	else if (request.method == "POST"){
		string content_type = request.getHeader("Content-Type");
		if (content_type.empty()){
			return reply_status::bad_request;
		}
		if (content_type == "multipart/form-data"){
			return reply_status::not_implemented;
		}
		http::util::URLComponents::parseQuery(request.content, request.form_data);
	}
	return reply_status::ok;

	//for (multimap<string, string>::const_iterator itr = request.form_data.begin(); itr != request.form_data.end(); ++ itr){
	//	cerr << itr->first << " = " << itr->second << endl;
	//}
	//cerr << endl;
}

void UCAIRServer::parseCookieData(Request &request) {
	string cookie_line = request.getHeader("Cookie");
	if (! cookie_line.empty()) {
		static const regex re("([^\\s=;,]+)=([^\\s=;,]+)");
		boost::sregex_iterator end;
		for (boost::sregex_iterator itr = boost::make_regex_iterator(cookie_line, re); itr != end; ++ itr){
			const boost::smatch &results = *itr;
			std::string name = results[1];
			string value = results[2];
			request.cookie_data[name] = value;
		}
	}
}

void UCAIRServer::makeHTML(Reply &reply){
	if (reply.status == reply_status::ok){
		reply.setHeader("Content-Type", "text/html");
		if (reply.dom_root){
			reply.content = xml::util::makeHTML(reply.dom_root, reply.doc_type);
		}
		else {
			reply.content = xml::util::makeHTML(reply.content, reply.doc_type);
		}
	}
}

void UCAIRServer::internalRedirect(const string &new_path){
	throw RequestHandler::Interrupt() << RequestHandler::InterruptInfo(make_pair(INTERNAL_REDIRECT_CODE, new_path));
}

void UCAIRServer::externalRedirect(const string &new_url){
	throw RequestHandler::Interrupt() << RequestHandler::InterruptInfo(make_pair(EXTERNAL_REDIRECT_CODE, new_url));;
}

void UCAIRServer::err(int status, const std::string &error_message){
	throw RequestHandler::Interrupt() << RequestHandler::InterruptInfo(make_pair(status, error_message));
}

} // namespace ucair
