#include "search_model.h"
#include <cassert>
#include <boost/tuple/tuple.hpp>
#include "config.h"
#include "index_manager.h"
#include "long_term_search_model.h"
#include "mixture.h"
#include "search_proxy.h"
#include "ucair_util.h"

using namespace std;
using namespace boost;

namespace ucair {

SearchModel::SearchModel() : timestamp(time(NULL)), adaptive(false) {}

SearchModel::SearchModel(const string &model_name_, const string &model_description_, bool adaptive_) :
	model_name(model_name_),
	model_description(model_description_),
	timestamp(time(NULL)),
	adaptive(adaptive_) {
}

SearchModelGen::SearchModelGen(const string &model_name_, const string &model_description_) :
	model_name(model_name_),
	model_description(model_description_) {
}

////////////////////////////////////////////////////////////////////////////////

SearchModel QueryMLEModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
	SearchModel model(generateModelName(), generateModelDescription());
	model.probs.clear();
	map<int, double> query_term_counts;
	indexing::countTerms(getIndexManager().getTermDict(), search.query.concatKeywords(), query_term_counts);
	model.probs = query_term_counts;
	normalize(*indexing::ValueMap::from(model.probs));
	truncate(*indexing::ValueMap::from(model.probs), 20, 0.001);
	return model;
}

////////////////////////////////////////////////////////////////////////////////

WeightedClickModelGen::WeightedClickModelGen(const string &model_name, const string &model_description, double query_term_weight_, double clicked_result_term_weight_, double unclicked_result_term_weight_, bool use_pseudo_feedback_) :
	SearchModelGen(model_name, model_description),
	query_term_weight(query_term_weight_),
	clicked_result_term_weight(clicked_result_term_weight_),
	unclicked_result_term_weight(unclicked_result_term_weight_),
	use_pseudo_feedback(use_pseudo_feedback_) {}

bool WeightedClickModelGen::isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const {
	BOOST_FOREACH(const shared_ptr<UserEvent> &event, search_record.getEvents()) {
		if (event->timestamp <= search_model.getTimestamp()) {
			continue;
		}
		if (typeid(*event) == typeid(ClickResultEvent)) {
			return true;
		}
	}
	return false;
}

bool WeightedClickModelGen::isAdaptive(const UserSearchRecord &search_record) const {
	return clicked_result_term_weight > 0.0 && ! search_record.getClickedResults().empty();
}

