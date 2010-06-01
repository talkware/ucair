#include "aol_wrapper.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "http_download.h"
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
	// How dare AOL claim XHTML when it is not even valid HTML?
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

string AOLWrapper::buildURL(const SearchQuery &query, int start_pos, int result_count) {
	string encoded_query;
	http::util::urlEncode(query.text, encoded_query);
	return str(format("http://search.aol.com/aol/search?q=%1%&page=%2%&count_override=%3%") % encoded_query % ((start_pos - 1) / result_count + 1) % result_count);
}

bool AOLWrapper::parsePage(Search &search, const string &content) {
	static const regex re1("<li about=\"r.*?</li>");
	static const regex re2("<a [^>]*href=\"([^>]+)\" [^>]*property=\"f:title\"[^>]*>(.+?)</a>");
	static const regex re3("<p [^>]*property=\"f:desc\"[^>]*>(.*?)</p>");
	static const regex re4("<span [^>]*property=\"f:durl\"[^>]*>([^<]+?)</span>");
	static const regex re5("<span class=\"mimetype\">\\[.+?\\]</span>");
	static const regex re6("<span id=\"lowerLimit\">(\\d+)</span>");
	static const regex re7("<span id=\"upperLimit\">.*?</span>.*?<b>([\\d,]+)</b>");
	static const regex re8("Did you mean:.*?<a [^>]*>(.+?)</a>");

	smatch match1;
	int start_pos = 0;
	if (regex_search(content.begin(), content.end(), match1, re6)){
		try{
			string s = match1.str(1);
			start_pos = lexical_cast<int>(s);
		}
		catch (bad_lexical_cast &){
		}
	}

	if (regex_search(content.begin(), content.end(), match1, re7)){
		try{
			string s = erase_all_copy(match1.str(1), ",");
			search.setTotalResultCount(lexical_cast<long long>(s));
		}
		catch (bad_lexical_cast &){
		}
	}

	if (regex_search(content.begin(), content.end(), match1, re8)){
		search.query.spell_suggestion = match1.str(1);
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
		if (regex_search(match2[0].first, match2[0].second, match3, re5)){
			result.mime_type = match3.str(1);
		}
		search.results[result.original_rank] = result;
		++ start_pos;
	}
	return true;
}

bool AOLWrapper::fetchResults(Search &search, int start_pos, int result_count){
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
