#ifndef __long_term_search_model_h__
#define __long_term_search_model_h__

#include "search_model.h"
#include "user.h"

namespace ucair {

/*! Generates a search model from long term search history.
 *  See the paper "Mining long-term search history to improve search accuracy" for details.
 */
class LongTermSearchModelGen : public SearchModelGen {
public:
	LongTermSearchModelGen(const std::string &model_name,
			const std::string &model_description,
			double min_cos_sim,
			int max_neighbors,
			double query_prior,
			double background_prior,
			int max_em_tries,
			int max_em_iterators);

	SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;

private:
	double min_cos_sim; ///< past search must be similar to the current one
	int max_neighbors; ///< max number of past searches to use as neighbors
	double query_prior; ///< prior on the query
	double background_prior; /// prior on the background collection
	int max_em_tries; /// max number of EM tries (different start conditions)
	int max_em_iterations; /// max number of EM iterations in one try
};

} // namespace ucair

#endif
