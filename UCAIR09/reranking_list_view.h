#ifndef __reranking_list_view_h__
#define __reranking_list_view_h__

#include <set>
#include <string>
#include "result_list_view.h"

namespace ucair {

/// UI widget to allow user to change reranking settings.
class RerankingSettingsWidget : public PageModule {
public:
	RerankingSettingsWidget() : PageModule("reranking_settings", "Reranking settings") {}

	void render(templating::TemplateData &t_main, Request &request, util::Properties &param);
};

/// A special result list view that reranks results by search model.
class RerankingResultListView: public ResultListView {
public:

	RerankingResultListView(const std::string &id, const std::string &name);

	void render(templating::TemplateData &t_main, Request &request, util::Properties &params);

protected:

	void makeRanking(User &user, const Search &search, std::vector<int> &ranking);

	void renderResult(templating::TemplateData &t_result, User &user, const Search &search, const SearchResult &result);

	double dir_prior;

	std::set<int> promoted_results;
};

} // namespace ucair

#endif
