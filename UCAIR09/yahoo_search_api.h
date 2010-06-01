#ifndef __yahoo_search_api_h__
#define __yahoo_search_api_h__

#include <string>
#include "search_engine.h"

namespace ucair {

/// Legacy support for the Yahoo! Search API XML specification.
class YahooSearchAPI {
public:
	/*! \brief Parses the HTML search page into a search instance.
	 *  \param[out] search search instance that receives results.
	 *              if it already contains content from a previous search page,
	 *              its result map and total result count will be updated.
	 *  \param[in] content HTML page content
	 *  \return true if succeeded
	 */
	static bool parsePage(Search &search, const std::string &content);
};

}

#endif
