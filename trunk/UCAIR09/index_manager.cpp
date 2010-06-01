#include "index_manager.h"
#include <iostream>
#include "config.h"
#include "logger.h"
#include "ucair_util.h"

using namespace std;
using namespace boost;

namespace ucair {

bool IndexManager::initialize(){
	string col_stats_file_name = util::getParam<string>(Main::instance().getConfig(), "col_stats_file");
	if (! indexing::loadColProbs(col_stats_file_name, term_dict, *(indexing::ValueMap::from(col_probs)), default_col_prob)) {
		getLogger().error("Failed to load collection stats");
		return false;
	}
	return true;
}

shared_ptr<indexing::SimpleIndex> IndexManager::newIndex(){
	return shared_ptr<indexing::SimpleIndex>(new indexing::SimpleIndex(term_dict));
}

double IndexManager::getColProb(int term_id) const {
	unordered_map<int, double>::const_iterator itr = col_probs.find(term_id);
	if (itr != col_probs.end()){
		return itr->second;
	}
	return default_col_prob;
}

} // namespace ucair
