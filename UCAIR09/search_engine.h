#ifndef __search_engine_h__
#define __search_engine_h__

#include <list>
#include <map>
#include <string>
#include <vector>
#include "document.h"

namespace ucair {

/// Search query as submitted to a search engine.
class SearchQuery{
public:
	std::string text; ///< query text
	std::string spell_suggestion; ///< spell suggestion, if any

	class Keyword {
	public:
		std::vector<std::string> words;
		enum { NONE, INCLUDE, EXCLUDE, SITE } flag;
	};

	void parseKeywords();
	std::string concatKeywords() const;

	std::vector<Keyword> keywords;
};

/// A search result as returned by a search engine
class SearchResult: public Document{
public:
	SearchResult();
	SearchResult(const std::string &search_id, int original_rank);

	std::string search_id;
	int original_rank;
};

/// A search instance with query and results.
class Search{
public:
	Search();

	/*! \brief Whether all results in a given rank/pos range are present (in the map).
	 *  \param start_pos start pos (starts at 1)
	 *  \param result_count number of results in the range
	 *  \return true if all results between [start_pos .. start_pos + result_count) are present
	 */
	bool hasResults(int start_pos, int result_count = 1) const;

	/*! \brief Get a result at a given rank/pos.
	 *  \param start_pos start pos (starts at 1)
	 *  \return NULL if result is absent
	 */
	const SearchResult* getResult(int start_pos) const;

    std::string getSearchEngineId() const { return search_engine_id; }
    void setSearchEngineId(const std::string &search_engine_id_) { search_engine_id = search_engine_id_; }

    std::string getSearchId() const { return search_id; }
    void setSearchId(const std::string &search_id_) { search_id = search_id_; }

    long long getTotalResultCount() const { return total_result_count; }
    void setTotalResultCount(long long total_result_count_) { total_result_count = total_result_count_; }

    SearchQuery query; ///< query

    /// map from rank/pos to result
    std::map<int, SearchResult> results;

private:
    long long total_result_count; ///< search engine estimate of number of results
	std::string search_engine_id; ///< search engine identifier
	std::string search_id; ///< search id
};

/// Search engine interface
class SearchEngine{
public:
	virtual ~SearchEngine() {}

	/// Returns the id of the search engine (e.g. yahoo)
	virtual std::string getSearchEngineId() const = 0;

	/// Returns the formal name of the search engine (e.g. Yahoo!)
	virtual std::string getSearchEngineName() const = 0;

	/*! \brief Retrieves results for a query.
	 *  \param[in,out] search search instance that gives query and receives results
	 *  \param[in] start_pos start pos of the first result
	 *  \param[in] result_count number of results to retrieve
	 *  \return true if succeeded
	 */
	virtual bool fetchResults(Search &search, int start_pos, int result_count) = 0;

	/*! \brief Returns max number of results that can be returned at one time.
	 *
	 *  Usually search engine limits the number of results that can be returned on a page.
	 *  To get more results, one needs to specify a start pos to start at a different page.
	 *  For example, if max result count is 10, then start pos 11 brings you results 11-20 on the second page.
	 */
	virtual int maxAllowedResultCount() const { return 10; }
};

/// A search engine external to UCAIR, i.e. we show their search page as it is without modification.
class ExternalSearchEngine {
public:
	/*! \brief Constructor.
	 *  \param id search engine id
	 *  \param name search engine name
	 *  \param search_link_queryless search engine main page link, e.g. http://www.google.com
	 *  \param search_link_query_prefix search engine search page link prefix, e.g. http://www.google.com/search?q=
	 */
	ExternalSearchEngine(const std::string &id, const std::string &name, const std::string &search_link_queryless, const std::string &search_link_query_prefix);
	/// Returns the id of the search engine (e.g. yahoo)
	std::string getSearchEngineId() const { return id; }
	/// Returns the formal name of the search engine (e.g. Yahoo!)
	std::string getSearchEngineName() const { return name; }
	/// Returns the link of the search engine's search page for a given query.
	std::string getSearchLink(const std::string &query) const;
private:
	std::string id;
	std::string name;
	std::string search_link_queryless;
	std::string search_link_query_prefix;
};

/// Returns a list of all registered external search engines.
std::list<const ExternalSearchEngine*> getAllExternalSearchEngines();

} // namespace ucair

#endif
