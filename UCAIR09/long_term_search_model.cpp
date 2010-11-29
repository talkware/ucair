#include "long_term_search_model.h"
#include <algorithm>
#include <iterator>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "index_manager.h"
#include "index_util.h"
#include "mixture.h"
#include "ucair_util.h"
#include "user_manager.h"
#include "value_map.h"

using namespace std;
using namespace boost;

namespace ucair {

LongTermSearchModelGen::LongTermSearchModelGen(const string &model_name,
		const string &model_description,
		double min_cos_sim_,
		int max_neighbors_,
		double query_prior_,
		double background_prior_,
		int max_em_tries_,
		int max_em_iterations_,
		bool session_scope_) :
	SearchModelGen(model_name, model_description),
	min_cos_sim(min_cos_sim_),
	max_neighbors(max_neighbors_),
	query_prior(query_prior_),
	background_prior(background_prior_),
	max_em_tries(max_em_tries_),
	max_em_iterations(max_em_iterations_),
	session_scope(session_scope_) {
}

SearchModel LongTermSearchModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
	User *user = getUserManager().getUser(search_record.getUserId());
	assert(user);

	map<int, double> query_model = getSearchModelManager().getModel(search_record, search, "query").probs;
	map<int, double> pseudo_feedback_model = getSearchModelManager().getModel(search_record, search, "pseudo").probs;

	vector<string> neighbor_search_ids;

	user->updateSession(search_record.getSearchId());
	set<string> session = user->getSession(search_record.getSessionId());
	BOOST_FOREACH(const string &search_id, session) {
		if (search_id == search_record.getSearchId()) {
			continue; // Do not include self.
		}
		const UserSearchRecord* past_search_record = user->getSearchRecord(search_id);
		assert(past_search_record);
		if (past_search_record->getClickedResults().empty()) {
			continue;
		}
		neighbor_search_ids.push_back(search_id);
	}

	if (! session_scope) { // Not limited to session scope
		vector<pair<string, double> > search_scores = user->searchInHistory(*indexing::ValueMap::from(pseudo_feedback_model));
		typedef pair<string, double> P1;
		BOOST_FOREACH(const P1 &p, search_scores) {
			const string &search_id = p.first;
			if (session.find(search_id) != session.end()) {
				continue; // already covered
			}
			const UserSearchRecord* past_search_record = user->getSearchRecord(search_id);
			assert(past_search_record);
			if (past_search_record->getClickedResults().empty()) {
				continue;
			}
			map<int, double> past_search_model = user->getIndexedSearchModel(search_id);
			double cos_sim = getCosSim(past_search_model, pseudo_feedback_model);
			if (cos_sim >= min_cos_sim) {
				neighbor_search_ids.push_back(search_id);
				if ((int) neighbor_search_ids.size() >= max_neighbors) {
					break;
				}
			}
		}
	}

	bool isAdaptive = ! neighbor_search_ids.empty();

	SearchModel result(generateModelName(), generateModelDescription(), isAdaptive);

