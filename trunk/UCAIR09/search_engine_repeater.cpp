#include "search_engine_repeater.h"

using namespace std;
using namespace boost;

namespace ucair {

SearchEngineRepeater::SearchEngineRepeater(const shared_ptr<SearchEngine> &base_search_engine_):
		base_search_engine(base_search_engine_) {
}

string SearchEngineRepeater::getSearchEngineId() const {
	return base_search_engine->getSearchEngineId() + "2";
}

string SearchEngineRepeater::getSearchEngineName() const {
	return base_search_engine->getSearchEngineName() + "2";
}

bool SearchEngineRepeater::fetchResults(Search &search, int &start_pos, int &result_count) {
	int end_pos = start_pos + result_count - 1;
	int s = start_pos;
	int r = result_count;
	bool first_fetch = true;
	while (true) {
		bool ok = base_search_engine->fetchResults(search, s, r);
		if (! ok) {
			return false;
		}
		if (first_fetch) {
			start_pos = s;
			first_fetch = false;
		}
		s += r;
		if (s > end_pos) {
			result_count = s - start_pos;
			break;
		}
	}
	return true;
}

}
