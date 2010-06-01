#include "yahoo_search_api.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "xml_dom.h"
#include "xml_util.h"

using namespace std;
using namespace boost;
using xml::dom::Node;

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

bool YahooSearchAPI::parsePage(Search &search, const string &content) {
	string temp_str;

	Node root(content);
	if (! root){
		return false;
	}
	root.setSelfDestroy();

	Node node_result_set = root.findElement(root, "ResultSet", "", "", Node::DESCEND);
	if (! node_result_set){
		return false;
	}

	if (! node_result_set.getAttr("totalResultsReturned", temp_str)){
		return false;
	}
	try{
		search.setTotalResultCount(lexical_cast<long long>(temp_str));
	}
	catch (bad_lexical_cast &){
		return false;
	}

	int start_pos;
	if (! node_result_set.getAttr("firstResultPosition", temp_str)){
		return false;
	}
	try{
		start_pos = lexical_cast<int>(temp_str);
	}
	catch (bad_lexical_cast &){
		return false;
	}

	Node node_result = node_result_set.findElement(node_result_set, "Result", "", "", Node::DESCEND_FIRST);
	while (node_result){
		SearchResult result(search.getSearchId(), start_pos);

		Node n = node_result.findElement(node_result, "Title", "", "", Node::DESCEND_FIRST);
		if (n){
			result.formatted_title = n.getInnerText();
			unformat(result.formatted_title, result.title);
		}

		n = node_result.findElement(node_result, "Summary", "", "", Node::DESCEND_FIRST);
		if (n){
			result.formatted_summary = n.getInnerText();
			unformat(result.formatted_summary, result.summary);
		}

		n = node_result.findElement(node_result, "Url", "", "", Node::DESCEND_FIRST);
		if (n){
			result.url = n.getInnerText();
		}

		n = node_result.findElement(node_result, "ClickUrl", "", "", Node::DESCEND_FIRST);
		if (n){
			result.click_url = n.getInnerText();
		}

		search.results[result.original_rank] = result;
		++ start_pos;
		node_result = node_result_set.findElement(node_result, "Result", "", "", Node::NO_DESCEND);
	}
	return true;
}

}