	if (isAdaptive) {
		vector<int> term_ids(pseudo_feedback_model.size());
		vector<double> term_counts(term_ids.size());
		map<int, int> term_id_to_pos;
		int i = 0;
		typedef pair<int, double> P2;
		BOOST_FOREACH(const P2 &p, pseudo_feedback_model) {
			tie(term_ids[i], term_counts[i]) = p;
			term_id_to_pos[term_ids[i]] = i;
			++ i;
		}

		vector<vector<double> > components(2 + neighbor_search_ids.size());
		for (size_t i = 0; i < components.size(); ++ i) {
			components[i].resize(term_ids.size());
			fill(components[i].begin(), components[i].end(), 0.0);
		}

		vector<double> &query_component = components[0];
		for (int i = 0; i < (int) term_ids.size(); ++ i) {
			map<int, double>::const_iterator itr = query_model.find(term_ids[i]);
			if (itr != query_model.end()) {
				query_component[i] = itr->second;
			}
		}

		vector<double> &background_component = components[1];
		for (int i = 0; i < (int) term_ids.size(); ++ i) {
			background_component[i] = getIndexManager().getColProb(term_ids[i]);
		}

		int k = 2;
		BOOST_FOREACH(const string &search_id, neighbor_search_ids) {
			vector<double> &search_component = components[k ++];
			map<int, double> search_model = user->getIndexedSearchModel(search_id);
			BOOST_FOREACH(const P2 &p, search_model) {
				map<int, int>::const_iterator itr = term_id_to_pos.find(p.first);
				if (itr != term_id_to_pos.end()) {
					search_component[itr->second] = p.second;
				}
			}
		}

		vector<double> weights(components.size());
		fill(weights.begin(), weights.end(), 0.0);
		weights[0] = query_prior;
		weights[1] = background_prior;
		estimateMixtureWeights(term_counts, components, weights, max_em_tries, max_em_iterations);

		for (int i = 0; i < (int) neighbor_search_ids.size(); ++ i) {
			string search_id = neighbor_search_ids[i];
			double weight = weights[i + 2];
			map<int, double> search_model = user->getIndexedSearchModel(search_id);
			BOOST_FOREACH(const P2 &p, search_model) {
				map<int, double>::iterator itr;
				tie(itr, tuples::ignore)= result.probs.insert(make_pair(p.first, 0.0));
				itr->second += p.second * weight;
			}
		}

		/*BOOST_FOREACH(const P2 &p, query_model) {
			map<int, double>::iterator itr;
			tie(itr, tuples::ignore)= result.probs.insert(make_pair(p.first, 0.0));
			itr->second += p.second * query_prior;
		}*/

		normalize(*indexing::ValueMap::from(result.probs));
		truncate(*indexing::ValueMap::from(result.probs), 20, 0.001);
	}

	return result;
}

LongTermShortTermSearchModelGen::LongTermShortTermSearchModelGen(const string &model_name,
		const string &model_description,
		const shared_ptr<SearchModelGen> &long_term_model_gen_,
		const shared_ptr<SearchModelGen> &short_term_model_gen_,
		double long_term_model_click_prior_) :
		SearchModelGen(model_name, model_description),
		long_term_model_gen(long_term_model_gen_),
		short_term_model_gen(short_term_model_gen_),
		long_term_model_click_prior(long_term_model_click_prior_) {
}

bool LongTermShortTermSearchModelGen::isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const {
	return long_term_model_gen->isOutdated(search_record, search_model) || short_term_model_gen->isOutdated(search_record, search_model);
}

SearchModel LongTermShortTermSearchModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
	SearchModel long_term_model = long_term_model_gen->getModel(search_record, search);
	SearchModel short_term_model = short_term_model_gen->getModel(search_record, search);

	if (long_term_model.probs.empty() || long_term_model_click_prior == 0.0) {
		return short_term_model;
	}
	if (short_term_model.probs.empty() || search_record.getClickedResults().empty()) {
		return long_term_model;
	}

	bool isAdaptive = long_term_model.isAdaptive() || short_term_model.isAdaptive();

	SearchModel result(generateModelName(), generateModelDescription(), isAdaptive);

	typedef pair<int, double> P;
	BOOST_FOREACH(const P &p, long_term_model.probs) {
		map<int, double>::iterator itr;
		tie(itr, tuples::ignore)= result.probs.insert(make_pair(p.first, 0.0));
		itr->second += p.second * long_term_model_click_prior;
	}
	BOOST_FOREACH(const P &p, short_term_model.probs) {
		map<int, double>::iterator itr;
		tie(itr, tuples::ignore)= result.probs.insert(make_pair(p.first, 0.0));
		itr->second += p.second * search_record.getClickedResults().size();
	}

	normalize(*indexing::ValueMap::from(result.probs));
	truncate(*indexing::ValueMap::from(result.probs), 20, 0.001);
	return result;
}

} // namespace ucair
