#include "basic_search_ui.h"
#include <ctime>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "common_util.h"
#include "config.h"
#include "error.h"
#include "logger.h"
#include "result_list_view.h"
#include "search_proxy.h"
#include "ucair_util.h"
#include "url_components.h"
#include "url_encoding.h"
#include "user_manager.h"

using namespace std;
using namespace boost;
using namespace templating;

namespace ucair {

string SearchViewMenuItem::getName(const ucair::Request &request) const {
	const PageModule *page_module = getPageModuleManager().getPageModule(search_view_id);
	return page_module->getModuleName();
}

string SearchViewMenuItem::getLink(const Request &request) const {
	const User *user= getUserManager().getUser(request, false);
	assert(user);
	string search_id = request.getFormData("sid");
	if (search_id.empty()) {
		search_id = user->getShortTermLastSearchId();
	}
	if (search_id.empty()) {
		return str(format("/search?view=%1%") % search_view_id);
	}
	else {
		return str(format("/search?sid=%1%&view=%2%") % search_id % search_view_id);
	}
}

bool SearchViewMenuItem::isActive(const ucair::Request &request) const {
	const User *user= getUserManager().getUser(request, false);
	assert(user);
	if (request.url_components.path == "/search") {
		string view_id = request.getFormData("view");
		if (view_id.empty()){
			view_id = user->getDefaultSearchView();
		}
		if (search_view_id == view_id) {
			return true;
		}
	}
	return false;
}

void BasicSearchUI::addSearchView(const std::string &view_id) {
	search_view_ids.push_back(view_id);
	getSearchMenu().addMenuItem(shared_ptr<SearchViewMenuItem>(new SearchViewMenuItem(view_id)));
}

bool BasicSearchUI::initialize(){
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "", bind(&BasicSearchUI::displaySearchPage, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/search", bind(&BasicSearchUI::displaySearchPage, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/click", bind(&BasicSearchUI::clickResult, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::CGI_OTHER, "/rate", bind(&BasicSearchUI::rateResult, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/external_search", bind(&BasicSearchUI::useExternalSearchEngine, this, _1, _2));
	shared_ptr<ResultListView> base_view(new ResultListView("base", "Base"));
	getPageModuleManager().addPageModule(base_view);
	addSearchView("base");

	Main &main = Main::instance();
	first_page_fetch_result_count = util::getParam<int>(main.getConfig(), "first_page_fetch_result_count");
	next_pages_fetch_result_count = util::getParam<int>(main.getConfig(), "next_pages_fetch_result_count");

	return true;
}

bool BasicSearchUI::initializeUser(User &user) {
	user.setDefaultSearchView(util::getParam<string>(user.getConfig(), "default_search_view"));
	return true;
}

void BasicSearchUI::useExternalSearchEngine(Request &request, Reply &reply){
	reply.doc_type = xml::util::NO_DOC_TYPE; // iframe is not in XHTML standard

	string query = request.getFormData("query");
	string encoded_query;
	http::util::urlEncode(query, encoded_query);

	string search_engine_id = request.getFormData("seng");
	try {
		TemplateData t_main;
		t_main.set("query", query)
			.set("encoded_query", encoded_query)
			.set("search_engine_id", search_engine_id);

		string search_engine_name;
		string search_page_link;
		list<const ExternalSearchEngine*> external_search_engines = getAllExternalSearchEngines();
		BOOST_FOREACH(const ExternalSearchEngine *external_search_engine, external_search_engines){
			if (search_engine_id == external_search_engine->getSearchEngineId()){
				search_engine_name = external_search_engine->getSearchEngineName();
				search_page_link = external_search_engine->getSearchLink(query);
			}
			t_main.addChild("external_search_engine")
				.set("external_search_engine_id", external_search_engine->getSearchEngineId())
				.set("external_search_engine_name", external_search_engine->getSearchEngineName())
				.set("active", search_engine_id == external_search_engine->getSearchEngineId() ? "active" : "inactive");
		}
		t_main.set("search_engine_name", search_engine_name)
			.set("external_search_page_link", search_page_link);

		reply.content = getTemplateEngine().render(t_main, "external_search.htm");
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}
}

void BasicSearchUI::displaySearchPage(Request &request, Reply &reply){
	User *user = getUserManager().getUser(request, true);
	assert(user);

	// Do not cache, so that when a user clicks on a result and then comes back,
	// the page will be refreshed rather than loaded from cache.
	reply.setHeader("Cache-Control", "no-store");

	string search_id = request.getFormData("sid");
	string query = request.getFormData("query");

	string search_engine_id = request.getFormData("seng");
	if (search_engine_id.empty()){
		search_engine_id = user->getDefaultSearchEngine();
	}
	if (! getSearchProxy().getSearchEngine(search_engine_id)) {
		//getUCAIRServer().err(reply_status::bad_request, "Invalid search engine");
		search_engine_id = "bing";
	}

	int start_pos = 0;
	try {
		string s = request.getFormData("start");
		if (! s.empty()){
			start_pos = lexical_cast<int>(s);
		}
	}
	catch (bad_lexical_cast &){
	}
	bool use_last_start_pos = false;
	if (start_pos < 1){
		use_last_start_pos = true;
		start_pos = 1;
	}
	else if (start_pos > 1000) {
		getUCAIRServer().err(reply_status::bad_request, "Start pos exceeds limit");
	}

	int result_count = 10;
	int result_fetch_count = start_pos == 1 ? first_page_fetch_result_count : next_pages_fetch_result_count;

	string search_view_id = request.getFormData("view");
	if (search_view_id.empty()){
		search_view_id = user->getDefaultSearchView();
	}
	PageModule *search_view = getPageModuleManager().getPageModule(search_view_id);
	if (! search_view){
		getUCAIRServer().err(reply_status::bad_request, "Invalid search view");
	}

	// Search id is needed to identify a search.
	// For a new search we only have query but no search id.
	// SearchProxy can fetch search results for the query, and return a search id.
	// We then reload the page using this search id.
	if (! query.empty() || ! search_id.empty()){
		string orig_search_id = search_id;

		if (user->isSearchExpired(search_id)){
			getUCAIRServer().err(reply_status::bad_request, "Search session expired");
		}

		SearchProxy::ReturnCode rc = getSearchProxy().search(search_id, query, search_engine_id, start_pos, result_fetch_count);
		if (rc == SearchProxy::BAD_PARAM){
			search_id.clear();
		}
		else if (rc == SearchProxy::BAD_CONNECTION){
			getUCAIRServer().err(reply_status::bad_gateway, "Failed to fetch results from " + search_engine_id);
		}

		if (orig_search_id != search_id){
			http::util::URLComponents url_components = request.url_components;
			url_components.eraseQueryParam("query");
			url_components.eraseQueryParam("seng");
			url_components.setQueryParam("sid", search_id);
			getUCAIRServer().externalRedirect(url_components.rebuildURL());
		}
	}

	string encoded_query;
	http::util::urlEncode(query, encoded_query);

	const Search *search = NULL;
	UserSearchRecord *search_record = NULL;
	if (! search_id.empty()){
		search = getSearchProxy().getSearch(search_id);
		if (! search){
			getUCAIRServer().err(reply_status::internal_server_error);
		}

		search_record = user->getSearchRecord(search_id);
		if (! search_record){
			search_record = user->addSearchRecord(search_id);
		}
		else{
			if (use_last_start_pos){
				start_pos = search_record->getLastStartPos(search_view_id);
			}
		}

		// Only index search results when the query is English.
		if (util::isASCIIPrintable(search->query.text)) {
			for (map<int, SearchResult>::const_iterator itr = search->results.begin(); itr != search->results.end(); ++ itr) {
				indexDocument(*search_record->getIndex(), itr->second);
			}
		}

		// Add the event of user viewing this page.
		shared_ptr<ViewSearchPageEvent> event(new ViewSearchPageEvent);
		event->search_id = search_id;
		event->start_pos = start_pos;
		event->result_count = result_count;
		event->view_id = search_view_id;
		user->addEvent(event);

		// Mark all search results in previous pages as viewed.
		ResultListView *result_list_view = dynamic_cast<ResultListView*>(search_view);
		if (result_list_view){
			const string property_name = "ranking_" + search_view_id;
			search_record->properties.set(property_name, vector<int>()); // Only executed when property does not exist yet.
			vector<int> &ranking = search_record->properties.get<vector<int> >(property_name);
			int count = 0;
			BOOST_FOREACH(const int &result_pos, ranking){
				if (++ count >= start_pos){
					break;
				}
				search_record->addViewedResult(result_pos);
			}
		}
	}

	try{
		TemplateData t_main;
		t_main.set("user_id", user->getUserId())
			.set("search_id", search_id)
			.set("start_pos", lexical_cast<string>(start_pos))
			.set("query", query)
			.set("encoded_query", encoded_query)
			.set("search_engine_id", search_engine_id)
			.set("view_id", search_view_id)
			.set("timestamp", lexical_cast<string>(time(NULL)));

		BOOST_FOREACH(const ExternalSearchEngine *external_search_engine, getAllExternalSearchEngines()){
			t_main.addChild("external_search_engine")
				.set("external_search_engine_id", external_search_engine->getSearchEngineId())
				.set("external_search_engine_name", external_search_engine->getSearchEngineName());
		}

		BOOST_FOREACH(const SearchEngine* search_engine, getSearchProxy().getAllSearchEngines()){
			t_main.addChild("internal_search_engine")
				.set("internal_search_engine_id", search_engine->getSearchEngineId())
				.set("internal_search_engine_name", search_engine->getSearchEngineName())
				.set("active", search_engine->getSearchEngineId() == search_engine_id ? "active" : "inactive");
		}

		getSearchMenu().render(t_main, request);

		if (! search_id.empty()){
			renderSearchCounts(t_main, search->getTotalResultCount(), start_pos, result_count);
			if (start_pos == 1) {
				const string &spell_suggestion = search->query.spell_suggestion;
				if (! spell_suggestion.empty()){
					string encoded_spell_suggestion;
					http::util::urlEncode(spell_suggestion, encoded_spell_suggestion);
					t_main.set("spell_suggestion_query", encoded_spell_suggestion)
						.set("spell_suggestion_text", spell_suggestion);
				}
			}
			renderPrevNextPage(t_main, search->getTotalResultCount(), start_pos, result_count);

			util::Properties params;
			params.set("search_id", search_id);
			params.set("start_pos", start_pos);
			params.set("result_count", result_count);
			search_view->recursiveRender(t_main, request, params);
		}

		string content = getTemplateEngine().render(t_main, "basic_search.htm");
		reply.content = content;
		// Uncomment below if you want to work on the DOM tree.
		/*reply.dom_root.fromString(content);
		if (! reply.dom_root){
			getUCAIRServer().err(reply_status::bad_request, "Failed to parse HTML into DOM");
		}*/
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}
}

void BasicSearchUI::clickResult(Request &request, Reply &reply){
	User *user = getUserManager().getUser(request, true);
	assert(user);

	int result_pos = 0;
	try{
		string s = request.getFormData("pos");
		if (! s.empty()){
			result_pos = lexical_cast<int>(s);
		}
	}
	catch (bad_lexical_cast &){
		getUCAIRServer().err(reply_status::bad_request);
	}

	string search_id = request.getFormData("sid");
	string view_id = request.getFormData("view");
	const SearchResult *result = getSearchProxy().getResult(search_id, result_pos);
	if (result){
		// Adds an event of user clicking on the result.
		shared_ptr<ClickResultEvent> event(new ClickResultEvent);
		event->search_id = search_id;
		event->result_pos = result_pos;
		event->url = result->url;
		user->addEvent(event);

		UserSearchRecord *search_record = user->getSearchRecord(search_id);
		if (search_record){
			search_record->addClickedResult(result_pos);
			// Mark all previous search results as viewed.
			const string property_name = "ranking_" + view_id;
			if (search_record->properties.has(property_name)){
				vector<int> &ranking = search_record->properties.get<vector<int> >(property_name);
				BOOST_FOREACH(const int &pos, ranking){
					search_record->addViewedResult(pos);
					if (pos == result_pos){
						break;
					}
				}
			}
		}

		getUCAIRServer().externalRedirect(result->click_url);
	}

	getUCAIRServer().err(reply_status::not_found);
}

void BasicSearchUI::rateResult(Request &request, Reply &reply){
	User *user = getUserManager().getUser(request, true);
	assert(user);

	int result_pos = 0;
	try{
		string s = request.getFormData("pos");
		if (! s.empty()){
			result_pos = lexical_cast<int>(s);
		}
	}
	catch (bad_lexical_cast &){
		getUCAIRServer().err(reply_status::bad_request);
	}
	string rating = request.getFormData("rating");

	string search_id = request.getFormData("sid");
	string view_id = request.getFormData("view");
	const SearchResult *result = getSearchProxy().getResult(search_id, result_pos);
	if (result){
		// Adds an event of user explicitly rating the result.
		shared_ptr<RateResultEvent> event(new RateResultEvent);
		event->search_id = search_id;
		event->result_pos = result_pos;
		event->rating = rating;
		user->addEvent(event);

		UserSearchRecord *search_record = user->getSearchRecord(search_id);
		if (search_record){
			search_record->setResultRating(result_pos, rating);
			// Mark all previous search results as viewed.
			const string property_name = "ranking_" + view_id;
			if (search_record->properties.has(property_name)){
				vector<int> &ranking = search_record->properties.get<vector<int> >(property_name);
				BOOST_FOREACH(const int &pos, ranking){
					search_record->addViewedResult(pos);
					if (pos == result_pos){
						break;
					}
				}
			}
		}
	}

	// Empty reply
	reply.setHeader("Content-Type", "text/plain");
}

void BasicSearchUI::renderSearchCounts(TemplateData &t_main, long long total_result_count, int start_pos, int result_count){
	int end_pos = start_pos + result_count - 1;
	if (end_pos > total_result_count){
		end_pos = (int) total_result_count;
	}
	t_main.set("start_pos", lexical_cast<string>(start_pos))
		.set("end_pos", lexical_cast<string>(end_pos))
		.set("total_count", lexical_cast<string>(total_result_count));
}

void BasicSearchUI::renderPrevNextPage(TemplateData &t_main, long long total_result_count, int start_pos, int result_count){
	t_main.set("show_prev_next_page", "true");
	if (start_pos > 1){
		int new_start_pos = start_pos - result_count;
		if (new_start_pos <= 0){
			new_start_pos = 1;
		}
		t_main.set("prev_page_start_pos", lexical_cast<string>(new_start_pos));
	}
	if (start_pos + result_count <= total_result_count){
		int new_start_pos = start_pos + result_count;
		t_main.set("next_page_start_pos", lexical_cast<string>(new_start_pos));
	}
}

} // namespace ucair
