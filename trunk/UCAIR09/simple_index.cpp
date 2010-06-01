#include "simple_index.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "common_util.h"

using namespace std;
using namespace boost;

namespace indexing {

SimpleIndex::SimpleIndex(NameDict &term_dict_):
	term_dict(term_dict_)
{
	doc_info_list.push_back(DocInfo()); // dummy DocInfo for subscript 0
}

void SimpleIndex::clear() {
	doc_dict.clear();
	doc_info_list.resize(1);
	term_info_map.clear();
}

bool SimpleIndex::addDoc(const string &doc_name, const ValueMap &doc_term_counts){
	if (doc_dict.getId(doc_name) != -1){
		return false;
	}
	int doc_id = doc_dict.getId(doc_name, true);

	doc_info_list.push_back(DocInfo());
	DocInfo &doc_info = doc_info_list.back();

	for (shared_ptr<ConstValueIterator> value_itr = doc_term_counts.const_iterator(); value_itr->ok(); value_itr->next()){
		const int term_id = value_itr->id();
		const double term_count = value_itr->get();
		doc_info.doc_length += term_count;
		doc_info.term_list.push_back(make_pair(term_id, term_count));

		map<int, TermInfo>::iterator itr;
		tie(itr, tuples::ignore) = term_info_map.insert(make_pair(term_id, TermInfo()));
		itr->second.doc_list.push_back(make_pair(doc_id, term_count));
	}

	return true;
}

int SimpleIndex::getDocCount() const {
	return (int) doc_info_list.size() - 1;
}

double SimpleIndex::getDocLength(int doc_id) const {
	assert(doc_id > 0 && doc_id <= getDocCount());
	return doc_info_list[doc_id].doc_length;
}

const vector<pair<int, float> >* SimpleIndex::getTermList(int doc_id) const{
	assert(doc_id > 0 && doc_id <= getDocCount());
	return &(doc_info_list[doc_id].term_list);
}

const vector<pair<int, float> >* SimpleIndex::getDocList(int term_id) const{
	map<int, TermInfo>::const_iterator itr = term_info_map.find(term_id);
	if (itr != term_info_map.end()) {
		return &(itr->second.doc_list);
	}
	return NULL;
}

SimpleKLRetriever::SimpleKLRetriever(const ValueMap &col_probs_, double default_col_prob_, double dir_prior_):
	col_probs(col_probs_.copy()),
	default_col_prob(default_col_prob_),
	dir_prior(dir_prior_)
{
}

double SimpleKLRetriever::getColProb(int term_id) const {
	double prob = 0.0;
	if (col_probs->get(term_id, prob)){
		return prob;
	}
	return default_col_prob;
}

void SimpleKLRetriever::retrieve(const SimpleIndex &index, const ValueMap &query_term_counts, std::vector<std::pair<int, double> > &ranking) const {
	ranking.clear();

	vector<double> doc_scores(index.getDocCount() + 1);
	fill(doc_scores.begin(), doc_scores.end(), 0.0);

	double query_length = 0.0;
	double col_likelihood = 0.0;
	for (shared_ptr<ConstValueIterator> value_itr = query_term_counts.const_iterator(); value_itr->ok(); value_itr->next()){
		const int term_id = value_itr->id();
		const double query_term_count = value_itr->get();

		query_length += query_term_count;
		const double col_prob = getColProb(term_id);
		col_likelihood += query_term_count * log(col_prob);

		const vector<pair<int, float> > *doc_list = index.getDocList(term_id);
		if (doc_list) {
			typedef pair<int, float> P;
			BOOST_FOREACH(const P &p, *doc_list) {
				const int doc_id = p.first;
				const float doc_term_count = p.second;
				doc_scores[doc_id] += query_term_count * log(1.0 + doc_term_count / dir_prior / col_prob);
			}
		}
	}

	for (int doc_id = 1; doc_id <= index.getDocCount(); ++ doc_id){
		if (doc_scores[doc_id] > 0.0){
			double score = doc_scores[doc_id] + col_likelihood;
			score /= query_length;
			score += log(dir_prior / (index.getDocLength(doc_id) + dir_prior));
			ranking.push_back(make_pair(doc_id, score));
		}
	}
	sort(ranking.begin(), ranking.end(), util::cmp2ndReverse<int, double>);
}

} // namespace indexing
