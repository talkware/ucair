#ifndef __rss_feed_parser_h__
#define __rss_feed_parser_h__

#include "doc_stream_manager.h"

namespace ucair {

/// Parses RSS feeds as a document source.
class RSSFeedParser : public DocProducer {
public:
	RSSFeedParser(const std::string &doc_producer_id, const std::string &url);

	bool produceDoc(DocStream &doc_stream);

private:
	bool parseRSSFeed(const std::string &source, DocStream &doc_stream);
	std::string url;
};

} // namespace ucair

#endif
