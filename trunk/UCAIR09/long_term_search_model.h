#ifndef __long_term_search_model_h__
#define __long_term_search_model_h__

#include "search_model.h"
#include "user.h"
#include <boost/smart_ptr.hpp>

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
			int max_em_iterators,
			bool session_scope = false);

	SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;

private:
	double min_cos_sim; ///< past search must be similar to the current one
	int max_neighbors; ///< max number of past searches to use as neighbors
	double query_prior; ///< prior on the query
	double background_prior; ///< prior on the background collection
	int max_em_tries; ///< max number of EM tries (different start conditions)
	int max_em_iterations; ///< max number of EM iterations in one try
	int session_scope; ///< limited history scope to within the current session
	int interpolate_click_weight; ///< click weight when interpolating with short-term model
};

class LongTermShortTermSearchModelGen : public SearchModelGen {
public:
	LongTermShortTermSearchModelGen(const std::string &model_name,
			const std::string &model_description,
			const boost::shared_ptr<SearchModelGen> &long_term_model_gen,
			const boost::shared_ptr<SearchModelGen> &short_term_model_gen,
			double long_term_model_click_prior);

	bool isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const;

	SearchModel getModel(const UserSearchRecord &search_record, const Search &search) const;

private:
	boost::shared_ptr<SearchModelGen> long_term_model_gen;
	boost::shared_ptr<SearchModelGen> short_term_model_gen;
	double long_term_model_click_prior; ///< how many clicks long term model is equivalent to when interpolating with short term model
};

} // namespace ucair

#endif
