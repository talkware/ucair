#ifndef __index_manager_h__
#define __index_manager_h__

#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "component.h"
#include "index_util.h"
#include "main.h"
#include "simple_index.h"

namespace ucair {

/// Manages data shared by different indices.
class IndexManager: public Component {
public:

	bool initialize();

	/// Creates an empty index.
	boost::shared_ptr<indexing::SimpleIndex> newIndex();

	/// Returns the global term id-str dictionary.
	indexing::NameDict& getTermDict() { return term_dict; }

	/// Returns collection probability if a term is found, or a default value otherwise.
	double getColProb(int term_id) const;

	/// Returns a dictionary which stores term probabilities in a background collection.
	const boost::unordered_map<int, double>& getColProbs() const { return col_probs; }

	/// Returns the default probability for a term not found in a background collection.
	double getDefaultColProb() const { return default_col_prob; }

private:

	indexing::NameDict term_dict;

	boost::unordered_map<int, double> col_probs;
	double default_col_prob;
};

DECLARE_GET_COMPONENT(IndexManager);

} // namespace ucair

#endif
