#ifndef __search_proxy_h__
#define __search_proxy_h__

#include <list>
#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "main.h"
#include "search_engine.h"

namespace ucair {

class SearchProxy: public Component {
public:

	bool initialize();
	bool finalize();
	bool initializeUser(User &user);

	enum ReturnCode { OK, BAD_PARAM, BAD_CONNECTION };

	/*! \brief Performs a search.
	 *  It will not go to the search engine if an identical search with the given params has been done before.
	 *  If only search id is given, query text and search engine id will be returned (if search id is found).
	 *  If only query text and search engine id are given, search id will be returned (whether search is old or new).
	 *  \param[in,out] search_id
	 *  \param[in,out] query_text query text
	 *  \param[in,out] search_engine_id search engine id
	 *  \param[in] start_pos which result to start fetch with
	 *  \param[in] result_count how many results to fetch
	 *  \return one of OK, BAD_PARAM, BAD_CONNECTION
	 */
	ReturnCode search(std::string &search_id, std::string &query_text, std::string &search_engine_id, int start_pos, int result_count);

	/*! \brief Returns a search instance.
	 *  \param search_id search id
	 *  \return NULL if not found
	 */
	const Search* getSearch(const std::string &search_id) const;

	/*! \brief Returns a search result.	
	 *  \param search_id search id
	 *  \param result_pos result pos/rank
	 *  \return NULL if not found
	 */
	const SearchResult* getResult(const std::string &search_id, int result_pos) const;

	/// Returns a search engine.
	SearchEngine* getSearchEngine(const std::string &search_engine_id) const;
	/// Returns a list of all search engines.
	std::list<SearchEngine*> getAllSearchEngines() const;

private:

	std::map<std::string, Search> searches;
	std::list<boost::shared_ptr<SearchEngine> > search_engines;
};

DECLARE_GET_COMPONENT(SearchProxy)

} // namespace ucair

#endif
