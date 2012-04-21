#include "bing_wrapper.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include "http_download.h"
#include "logger.h"
#include "url_encoding.h"
#include "xml_util.h"

using namespace std;
using namespace boost;

namespace {

/*! \brief Takes HTML-formatted text and only keeps necessary tags.
 *
 *  This means changing <b> to <strong> and removing other tags.
 *  \param[in,out] a formatted text (after call, only <strong>)
 *  \param[out] b plain text without any tags
 */
void unformat(string &a, string &b){
	static regex re1("<b>(.*?)</b>");
	static regex re2("</?.*?>");
	a = regex_replace(a, re1, "UCAIR_BEGIN_STRONG$1UCAIR_END_STRONG");
	a = regex_replace(a, re2, "");
	replace_all(a, "UCAIR_BEGIN_STRONG", "<strong>");
	replace_all(a, "UCAIR_END_STRONG", "</strong>");

	b = erase_all_copy(a, "<strong>");
	erase_all(b, "</strong>");
	b = xml::util::unquote(b);
}

} // anonymous namespace

namespace ucair {

string BingWrapper::buildURL(const SearchQuery &query, int start_pos, int result_count) {
	string encoded_query;
	http::util::urlEncode(query.text, encoded_query);
	return str(format("http://www.bing.com/search?q=%1%&first=%2%") % encoded_query % start_pos);
}

bool BingWrapper::parsePage(Search &search, const string &content) {
	static const regex re1("<li class=\"sa_wr\">.*?</li>");
	static const regex re2("<h3><a .*?href=\"(.*?)\">(.*?)</a></h3>");
	static const regex re3("<p>(.*?)</p>");
	static const regex re4("<cite>(.*?)</cite>");
	static const regex re5("<span .*?id=\"count\">(\\d+).*? of ([\\d,]+) results</span>");
	static const regex re6("Including results for.*?<a .*?>(.*?)</a>");

	smatch match1;
	int start_pos = 0;
	if (regex_search(content.begin(), content.end(), match1, re5)){
		try{
			string s = match1.str(1);
			start_pos = lexical_cast<int>(s);
			s = erase_all_copy(match1.str(2), ",");
			search.setTotalResultCount(lexical_cast<long long>(s));
		}
		catch (bad_lexical_cast &){
		}
	}

	if (regex_search(content.begin(), content.end(), match1, re6)){
		string temp = match1.str(1);
		unformat(temp, search.query.spell_suggestion);
	}

	sregex_iterator itr(content.begin(), content.end(), re1);
	sregex_iterator end;
	for(; itr != end; ++ itr){
		const smatch& match2 = *itr;
		SearchResult result(search.getSearchId(), start_pos);
		smatch match3;
		if (regex_search(match2[0].first, match2[0].second, match3, re2)){
			string temp = match3.str(1);
			unformat(temp, result.url);
			result.click_url = result.url;
			result.formatted_title = match3.str(2);
			unformat(result.formatted_title, result.title);
		}
		if (result.url.empty() || result.title.empty()){
			continue;
		}
		if (regex_search(match2[0].first, match2[0].second, match3, re3)){
			result.formatted_summary = match3.str(1);
			unformat(result.formatted_summary, result.summary);
		}
		if (regex_search(match2[0].first, match2[0].second, match3, re4)){
			string temp = match3.str(1);
			unformat(temp, result.display_url);
		}
		search.results[result.original_rank] = result;
		++ start_pos;
	}
	return true;
}

bool BingWrapper::fetchResults(Search &search, int &start_pos, int &result_count){
	int end_pos = start_pos + result_count - 1;
	start_pos = start_pos / 10 * 10 + 1;
	result_count = end_pos - start_pos + 1;
	// result count can only be 10
	result_count = 10;
	// start pos can only be divisible by result count
	start_pos = start_pos / result_count * result_count + 1;

	string url = buildURL(search.query, start_pos, result_count);
	string content, error;
	if (! http::util::downloadPage(url, content, error)){
		return false;
	}
	if (! parsePage(search, content)){
		return false;
	}
	return true;
}

} // namespace ucair
