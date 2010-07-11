#ifndef __result_list_view_h__
#define __result_list_view_h__

#include <vector>
#include "page_module.h"
#include "common_util.h"

namespace ucair {

class Search;
class SearchResult;

/// UI view for showing a list of search results.
class ResultListView: public PageModule {
public:

	ResultListView(const std::string &id, const std::string &name): PageModule(id, name) {}

	void render(templating::TemplateData &t_main, Request &request, util::Properties &params);

protected:

	/*! Generates the result ranking.
	 *
	 *  \param[in] user
	 *  \param[in] search
	 *  \param[out] ranking ranking of all results in the search, visible or not
	 */
	virtual void makeRanking(User &user, const Search &search, std::vector<int> &ranking);

	/// Renders a given search result.
	virtual void renderResult(templating::TemplateData &t_result, UserSearchRecord &search_record, const Search &search, const SearchResult &result);
};

} // namespace ucair

#endif
