#include "search_topics_ui.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <vector>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "common_util.h"
#include "index_manager.h"
#include "logger.h"
#include "search_topics.h"
#include "template_engine.h"
#include "ucair_util.h"
#include "url_encoding.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

string SearchTopicsMenuItem::getName(const Request &request) const {
	return "Search interests";
}

string SearchTopicsMenuItem::getLink(const Request &request) const {
	return "/search_topics";
}

bool SearchTopicsMenuItem::isActive(const Request &request) const {
	if (request.url_components.path == "/search_topics" || request.url_components.path == "/display_search_topic") {
		return true;
	}
	return false;
}

bool SearchTopicsUI::initialize(){
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/search_topics", bind(&SearchTopicsUI::listSearchTopics, this, _1, _2));
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/display_search_topic", bind(&SearchTopicsUI::displaySearchTopic, this, _1, _2));
	getSearchMenu().addMenuItem(shared_ptr<SearchTopicsMenuItem>(new SearchTopicsMenuItem));
	return true;
}

void SearchTopicsUI::listSearchTopics(Request &request, Reply &reply){
	User *user = getUserManager().getUser(request, true);
	assert(user);

	string sorting_criteria = request.getFormData("sort");

	string to_update = request.getFormData("update");
	if (! to_update.empty()) {
		getUserSearchTopicManager().updateSearchTopics(user->getUserId());
	}

	map<int, UserSearchTopic> *topics = getUserSearchTopicManager().getAllSearchTopics(user->getUserId());
	assert(topics);
	vector<pair<int, double> > sort_scores;
	sort_scores.reserve(topics->size());
	for (map<int, UserSearchTopic>::iterator itr = topics->begin(); itr != topics->end(); ++ itr) {
		if (itr->second.trivial) {
			continue;
		}
		sort_scores.push_back(make_pair(itr->first, itr->second.getSortingScore(sorting_criteria)));
	}
	sort(sort_scores.begin(), sort_scores.end(), util::cmp2ndReverse<int, double>);
	vector<UserSearchTopic*> sorted_topics(sort_scores.size());
	for (int i = 0; i < (int) sort_scores.size(); ++ i) {
		sorted_topics[i] = &(*topics)[sort_scores[i].first];
	}

	try {
		templating::TemplateData t_main;
		t_main.set("user_id", user->getUserId());
		getSearchMenu().render(t_main, request);

		BOOST_FOREACH(const string &criteria, UserSearchTopic::getAllSortingCriteria()) {
			t_main.addChild("sort")
				.set("criteria", criteria)
				.set("selected", sorting_criteria == criteria ? "true" : "false");
		}

		BOOST_FOREACH(const UserSearchTopic* topic, sorted_topics) {
			templating::TemplateData t_topic = t_main.addChild("topic");
			t_topic.set("topic_id", lexical_cast<string>(topic->topic_id))
				.set("session_count", lexical_cast<string>(topic->sessions.size()))
				.set("total_query_count", lexical_cast<string>(topic->searches.size()))
				.set("unique_query_count", lexical_cast<string>(topic->queries.size()))
				.set("total_click_count", lexical_cast<string>(topic->total_click_count))
				.set("unique_click_count", lexical_cast<string>(topic->clicks.size()));

			vector<pair<int, double> > truncated_model;
			truncate(*indexing::ValueMap::from(topic->model), truncated_model, 10, 0.01, 0.9);
			typedef pair<int, double> P;
			BOOST_FOREACH(const P &p, truncated_model){
				string term = getIndexManager().getTermDict().getName(p.first);
				t_topic.addChild("language_model_term_info")
					.set("term", term)
					.set("prob", str(format("%1$.3f") % p.second));
			}
		}

		string content = templating::getTemplateEngine().render(t_main, "search_topics.htm");
		reply.content = content;
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}
}

void SearchTopicsUI::displaySearchTopic(Request &request, Reply &reply){
	User *user = getUserManager().getUser(request, true);
	assert(user);

	int topic_id = -1;
	try {
		string s = request.getFormData("topic_id");
		topic_id = lexical_cast<int>(s);
	}
	catch (bad_lexical_cast &) {
	}
	if (topic_id <= 0) {
		getUCAIRServer().err(reply_status::bad_request, "Bad param topic_id");
	}

	try {
		templating::TemplateData t_main;
		t_main.set("user_id", user->getUserId());
		getSearchMenu().render(t_main, request);

		map<int, UserSearchTopic> *topics = getUserSearchTopicManager().getAllSearchTopics(user->getUserId());
		assert(topics);

		map<int, UserSearchTopic>::iterator itr = topics->find(topic_id);
		if (itr != topics->end()) {
			UserSearchTopic &topic = itr->second;

			t_main.set("topic_id", lexical_cast<string>(topic.topic_id))
				.set("session_count", lexical_cast<string>(topic.sessions.size()))
				.set("total_query_count", lexical_cast<string>(topic.searches.size()))
				.set("unique_query_count", lexical_cast<string>(topic.queries.size()))
				.set("total_click_count", lexical_cast<string>(topic.total_click_count))
				.set("unique_click_count", lexical_cast<string>(topic.clicks.size()));

			vector<pair<int, double> > truncated_model;
			truncate(*indexing::ValueMap::from(topic.model), truncated_model, 20, 0.001);
			typedef pair<int, double> P1;
			BOOST_FOREACH(const P1 &p, truncated_model){
				string term = getIndexManager().getTermDict().getName(p.first);
				t_main.addChild("language_model_term_info")
					.set("term", term)
					.set("prob", str(format("%1$.3f") % p.second));
			}
			if (truncated_model.size() < topic.model.size()) {
				t_main.addChild("language_model_term_info")
					.set("term", "...")
					.set("prob", "...");
			}

			typedef pair<string, int> P2;
			vector<pair<string, int> > sorted_queries;
			sorted_queries.reserve(topic.queries.size());
			copy(topic.queries.begin(), topic.queries.end(), back_inserter(sorted_queries));
			sort(sorted_queries.begin(), sorted_queries.end(), util::cmp2ndReverse<string, int>);
			BOOST_FOREACH(const P2 &p, sorted_queries) {
				const string &query = p.first;
				string encoded_query;
				http::util::urlEncode(query, encoded_query);
				int count = p.second;
				t_main.addChild("query")
					.set("query", query)
					.set("encoded_query", encoded_query)
					.set("count", lexical_cast<string>(count));
			}

			vector<pair<string, int> > sorted_clicks;
			sorted_queries.reserve(topic.clicks.size());
			copy(topic.clicks.begin(), topic.clicks.end(), back_inserter(sorted_clicks));
			sort(sorted_clicks.begin(), sorted_clicks.end(), util::cmp2ndReverse<string, int>);
			BOOST_FOREACH(const P2 &p, sorted_clicks) {
				const string &url = p.first;
				int count = p.second;
				t_main.addChild("click")
					.set("url", url)
					.set("count", lexical_cast<string>(count));
			}
		}


		string content = templating::getTemplateEngine().render(t_main, "display_search_topic.htm");
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
