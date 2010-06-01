#ifndef __simple_index_h__
#define __simple_index_h__

#include <map>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include "index_util.h"
#include "value_map.h"

namespace indexing {

/*! \brief A simple in-memory inverted index.
 *
 *  Maintains a term-doc matrix. One can look up terms in a doc and docs having a term.
 *  Index is initially empty and docs can be added to it.
 *  \sa SimpleKLRetriever
 */
class SimpleIndex{
public:

	/*! \brief Constructor.
	 *  \param term_dict term id-name dict
	 */
	SimpleIndex(NameDict &term_dict);

	/// \brief Clears added docs.
	void clear();

	/*! \brief Adds a doc to index.
	 *  \param doc_name doc name
	 *  \param doc_term_counts terms in this doc (term ids and weights)
	 *  \return false if addDoc failed
	 */
	bool addDoc(const std::string &doc_name, const ValueMap &doc_term_counts);

	/// Returns number of docs in index.
	int getDocCount() const;

	/// Returns length of a doc.
	double getDocLength(int doc_id) const;

	/*! \brief Returns terms in a doc (term ids and weights).
	 *  \param doc_id doc id
	 *  \return NULL if doc does not exist in index
	 */
	const std::vector<std::pair<int, float> >* getTermList(int doc_id) const;
	/*! \brief Returns docs having a term (doc ids and term weights in the corresponding docs)
	 *  \param term_id term id
	 *  \return NULL if term does not exist in index
	 */
	const std::vector<std::pair<int, float> >* getDocList(int term_id) const;

	/// Returns the term id-name dict.
	NameDict& getTermDict() { return term_dict; }
	/// Returns the doc id-name dict.
	NameDict& getDocDict() { return doc_dict; }

private:

	NameDict &term_dict; ///< term id-name dict
	NameDict doc_dict; ///< doc id-name dict

	/// Information about a doc.
	class DocInfo{
	public:
		DocInfo(): doc_length(0.0) {}
		double doc_length; ///< doc length
		std::vector<std::pair<int, float> > term_list; ///< term ids and weights
	};

	class TermInfo{
	public:
		std::vector<std::pair<int, float> > doc_list; ///< doc ids and term weights in the corresponding docs
	};

	std::vector<DocInfo> doc_info_list; ///< map from doc id (array subscript) to DocInfo

	std::map<int, TermInfo> term_info_map; /// map from term id to TermInfo
};

/*! A simple KL retrieval method to work with SimpleIndex
 *  \sa SimpleIndex
 */
class SimpleKLRetriever {
public:

	/*! \brief Constructor.
	 *  \param col_probs term probabilities in a background collection
	 *  \param default_col_prob background probability for a term that has not appeared in collection
	 *  \param dir_prior Dirichlet prior
	 */
	SimpleKLRetriever(const ValueMap &col_probs, double default_col_prob, double dir_prior);

	/*! \brief Ranks docs given a query.
	 *  \param[in] SimpleIndex simple inverted index
	 *  \param[in] query_term_counts query term ids and weights
	 *  \param[out] ranking doc ids and relevance scores ranked by score.
	 *               docs that do not overlap with query are not included.
	 */
	void retrieve(const SimpleIndex &index, const ValueMap &query_term_counts, std::vector<std::pair<int, double> > &ranking) const;

private:

	/*! \brief Returns probability of a term in the background collection.
	 *
	 *  If a term has not appeared in the collection, just return a default value.
	 */
	double getColProb(int term_id) const;

	boost::shared_ptr<const ValueMap> col_probs; ///< map from term ids to their probabilities in the background collection

	double default_col_prob; ///< background probability for a term that has not appeared in collection

	double dir_prior; ///< Dirichlet prior
};

} // namespace indexing

#endif
