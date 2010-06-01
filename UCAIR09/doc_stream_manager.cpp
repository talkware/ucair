#include "doc_stream_manager.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "config.h"
#include "index_manager.h"
#include "mixture.h"
#include "search_topics.h"
#include "ucair_util.h"
#include "user.h"

using namespace std;
using namespace boost;

namespace ucair {

DocStream::DocStream(const string &doc_stream_id_) :
	doc_stream_id(doc_stream_id_),
	index(getIndexManager().newIndex()) {
}

void DocStream::addDoc(const Document &doc) {
	doc_list.push_back(doc);
}

Document* DocStream::getDoc(const string &doc_id) {
	DocIdIndex& doc_id_index = doc_list.get<1>();
	// http://www.boost.org/doc/libs/1_40_0/boost/multi_index/detail/bidir_node_iterator.hpp
	DocIdIndex::iterator itr = doc_id_index.find(doc_id);
	if (itr != doc_id_index.end()) {
		return &(itr.get_node()->value());
	}
	return NULL;
}

void DocStream::update() {
	DocProducer *doc_producer = getDocStreamManager().getDocProducer(doc_producer_id);
	assert(doc_producer);
	doc_producer->produceDoc(*this);
	BOOST_FOREACH(const Document &doc, doc_list) {
		indexDocument(*index, doc);
	}
}

bool DocStreamManager::initialize() {
	recommend_min_sim = util::getParam<double>(Main::instance().getConfig(), "recommend_min_sim");
	return true;
}

DocStream* DocStreamManager::getDocStream(const string &doc_stream_id) {
	map<string, DocStream>::iterator itr = doc_streams.find(doc_stream_id);
	if (itr != doc_streams.end()) {
		return &itr->second;
	}
	return NULL;
}

void DocStreamManager::addDocStream(const string &doc_stream_id) {
	doc_streams.insert(make_pair(doc_stream_id, DocStream(doc_stream_id)));
}

void DocStreamManager::addDocProducer(const shared_ptr<DocProducer> &producer) {
	producers.insert(make_pair(producer->getId(), producer));
}

list<string> DocStreamManager::getAllDocStreams() const {
	list<string> ids;
	for (map<string, DocStream>::const_iterator itr = doc_streams.begin(); itr != doc_streams.end(); ++ itr) {
		ids.push_back(itr->first);
	}
	return ids;
}

DocProducer* DocStreamManager::getDocProducer(const string &doc_producer_id) {
	map<string, shared_ptr<DocProducer> >::iterator itr = producers.find(doc_producer_id);
	if (itr != producers.end()) {
		return itr->second.get();
	}
	return NULL;
}

bool DocStreamManager::isRecommended(const User &user, const Document &doc, double &sim, int &topic_id) {
	// Estimate a mixture model from the document.
	map<int, double> doc_term_counts;
	map<int, double> doc_model;
	indexing::countTerms(getIndexManager().getTermDict(), doc.title + " " + doc.summary, doc_term_counts);
	vector<tuple<double, double, double> > values;
	for (map<int, double>::const_iterator itr = doc_term_counts.begin(); itr != doc_term_counts.end(); ++ itr) {
		double f = itr->second;
		double p = getIndexManager().getColProb(itr->first);
		values.push_back(make_tuple(f, p, 0.0));
	}
	estimateMixture(values, 0.9);
	int i = 0;
	for (map<int, double>::const_iterator itr = doc_term_counts.begin(); itr != doc_term_counts.end(); ++ itr, ++ i) {
		double q = values[i].get<2>();
		if (q > 0.0) {
			doc_model.insert(make_pair(itr->first, q));
		}
	}

	// Computes maximal similarity with a search topic.
	const map<int, UserSearchTopic>* topics = getUserSearchTopicManager().getAllSearchTopics(user.getUserId());
	assert(topics);
	sim = 0.0;
	topic_id = -1;
	for (map<int, UserSearchTopic>::const_iterator itr = topics->begin(); itr != topics->end(); ++ itr) {
		const UserSearchTopic &topic = itr->second;
		if (topic.trivial) {
			continue;
		}
		double sim_ = getCosSim(doc_model, topic.model);
		if (sim_ > sim) {
			sim = sim_;
			topic_id = itr->first;
		}
	}
	return sim > recommend_min_sim;
}

} // namespace ucair
