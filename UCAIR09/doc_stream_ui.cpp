#include "doc_stream_ui.h"
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "error.h"
#include "logger.h"
#include "rss_feed_parser.h"
#include "ucair_server.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;
using namespace templating;

namespace ucair {

string DocStreamMenuItem::getName(const Request &request) const {
	return "Streams";
}

string DocStreamMenuItem::getLink(const Request &request) const {
	const User *user = getUserManager().getUser(request, false);
	assert(user);
	string stream_id = request.getFormData("stream_id");
	if (stream_id.empty()) {
		stream_id = user->getLastViewedDocStreamId();
	}
	if (stream_id.empty()) {
		return "/stream";
	}
	string encoded_stream_id;
	http::util::urlEncode(stream_id, encoded_stream_id);
	return "/stream?stream_id=" + encoded_stream_id;
}

bool DocStreamMenuItem::isActive(const Request &request) const {
	if (request.url_components.path == "/stream") {
		return true;
	}
	return false;
}

bool DocStreamUI::initialize(){
	getUCAIRServer().registerHandler(RequestHandler::CGI_HTML, "/stream", bind(&DocStreamUI::displayDocStream, this, _1, _2));
	getSearchMenu().addMenuItem(shared_ptr<DocStreamMenuItem>(new DocStreamMenuItem));

	//TODO: allow user to edit sources rather than hardcoding here.
	{
	string id = "ACM News";
	getDocStreamManager().addDocStream(id);
	DocStream *doc_stream = getDocStreamManager().getDocStream(id);
	doc_stream->setProducer(id);
	shared_ptr<DocProducer> doc_producer(new RSSFeedParser(id, "http://cacm.acm.org/news.rss"));
	getDocStreamManager().addDocProducer(doc_producer);
	}
	{
	string id = "Engadget";
	getDocStreamManager().addDocStream(id);
	DocStream *doc_stream = getDocStreamManager().getDocStream(id);
	doc_stream->setProducer(id);
	shared_ptr<DocProducer> doc_producer(new RSSFeedParser(id, "http://www.engadget.com/rss.xml"));
	getDocStreamManager().addDocProducer(doc_producer);
	}
	{
	string id = "Google News Sci/Tech";
	getDocStreamManager().addDocStream(id);
	DocStream *doc_stream = getDocStreamManager().getDocStream(id);
	doc_stream->setProducer(id);
	shared_ptr<DocProducer> doc_producer(new RSSFeedParser(id, "http://news.google.com/news?ned=us&topic=t&output=rss"));
	getDocStreamManager().addDocProducer(doc_producer);
	}
	return true;
}

void DocStreamUI::displayDocStream(Request &request, Reply &reply) {
	User *user = getUserManager().getUser(request, true);
	assert(user);

	string stream_id = request.getFormData("stream_id");
	if (stream_id.empty()) {
		stream_id = user->getLastViewedDocStreamId();
		if (stream_id.empty() && ! getDocStreamManager().getAllDocStreams().empty()) {
			stream_id = getDocStreamManager().getAllDocStreams().front();
		}
	}

	DocStream *doc_stream = getDocStreamManager().getDocStream(stream_id);

	try {
		TemplateData t_main;
		t_main.set("user_id", user->getUserId());
		getSearchMenu().render(t_main, request);

		list<string> all_streams = getDocStreamManager().getAllDocStreams();
		BOOST_FOREACH(const string &id, all_streams) {
			t_main.addChild("each_stream")
				.set("stream_id", id)
				.set("selected", id == stream_id ? "true" : "false");
		}

		if (doc_stream) {
			doc_stream->update();
			renderDocList(t_main, *user, *doc_stream);
			user->setLastViewedDocStreamId(stream_id);
		}

		string content = getTemplateEngine().render(t_main, "doc_stream.htm");
		reply.content = content;
	}
	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		throw e;
	}
}

int cmpDocTimeDesc(const Document *a, const Document *b) {
	return a->date > b->date;
}

void DocStreamUI::renderDocList(templating::TemplateData &t_main, const User &user, const DocStream &doc_stream) {
	vector<const Document*> documents;
	documents.reserve(doc_stream.getDocList().size());
	BOOST_FOREACH(const Document &doc, doc_stream.getDocList()) {
		documents.push_back(&doc);
	}
	sort(documents.begin(), documents.end(), cmpDocTimeDesc);

	BOOST_FOREACH(const Document *doc, documents) {
		posix_time::ptime utc_time = posix_time::from_time_t(doc->date);
		posix_time::ptime local_time = util::utc_to_local(utc_time);
		string date_str = getDateStr(local_time, true);
		string time_str = getTimeStr(local_time);

		double sim = 0.0;
		int topic_id = -1;
		bool recommended = getDocStreamManager().isRecommended(user, *doc, sim, topic_id);

		templating::TemplateData t_doc = t_main.addChild("stream_doc");
		t_doc.set("title", doc->title)
			.set("url", doc->url)
			.set("doc_id", doc->doc_id)
			.set("date", time_str + " " + date_str)
			.set("source", doc->source)
			.set("recommended", recommended ? "true" : "false")
			.set("sim", lexical_cast<string>(sim))
			.set("topic_id", lexical_cast<string>(topic_id));
		if (doc->formatted_summary.empty()) {
			t_doc.set("summary", doc->summary);
		}
		else {
			t_doc.set("summary", doc->formatted_summary, false);
		}
	}
}

} // namespace ucair
