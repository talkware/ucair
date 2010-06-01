#include "yahoo_boss_api.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "http_download.h"
#include "url_encoding.h"
#include "xml_dom.h"
#include "xml_util.h"

using namespace std;
using namespace boost;
using xml::dom::Node;

namespace{
	const char *APPID = "3SEGeKzV34H1yGktZ90N3YpLuefdwQhhtcBrakkkhWTFVaF3tM6V.7JJG1FOZsM-";
}

namespace {

/*! \brief Takes HTML-formatted text and only keeps necessary tags.
 *
 *  This means changing <b> to <strong> and removing other tags.
 *  \param[in,out] a formatted text (after call, only <strong>)
 *  \param[out] b plain text without any tags
 */
void unformat(string &a, string &b){
	erase_all(a, "<wbr>");
	replace_all(a, "<b>", "<strong>");
	replace_all(a, "</b>", "</strong>");

	b = erase_all_copy(a, "<strong>");
	erase_all(b, "</strong>");
	b = xml::util::unquote(b);
}

} // anonymous namespace

namespace ucair {

string YahooBossAPI::buildURL(const SearchQuery &query, int start_pos, int result_count) {
	string encoded_query;
	http::util::urlEncode(query.text, encoded_query);
	return str(format("http://boss.yahooapis.com/ysearch/web/v1/%2%?appid=%1%&start=%3%&count=%4%&format=xml") % APPID % encoded_query % (start_pos - 1) % result_count);
}

bool YahooBossAPI::parsePage(Search &search, const string &content) {
	string temp_str;

	Node root(content);
	if (! root){
		return false;
	}
	root.setSelfDestroy();

	Node node_result_set = root.findElement(root, "resultset_web", "", "", Node::DESCEND);
	if (! node_result_set){
		return false;
	}

	if (! node_result_set.getAttr("totalhits", temp_str)){
		return false;
	}
	try{
		search.setTotalResultCount(lexical_cast<long long>(temp_str));
	}
	catch (bad_lexical_cast &){
		return false;
	}

	int start_pos;
	if (! node_result_set.getAttr("start", temp_str)){
		return false;
	}
	try{
		start_pos = lexical_cast<int>(temp_str) + 1;
	}
	catch (bad_lexical_cast &){
		return false;
	}

	Node node_result = node_result_set.findElement(node_result_set, "result", "", "", Node::DESCEND_FIRST);
	while (node_result){
		SearchResult result(search.getSearchId(), start_pos ++);

		Node n = node_result.findElement(node_result, "title", "", "", Node::DESCEND_FIRST);
		if (n){
			result.formatted_title = n.getInnerText();
			unformat(result.formatted_title, result.title);
		}

		n = node_result.findElement(node_result, "abstract", "", "", Node::DESCEND_FIRST);
		if (n){
			result.formatted_summary = n.getInnerText();
			unformat(result.formatted_summary, result.summary);
		}

		n = node_result.findElement(node_result, "url", "", "", Node::DESCEND_FIRST);
		if (n){
			result.url = n.getInnerText();
		}

		n = node_result.findElement(node_result, "clickurl", "", "", Node::DESCEND_FIRST);
		if (n){
			result.click_url = n.getInnerText();
		}

		n = node_result.findElement(node_result, "dispurl", "", "", Node::DESCEND_FIRST);
		if (n){
			result.formatted_display_url = n.getInnerText();
			unformat(result.formatted_display_url, result.display_url);
		}

		search.results[result.original_rank] = result;
		node_result = node_result_set.findElement(node_result, "result", "", "", Node::NO_DESCEND);
	}
	return true;
}

bool YahooBossAPI::fetchResults(Search &search, int start_pos, int result_count){
	string url = buildURL(search.query, start_pos, result_count);
	string content, error;
	if (! http::util::downloadPage(url, content, error)){
		return false;
	}
	if (! parsePage(search, content)){
		return false;
	}
	if (start_pos == 1){
		checkSpelling(search.query.text, search.query.spell_suggestion);
	}
	return true;
}

bool YahooBossAPI::checkSpelling(const string &input, string &output){
	output.clear();

	string encoded_input;
	http::util::urlEncode(input, encoded_input);
	string url = str(format("http://boss.yahooapis.com/ysearch/spelling/v1/%1%?appid=%2%&format=xml") % encoded_input % APPID);
	string content, error;
	http::util::downloadPage(url, content, error);
	if (! error.empty()){
		return false;
	}

	Node root(content);
	if (! root){
		return false;
	}
	root.setSelfDestroy();

	Node n = root.findElement(root, "suggestion", "", "", Node::DESCEND);
	if (! n){
		return false;
	}
	output = n.getInnerText();

	return true;
}

} // namespace ucair