void WeightedClickModelGen::countTermsWeighted(const UserSearchRecord &search_record, const Search &search, map<int, double> &term_counts) const {
	term_counts.clear();

	if (query_term_weight > 0.0) {
		indexing::countTerms(getIndexManager().getTermDict(), search.query.concatKeywords(), term_counts);
		for (map<int, double>::iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr) {
			itr->second *= query_term_weight;
		}
	}

	if (! search_record.getClickedResults().empty() || use_pseudo_feedback) {
		indexing::SimpleIndex *index = search_record.getIndex();
		if (index) {
			const util::UniqueList &clicks = search_record.getClickedResults();
			const util::UniqueList::nth_index<1>::type &click_set = clicks.get<1>();

			typedef pair<int, SearchResult> P1;
			BOOST_FOREACH(const P1 &p1, search.results) {
				int result_pos = p1.first;
				bool clicked = click_set.find(result_pos) != click_set.end();
				string doc_name = buildDocName(search_record.getSearchId(), result_pos);
				int doc_id = index->getDocDict().getId(doc_name);
				if (doc_id > 0) {
					const vector<pair<int, float> >* term_list = index->getTermList(doc_id);
					if (term_list) {
						typedef pair<int, float> P2;
						BOOST_FOREACH(const P2 &p2, *term_list) {
							int term_id = p2.first;
							double term_weight = p2.second * (clicked ? clicked_result_term_weight : unclicked_result_term_weight);
							if (term_weight > 0.0) {
								map<int, double>::iterator itr;
								tie(itr, tuples::ignore) = term_counts.insert(make_pair(term_id, 0.0));
								itr->second += term_weight;
							}
						}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

RocchioModelGen::RocchioModelGen(const string &model_name,
		const string &model_description,
		double query_term_weight,
		double clicked_result_term_weight,
		double unclicked_result_term_weight,
		bool use_pseudo_feedback) :
		WeightedClickModelGen(model_name, model_description, query_term_weight, clicked_result_term_weight, unclicked_result_term_weight, use_pseudo_feedback) {
}

SearchModel RocchioModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
	SearchModel model(generateModelName(), generateModelDescription(), isAdaptive(search_record));
	countTermsWeighted(search_record, search, model.probs);
	normalize(*indexing::ValueMap::from(model.probs));
	truncate(*indexing::ValueMap::from(model.probs), 20, 0.001);
	return model;
}

////////////////////////////////////////////////////////////////////////////////

MixtureModelGen::MixtureModelGen(const string &model_name,
		const string &model_description,
		double query_term_weight,
		double clicked_result_term_weight,
		double unclicked_result_term_weight,
		double bg_coeff_,
		bool use_pseudo_feedback) :
		WeightedClickModelGen(model_name, model_description, query_term_weight, clicked_result_term_weight, unclicked_result_term_weight, use_pseudo_feedback),
		bg_coeff(bg_coeff_) {
}

SearchModel MixtureModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
	SearchModel model(generateModelName(), generateModelDescription(), isAdaptive(search_record));

	map<int, double> term_counts;
	countTermsWeighted(search_record, search, term_counts);
	vector<tuple<double, double, double> > values;
	for (map<int, double>::const_iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr) {
		double f = itr->second;
		double p = getIndexManager().getColProb(itr->first);
		values.push_back(make_tuple(f, p, 0.0));
	}
	estimateMixture(values, bg_coeff);
	int i = 0;
	for (map<int, double>::const_iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr, ++ i) {
		double q = values[i].get<2>();
		if (q > 0.0) {
			model.probs.insert(make_pair(itr->first, q));
		}
	}
	truncate(*indexing::ValueMap::from(model.probs), 20, 0.001);
	return model;
}

////////////////////////////////////////////////////////////////////////////////

RelevanceFeedbackModelGen::RelevanceFeedbackModelGen(const string &model_name, const string &model_description, double bg_coeff_) :
	SearchModelGen(model_name, model_description), bg_coeff(bg_coeff_) {
}

bool RelevanceFeedbackModelGen::isOutdated(const UserSearchRecord &search_record, const SearchModel &search_model) const {
	BOOST_FOREACH(const shared_ptr<UserEvent> &event, search_record.getEvents()) {
		if (event->timestamp <= search_model.getTimestamp()) {
			continue;
		}
		if (typeid(*event) == typeid(RateResultEvent)) {
			return true;
		}
	}
	return false;
}

bool RelevanceFeedbackModelGen::isAdaptive(const UserSearchRecord &search_record) const {
	return ! search_record.getRatedResults().empty();
}

SearchModel RelevanceFeedbackModelGen::getModel(const UserSearchRecord &search_record, const Search &search) const {
	SearchModel model(generateModelName(), generateModelDescription(), isAdaptive(search_record));

	map<int, double> term_counts;

	indexing::SimpleIndex *index = search_record.getIndex();
	if (index) {
		const map<int, string> &ratings = search_record.getRatedResults();

		typedef pair<int, string> P1;
		BOOST_FOREACH(const P1 &p1, ratings) {
			int result_pos = p1.first;
			string rating = p1.second;
			if (UserSearchRecord::isRatingPositive(rating)) {
				string doc_name = buildDocName(search_record.getSearchId(), result_pos);
				int doc_id = index->getDocDict().getId(doc_name);
				if (doc_id > 0) {
					const vector<pair<int, float> >* term_list = index->getTermList(doc_id);
					if (term_list) {
						typedef pair<int, float> P2;
						BOOST_FOREACH(const P2 &p2, *term_list) {
							int term_id = p2.first;
							double term_weight = p2.second;
							if (term_weight > 0.0) {
								map<int, double>::iterator itr;
								tie(itr, tuples::ignore) = term_counts.insert(make_pair(term_id, 0.0));
								itr->second += term_weight;
							}
						}
					}
				}
			}
		}
	}

	vector<tuple<double, double, double> > values;
	for (map<int, double>::const_iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr) {
		double f = itr->second;
		double p = getIndexManager().getColProb(itr->first);
		values.push_back(make_tuple(f, p, 0.0));
	}
	estimateMixture(values, bg_coeff);
	int i = 0;
	for (map<int, double>::const_iterator itr = term_counts.begin(); itr != term_counts.end(); ++ itr, ++ i) {
		double q = values[i].get<2>();
		if (q > 0.0) {
			model.probs.insert(make_pair(itr->first, q));
		}
	}
	truncate(*indexing::ValueMap::from(model.probs), 20, 0.001);
	return model;
}

////////////////////////////////////////////////////////////////////////////////

void SearchModelManager::addModelGen(const shared_ptr<SearchModelGen> &model_gen) {
	model_gens.push_back(model_gen);
}

SearchModelGen* SearchModelManager::getModelGen(const string &model_name) const {
	BOOST_FOREACH(const shared_ptr<SearchModelGen> &model_gen, model_gens) {
		if (model_gen->generateModelName() == model_name) {
			return model_gen.get();
		}
	}
	return NULL;
}

list<string> SearchModelManager::getAllModelGens() const {
	list<string> names;
	BOOST_FOREACH(const shared_ptr<SearchModelGen> &model_gen, model_gens) {
		names.push_back(model_gen->generateModelName());
	}
	return names;
}

const SearchModel& SearchModelManager::getModel(const UserSearchRecord &search_record, const Search &search, const string &model_name) {
	SearchModelGen *model_gen = getModelGen(model_name);
	assert(model_gen);
	pair<string, string> key = make_pair(search_record.getSearchId(), model_name);
	map<pair<string, string>, SearchModel>::const_iterator itr = cached_models.find(key);
	if (itr == cached_models.end() || model_gen->isOutdated(search_record, itr->second)) {
		cached_models[key] = model_gen->getModel(search_record, search);
		itr = cached_models.find(key);
	}
	return itr->second;
}

bool SearchModelManager::initialize(){
	Main &main = Main::instance();
	double query_term_weight = util::getParam<double>(main.getConfig(), "query_term_weight");
	double clicked_result_term_weight = util::getParam<double>(main.getConfig(), "clicked_result_term_weight");
	double unclicked_result_term_weight = util::getParam<double>(main.getConfig(), "unclicked_result_term_weight");
	double bg_coeff = util::getParam<double>(main.getConfig(), "feedback_bg_coeff");
	double long_term_search_model_min_cos_sim = util::getParam<double>(main.getConfig(), "long_term_search_model_min_cos_sim");
	int long_term_search_model_max_neighbors = util::getParam<int>(main.getConfig(), "long_term_search_model_max_neighbors");
	double long_term_search_model_query_prior = util::getParam<double>(main.getConfig(), "long_term_search_model_query_prior");
	double long_term_search_model_background_prior = util::getParam<double>(main.getConfig(), "long_term_search_model_background_prior");
	int long_term_search_model_max_em_tries = util::getParam<int>(main.getConfig(), "long_term_search_model_max_em_tries");
	int long_term_search_model_max_em_iterations = util::getParam<int>(main.getConfig(), "long_term_search_model_max_em_iterations");
	double long_term_search_model_click_prior = util::getParam<double>(main.getConfig(), "long_term_search_model_click_prior");

	shared_ptr<QueryMLEModelGen> query(new QueryMLEModelGen("query", "Query MLE"));
	getSearchModelManager().addModelGen(query);

	shared_ptr<MixtureModelGen> pseudo(new MixtureModelGen(
			"pseudo",
			"pseudo feedback",
			query_term_weight,
			0.0,
			unclicked_result_term_weight,
			bg_coeff,
			true));
	getSearchModelManager().addModelGen(pseudo);

	shared_ptr<RelevanceFeedbackModelGen> explicit_feedback(new RelevanceFeedbackModelGen("explicit",
			"explicit feedback",
			bg_coeff));
	getSearchModelManager().addModelGen(explicit_feedback);

	shared_ptr<MixtureModelGen> single_search(new MixtureModelGen("single-search",
			"implicit feedback (single search)",
			query_term_weight,
			clicked_result_term_weight,
			unclicked_result_term_weight,
			bg_coeff,
			false));
	getSearchModelManager().addModelGen(single_search);

	shared_ptr<LongTermSearchModelGen> session(new LongTermSearchModelGen("session",
			"implicit feedback (session)",
			long_term_search_model_min_cos_sim,
			long_term_search_model_max_neighbors,
			long_term_search_model_query_prior,
			long_term_search_model_background_prior,
			long_term_search_model_max_em_tries,
			long_term_search_model_max_em_iterations,
			true));

	shared_ptr<LongTermShortTermSearchModelGen> session_with_short_term(new LongTermShortTermSearchModelGen(
			"session", "implicit feedback (session)", session, single_search, long_term_search_model_click_prior));
	getSearchModelManager().addModelGen(session_with_short_term);

	shared_ptr<LongTermSearchModelGen> long_term_history(new LongTermSearchModelGen("long-term-history",
			"implicit feedback (long-term history)",
			long_term_search_model_min_cos_sim,
			long_term_search_model_max_neighbors,
			long_term_search_model_query_prior,
			long_term_search_model_background_prior,
			long_term_search_model_max_em_tries,
			long_term_search_model_max_em_iterations,
			false));

	shared_ptr<LongTermShortTermSearchModelGen> long_term_history_with_short_term(new LongTermShortTermSearchModelGen(
			"long-term-history", "implicit feedback (long-term-history)", long_term_history, single_search, long_term_search_model_click_prior));
	getSearchModelManager().addModelGen(long_term_history_with_short_term);

	return true;
}

} // namespace ucair
