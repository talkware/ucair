#include "search_proxy.h"
#include <cassert>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include "aol_wrapper.h"
#include "common_util.h"
#include "config.h"
#include "logger.h"
#include "search_engine_repeater.h"
#include "ucair_util.h"
#include "user.h"
#include "yahoo_boss_api.h"

using namespace std;
using namespace boost;

namespace ucair {

SearchEngine* SearchProxy::getSearchEngine(const std::string &search_engine_id) const {
	BOOST_FOREACH(const shared_ptr<SearchEngine> &search_engine, search_engines){
		if (search_engine->getSearchEngineId() == search_engine_id){
			return search_engine.get();
		}
	}
	return NULL;
}

list<SearchEngine*> SearchProxy::getAllSearchEngines() const {
	list<SearchEngine*> l;
	BOOST_FOREACH(const shared_ptr<SearchEngine> &search_engine, search_engines){
		l.push_back(search_engine.get());
	}
	return l;
}

bool SearchProxy::initialize(){
	shared_ptr<SearchEngine> yahoo(new YahooBossAPI);
	shared_ptr<SearchEngine> aol(new AOLWrapper);
	shared_ptr<SearchEngine> aolr(new SearchEngineRepeater(aol));
	search_engines.push_back(yahoo);
	search_engines.push_back(aol);
	search_engines.push_back(aolr);
	// Add more search engines here.
	return true;
}

bool SearchProxy::finalize(){
	searches.clear();
	search_engines.clear();
	return true;
}

bool SearchProxy::initializeUser(User &user) {
	user.setDefaultSearchEngine(util::getParam<string>(user.getConfig(), "default_search_engine"));
	return true;
}

SearchProxy::ReturnCode SearchProxy::search(string &search_id, string &query_text, string &search_engine_id, int start_pos, int result_count){
	if (searches.find(search_id) == searches.end()){
		// new search
		if (query_text.empty() || search_engine_id.empty()){
			getLogger().error("Query and search engine must be both specified");
			return BAD_PARAM;
		}

		SearchEngine* search_engine = getSearchEngine(search_engine_id);
		if (! search_engine){
			getLogger().error("Search engine not found: " + search_engine_id);
			return BAD_PARAM;
		}

		int allowed_result_count = search_engine->maxAllowedResultCount();
		if (result_count > allowed_result_count){
			result_count = allowed_result_count;
		}

		Search search;
		search_id = util::makeId();
		search.setSearchId(search_id);
		search.query.text = query_text;
		search.query.parseKeywords();
		search.setSearchEngineId(search_engine_id);
		if (! search_engine->fetchResults(search, start_pos, result_count)){
			getLogger().error("Failed to fetch results for query ( " + query_text + " ) from " + search_engine_id);
			return BAD_CONNECTION;
		}
		searches[search_id] = search;
	}
	else{
		// existing search
		Search& search = searches[search_id];
		query_text = search.query.text;
		search_engine_id = search.getSearchEngineId();

		SearchEngine* search_engine = getSearchEngine(search_engine_id);

		int allowed_result_count = search_engine->maxAllowedResultCount();
		if (result_count > allowed_result_count){
			result_count = allowed_result_count;
		}

		if (search.hasResults(start_pos, result_count)){
			getLogger().debug(str(format("Already has results from %d to %d") % start_pos % (start_pos + result_count - 1)));
			return OK;
		}
		start_pos = search.results.size() + 1;

		// Use a temporary search instance.
		Search temp_search;
		temp_search.setSearchId(search_id);
		temp_search.query = search.query;
		if (! search_engine->fetchResults(temp_search, start_pos, result_count)){
			getLogger().error("Failed to fetch results for query ( " + search.query.text + " ) from " + search_engine_id);
			return BAD_CONNECTION;
		}
		// Merge the temporary search instance.
		search.setTotalResultCount(temp_search.getTotalResultCount());
		for (map<int, SearchResult>::const_iterator itr = temp_search.results.begin(); itr != temp_search.results.end(); ++ itr){
			search.results[itr->first] = itr->second;
		}
	}
	return OK;
}

const Search* SearchProxy::getSearch(const string &search_id) const {
	map<string, Search>::const_iterator itr = searches.find(search_id);
	if (itr == searches.end()){
		return NULL;
	}
	return &itr->second;
}

const SearchResult* SearchProxy::getResult(const string &search_id, int result_pos) const {
	const Search* search = getSearch(search_id);
	if (! search){
		return NULL;
	}
	map<int, SearchResult>::const_iterator itr = search->results.find(result_pos);
	if (itr == search->results.end()){
		return NULL;
	}
	return &itr->second;
}

} // namespace ucair
