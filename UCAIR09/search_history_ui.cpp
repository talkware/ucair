#include "search_history_ui.h"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "basic_search_ui.h"
#include "common_util.h"
#include "error.h"
#include "index_manager.h"
#include "index_util.h"
#include "logger.h"
#include "template_engine.h"
#include "ucair_server.h"
#include "ucair_util.h"
#include "user_manager.h"
#include "url_encoding.h"

using namespace std;
using namespace boost;
using namespace templating;

namespace ucair {

string SearchHistoryMenuItem::getName(const Request &request) const {
	return "Search history";
}

string SearchHistoryMenuItem::getLink(const Request &request) const {
	const User *user = getUserManager().getUser(request, false);
	assert(user);

	string query = request.getFormData("query");
	if (query.empty()) {
		string search_id = user->getShortTermLastSearchId();
		if (! search_id.empty()) {
			query = user->getQuery(search_id);
		}
	}
	if (query.empty()) {
		return "/history_search";
	}
	string encoded_query;
	http::util::urlEncode(query, encoded_query);
	return "/history_search?query=" + encoded_query;
}

bool SearchHistoryMenuItem::isActive(const Request &request) const {
	if (request.url_components.path == "/history_search") {
		return true;
	}
	return false;
}

bool SearchHistoryUI::initialize(){
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/history_search", bind(&SearchHistoryUI::displaySearchHistory, this, _1, _2));
	getSearchMenu().addMenuItem(shared_ptr<SearchHistoryMenuItem>(new SearchHistoryMenuItem));
	return true;
}

void SearchHistoryUI::displaySearchHistory(Request &request, Reply &reply) {
	User *user = getUserManager().getUser(request, true);
	assert(user);

	string query = request.getFormData("query");
	string encoded_query;
	http::util::urlEncode(query, encoded_query);

	int start_pos = 1;
	try {
		string s = request.getFormData("start");
		if (! s.empty()){
			start_pos = lexical_cast<int>(s);
			if (start_pos < 0) {
				start_pos = 1;
			}
		}
	}
	catch (bad_lexical_cast &){
	}

	int search_count = 10;
	int total_search_count = 0;
	vector<string> search_ids_in_page;

	if (query.empty()) {
		const vector<string> &all_search_ids = user->getAllSearchIds();
		total_search_count = (int) all_search_ids.size();
		for (int i = total_search_count - start_pos; i > total_search_count - start_pos - search_count; -- i) {
			if (i < 0 || i >= total_search_count) {
				break;
			}
			search_ids_in_page.push_back(all_search_ids[i]);
		}
	}
	else {
		map<string, vector<string> >::iterator itr;
		tie(itr, tuples::ignore) = cached_searches.insert(make_pair(query, vector<string>()));
		vector<string> &search_ids = itr->second;
		if (start_pos == 1) {
			map<int, double> query_term_counts;
			indexing::countTerms(getIndexManager().getTermDict(), query, query_term_counts);
			search_ids.clear();
			typedef pair<string, double> P;
			vector<P> search_scores = user->searchInHistory(*indexing::ValueMap::from(query_term_counts));
			BOOST_FOREACH(const P &p, search_scores) {
				search_ids.push_back(p.first);
			}
		}
		// If start pos is not 1, we just use cached ranking.
		total_search_count = (int) search_ids.size();
		for (int i = start_pos - 1; i < start_pos - 1 + search_count && i < (int) search_ids.size(); ++ i) {
			search_ids_in_page.push_back(search_ids[i]);
		}
	}

	try {
		TemplateData t_main;
		t_main.set("user_id", user->getUserId())
			.set("start_pos", lexical_cast<string>(start_pos))
			.set("query", query)
			.set("encoded_query", encoded_query)
			.set("timestamp", lexical_cast<string>(time(NULL)));

		getSearchMenu().render(t_main, request);

		BasicSearchUI::renderSearchCounts(t_main, total_search_count, start_pos, search_count);
		BasicSearchUI::renderPrevNextPage(t_main, total_search_count, start_pos, search_count);

		string last_date_str;
		BOOST_FOREACH(const string &search_id, search_ids_in_page) {
			const Search* search = user->getSearch(search_id);
			const UserSearchRecord *search_record = user->getSearchRecord(search_id);
			if (! search || ! search_record) {
				continue;
			}

			posix_time::ptime search_time_utc = posix_time::from_time_t(search_record->getCreationTime());
			posix_time::ptime search_time_local = util::utc_to_local(search_time_utc);
			string date_str = getDateStr(search_time_local, true);
			string time_str = getTimeStr(search_time_local);

			string encoded_search_query;
			http::util::urlEncode(search->query.text, encoded_search_query);

			TemplateData t_search = t_main.addChild("search");
			t_search.set("search_id", search_id)
				.set("search_query", search->query.text)
				.set("encoded_search_query", encoded_search_query)
				.set("search_engine_id", search->getSearchEngineId())
				.set("time", time_str);

			if (! query.empty() || date_str != last_date_str) {
				t_search.set("date", date_str);
				last_date_str = date_str;
			}
			const util::UniqueList &clicks = search_record->getClickedResults();
			BOOST_FOREACH(int click_pos, clicks) {
				const SearchResult *result = search->getResult(click_pos);
				if (! result) {
					continue;
				}
				string display_url = result->display_url;
				if (display_url.empty()) {
					display_url = getDisplayUrl(result->url);
				}
				t_search.addChild("result")
					.set("title", result->title)
					.set("url", result->url)
					.set("display_url", display_url);
			}
		}

		string content = getTemplateEngine().render(t_main, "history_search.htm");
		reply.content = content;
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}
}

} // namespace ucair
