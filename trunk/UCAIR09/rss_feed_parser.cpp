#include "rss_feed_parser.h"
#include <fstream>
#include <boost/foreach.hpp>
#include "common_util.h"
#include "http_download.h"
#include "ucair_util.h"
#include "xml_dom.h"

using namespace std;
using namespace boost;
using xml::dom::Node;

namespace ucair {

RSSFeedParser::RSSFeedParser(const string &doc_producer_id, const string &url_) : DocProducer(doc_producer_id), url(url_) {
}

bool RSSFeedParser::produceDoc(DocStream &doc_stream) {
	string source, err_msg;
	if (! http::util::downloadPage(url, source, err_msg)) {
		return false;
	}
	return parseRSSFeed(source, doc_stream);
}

bool RSSFeedParser::parseRSSFeed(const string &source, DocStream &doc_stream) {
	Node root(source);
	if (! root){
		return false;
	}
	root.setSelfDestroy();

	Node node_channel = root.findElement(root, "channel", "", "", Node::DESCEND);
	if (! node_channel){
		return false;
	}

	string channel_name;
	Node n = node_channel.findElement(node_channel, "title", "", "", Node::DESCEND_FIRST);
	if (n){
		channel_name = n.getInnerText();
	}

	Node node_item = node_channel.findElement(node_channel, "item", "", "", Node::DESCEND_FIRST);
	while (node_item){
		Document doc;

		n = node_item.findElement(node_item, "guid", "", "", Node::DESCEND_FIRST);
		if (n) {
			doc.doc_id = n.getInnerText();
		}
		if (! doc.doc_id.empty()) {
			doc.source = getId();

			n = node_item.findElement(node_item, "title", "", "", Node::DESCEND_FIRST);
			if (n) {
				doc.title = n.getInnerText();
			}

			n = node_item.findElement(node_item, "title", "", "", Node::DESCEND_FIRST);
			if (n) {
				doc.title = n.getInnerText();
			}

			n = node_item.findElement(node_item, "description", "", "", Node::DESCEND_FIRST);
			if (n) {
				string summary = n.getInnerText();
				if (isHTML(summary)) {
					doc.summary = stripHTML(summary);
					doc.formatted_summary = summary;
				}
				else {
					doc.summary = summary;
				}
			}

			n = node_item.findElement(node_item, "link", "", "", Node::DESCEND_FIRST);
			if (n) {
				doc.url = n.getInnerText();
			}

			n = node_item.findElement(node_item, "pubDate", "", "", Node::DESCEND_FIRST);
			if (n) {
				string date_str = n.getInnerText();
				posix_time::ptime t = util::stringToTime(date_str, util::internet_time_format);
				doc.date = util::to_time_t(t);
			}

			doc_stream.addDoc(doc);
		}

		node_item = node_channel.findElement(node_item, "item", "", "", Node::NO_DESCEND);
	}

	return true;
}

} // namespace ucair
