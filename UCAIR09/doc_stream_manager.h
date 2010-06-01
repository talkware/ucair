#ifndef __doc_stream_manager_h__
#define __doc_stream_manager_h__

#include "document.h"
#include <list>
#include <map>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/shared_ptr.hpp>
#include "component.h"
#include "main.h"
#include "properties.h"
#include "simple_index.h"

namespace ucair {

/// A list of documents that can also be efficiently looked up by their doc id.
typedef boost::multi_index::multi_index_container<
	Document,
	boost::multi_index::indexed_by<
		boost::multi_index::sequenced<>,
		boost::multi_index::ordered_unique<boost::multi_index::member<Document, std::string, &Document::doc_id> >
	>
> IndexedDocList;

typedef IndexedDocList::nth_index<1>::type DocIdIndex;

/// Models a document stream.
class DocStream {
public:
	DocStream(const std::string &doc_stream_id);

	/// Returns stream id.
	std::string getId() const { return doc_stream_id; }

	/// Sets the id of the doc producer.
	void setProducer(const std::string &doc_producer_id_) { doc_producer_id = doc_producer_id_; }
	/// Returns the id of the doc producer.
	std::string getProducer() const { return doc_producer_id; }

	/// Adds a document to the stream.
	void addDoc(const Document &doc);
	/// Looks up a document by its id.
	Document* getDoc(const std::string &doc_id);
	/// Returns all documents in the stream.
	const IndexedDocList& getDocList() const { return doc_list; }

	/// Updates documents from the producer and indexes them.
	void update();

	/// Returns the index for documents in the stream.
	indexing::SimpleIndex* getIndex() const { return index.get(); }

	util::Properties properties; ///< additional properties

private:
	IndexedDocList doc_list;
	std::string doc_stream_id;
	std::string doc_producer_id;
	boost::shared_ptr<indexing::SimpleIndex> index;

friend class DocProducer;
};

/// Models a document stream source/producer.
class DocProducer {
public:
	DocProducer(const std::string &doc_producer_id_) : doc_producer_id(doc_producer_id_) {}
	virtual ~DocProducer() {}

	/// Returns id of the doc producer.
	std::string getId() const { return doc_producer_id; }

	/// Injects documents into the given doc stream.
	virtual bool produceDoc(DocStream &doc_stream) = 0;

protected:
	std::string doc_producer_id;
};

/// Manages all document streams.
class DocStreamManager : public Component {
public:

	bool initialize();

	/// Returns a doc stream by id.
	DocStream* getDocStream(const std::string &doc_stream_id);
	/// Registers a doc stream.
	void addDocStream(const std::string &doc_stream_id);
	/// Returns ids of all doc streams.
	std::list<std::string> getAllDocStreams() const;

	/// Registers a doc producer.
	void addDocProducer(const boost::shared_ptr<DocProducer> &producer);
	/// Returns a doc producer by id.
	DocProducer* getDocProducer(const std::string &doc_producer_id);

	/*! \brief Decides whether a document should be recommended to a user based on its similarity to a search topic.
	 *
	 *  \param[in] user
	 *  \param[in] doc
	 *  \param[out] user-doc maximal similarity between the document and a user search topic
	 *  \param[out] topic_id which topic is matched
	 *  \return true if recommended
	 */
	bool isRecommended(const User &user, const Document &doc, double &sim, int &topic_id);

private:
	std::map<std::string, DocStream> doc_streams;
	std::map<std::string, boost::shared_ptr<DocProducer> > producers;

	double recommend_min_sim; ///< similarity must be greater than this to recommend a document.
};

DECLARE_GET_COMPONENT(DocStreamManager);

} // namespace ucair

#endif
