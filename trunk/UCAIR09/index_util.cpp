#include "index_util.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include "common_util.h"
#include "logger.h"
#include "porter.h"

using namespace std;
using namespace boost;

namespace indexing {

int NameDict::getId(const string &name, bool insert_if_not_found){
	NameIndex& name_index = name_map.get<1>();
	NameIndex::iterator itr = name_index.find(name);
	if (itr == name_index.end()){
		if (insert_if_not_found){
			itr = name_index.insert(itr, name);
		}
		else{
			return -1;
		}
	}
	// Project the string hashmap's iterator to the random access array's iterator.
	NameMap::iterator itr0 = name_map.project<0>(itr);
	// The array element's index is the iterator's difference with the start.
	// Id is index plus one (so that it begins at 1 rather than 0).
	return itr0 - name_map.begin() + 1;
}

string NameDict::getName(int id){
	assert(id > 0 && id <= size());
	return name_map[id - 1];
}

void countTerms(NameDict &term_dict, const string &text, map<int, double> &term_counts, bool stem_term, bool update_term_dict){
	term_counts.clear();
	vector<string> terms = util::tokenizeWithPunctuation(text);
	BOOST_FOREACH(string &term, terms){
		if (! util::isASCIIPrintable(term)) {
			continue;
		}
		if (stem_term) {
			term = stem(term);
		}
		int term_id = term_dict.getId(term, update_term_dict);
		if (term_id > 0) {
			map<int, double>::iterator itr;
			tie(itr, tuples::ignore) = term_counts.insert(make_pair(term_id, 0.0));
			itr->second += 1.0;
		}
	}
}

bool loadColProbs(const std::string &file_name, NameDict &term_dict, ValueMap &col_probs, double &default_col_prob){
	col_probs.clear();
	ifstream fin(file_name.c_str());
	if (! fin){
		ucair::getLogger().error("Failed to open " + file_name);
		return false;
	}
	try{
		string line;
		if (! getline(fin, line)){
			return false;
		}
		size_t pos = line.find('\t');
		if (pos == string::npos){
			return false;
		}
		trim_right(line);
		int unique_term_count = lexical_cast<int>(line.substr(0, pos));
		long long total_term_count = lexical_cast<long long>(line.substr(pos + 1));
		default_col_prob = 1.0 / (total_term_count + unique_term_count);
		while (getline(fin, line)){
			size_t pos = line.find('\t');
			if (pos == string::npos){
				return false;
			}
			trim_right(line);
			string term = line.substr(0, pos);
			long long term_count = lexical_cast<long long>(line.substr(pos + 1));
			int term_id = term_dict.getId(term, true);
			double col_prob = (term_count + 1.0) / (total_term_count + unique_term_count);
			col_probs.set(term_id, col_prob, false);
		}
	}
	catch (bad_lexical_cast &){
		return false;
	}
	return true;
}

} // namespace indexing
