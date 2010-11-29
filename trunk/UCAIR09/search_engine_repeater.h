#ifndef __search_engine_repeater_h__
#define __search_engine_repeater_h__

#include <string>
#include <boost/smart_ptr.hpp>
#include "search_engine.h"

namespace ucair {

/// Allows one to send repeated requests to a base search engine, thus to circumvent the max allowed result limit.
class SearchEngineRepeater: public SearchEngine {
public:
	SearchEngineRepeater(const boost::shared_ptr<SearchEngine> &base_search_engine);

	std::string getSearchEngineId() const;
	std::string getSearchEngineName() const;
	bool fetchResults(Search &search, int &start_pos, int &result_count);
	int maxAllowedResultCount() const { return 100; }

private:
	boost::shared_ptr<SearchEngine> base_search_engine;
};

}

#endif
