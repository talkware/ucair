#include "search_engine.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "common_util.h"
#include "ucair_util.h"
#include "url_encoding.h"


using namespace std;
using namespace boost;

namespace ucair {

void SearchQuery::parseKeywords() {
	vector<string> tokens = util::tokenizeWithWhitespace(text, true);
	keywords.resize(tokens.size());
	for (int i = 0; i < (int) tokens.size(); ++ i) {
		string &token = tokens[i];
		Keyword &keyword = keywords[i];
		keyword.flag = Keyword::NONE;
		if (starts_with(token, "-")) {
			keyword.flag = Keyword::EXCLUDE;
			token = token.substr(1);
		}
		else if (starts_with(token, "+")) {
			keyword.flag = Keyword::INCLUDE;
			token = token.substr(1);
		}
		else if (starts_with(token, "site:")) {
			keyword.flag = Keyword::SITE;
			token = token.substr(5);
		}
		if (starts_with(token, "\"") && ends_with(token, "\"")) {
			token = token.substr(1, token.length() - 2);
			keyword.words = util::tokenizeWithWhitespace(token, false);
		}
		else {
			keyword.words.clear();
			keyword.words.push_back(token);
		}
	}
}

string SearchQuery::concatKeywords() const {
	string s;
	BOOST_FOREACH(const Keyword &keyword, keywords) {
		if (keyword.flag == Keyword::NONE || keyword.flag == Keyword::INCLUDE) {
			BOOST_FOREACH(const string &word, keyword.words) {
				if (! s.empty()) {
					s.push_back(' ');
				}
				s += word;
			}
		}
	}
	return s;
}

SearchResult::SearchResult(): original_rank(0) {}

SearchResult::SearchResult(const string &search_id_, int original_rank_):
search_id(search_id_),
original_rank(original_rank_) {
	doc_id = buildDocName(search_id, original_rank);
}

Search::Search(): total_result_count(0) {}

bool Search::hasResults(int start_pos, int result_count) const {
	if (start_pos < 1){
		return false;
	}
	map<int, SearchResult>::const_iterator itr = results.find(start_pos);
	for (int pos = start_pos; pos < start_pos + result_count; ++ pos, ++ itr){
		if (itr == results.end() || itr->first != pos){
			return false;
		}
	}
	return true;
}

const SearchResult* Search::getResult(int start_pos) const {
	map<int, SearchResult>::const_iterator itr = results.find(start_pos);
	if (itr == results.end()){
		return NULL;
	}
	return &itr->second;
}

////////////////////////////////////////////////////////////////////////////////

ExternalSearchEngine::ExternalSearchEngine(const string &id_, const string &name_, const string &search_link_queryless_, const string &search_link_query_prefix_) :
	id(id_),
	name(name_),
	search_link_queryless(search_link_queryless_),
	search_link_query_prefix(search_link_query_prefix_)
{
}

string ExternalSearchEngine::getSearchLink(const string &query) const {
	if (query.empty()) {
		return search_link_queryless;
	}
	string encoded_query;
	http::util::urlEncode(query, encoded_query);
	return search_link_query_prefix + encoded_query;
}

namespace external_search_engine{
	ExternalSearchEngine google("google", "Google", "http://www.google.com", "http://www.google.com/search?q=");
	ExternalSearchEngine bing("bing", "Bing", "http://www.bing.com", "http://www.bing.com/search?q=");
}

list<const ExternalSearchEngine*> getAllExternalSearchEngines() {
	list<const ExternalSearchEngine*> l;
	l.push_back(&external_search_engine::bing);
	l.push_back(&external_search_engine::google);
	return l;
}

} // namespace ucair
