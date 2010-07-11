#include "reranking_list_view.h"
#include <cassert>
#include "common_util.h"
#include "config.h"
#include "index_manager.h"
#include "logger.h"
#include "search_model.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

void RerankingSettingsWidget::render(templating::TemplateData &t_main, Request &request, util::Properties &params) {
	string search_id;
	bool rerank_all = false;
	int max_results_to_promote = 0;
	bool rerank_despite_inadaptive_model = false;
	try {
		search_id = params.get<string>("search_id");
		rerank_all = params.get<bool>("rerank_all");
		max_results_to_promote = params.get<int>("max_results_to_promote");
		rerank_despite_inadaptive_model = params.get<bool>("rerank_despite_inadaptive_model");
	}
	catch (bad_any_cast &) {
		getLogger().error("Invalid param for ResultListView");
	}

	templating::TemplateData t_settings = t_main.addChild("reranking_settings");
	t_settings.set("widget_id", getModuleId())
		.set("widget_name", getModuleName())
		.set("reranking_scope", rerank_all ? "all" : "top")
		.set("max_results_to_promote", lexical_cast<string>(max_results_to_promote))
		.set("rerank_despite_inadaptive_model", rerank_despite_inadaptive_model ? "true" : "false");
	string content = templating::getTemplateEngine().render(t_settings, "reranking_settings.htm");
	t_main.addChild("in_right_pane").set("content", content, false);
}

RerankingResultListView::RerankingResultListView(const string &id, const string &name):
	ResultListView(id, name) {
	dir_prior = util::getParam<double>(Main::instance().getConfig(), "snippet_dir_prior");
}

void RerankingResultListView::render(templating::TemplateData &t_main, Request &request, util::Properties &params){
	User *user = getUserManager().getUser(request, false);
	assert(user);

	if (! user->properties.has("search_model_name")) {
		// Read from config file.
		// which search model to use
		user->properties.set("search_model_name", util::getParam<string>(user->getConfig(), "default_search_model"));
		// whether to rerank all search results, or just the unviewed ones
		user->properties.set("rerank_all", util::getParam<bool>(user->getConfig(), "rerank_all"));
		// rerank even if the search model is not adaptive
		user->properties.set("rerank_despite_inadaptive_model", util::getParam<bool>(user->getConfig(), "rerank_despite_inadaptive_model"));
		// how many results to promote at most
		user->properties.set("max_results_to_promote", util::getParam<int>(user->getConfig(), "max_results_to_promote"));
	}

	string search_model_name = user->properties.get<string>("search_model_name");
	bool rerank_all = user->properties.get<bool>("rerank_all");
	bool rerank_despite_inadaptive_model = user->properties.get<bool>("rerank_despite_inadaptive_model");
	int max_results_to_promote = user->properties.get<int>("max_results_to_promote");

	// Process user form input from the reranking settings widget.
	string s = request.getFormData("settings");
	if (! s.empty()) {
		s = request.getFormData("rerank_scope");
		if (s == "all") {
			rerank_all = true;
		}
		else if (s == "top") {
			rerank_all = false;
		}
		user->properties.set("rerank_all", rerank_all);

		s = request.getFormData("max_promote");
		try {
			int n = lexical_cast<int>(s);
			if (n >= 0) {
				max_results_to_promote = n;
				user->properties.set("max_results_to_promote", max_results_to_promote);
			}
		}
		catch (bad_lexical_cast) {
		}

		s = request.getFormData("rerank_inadaptive");
		rerank_despite_inadaptive_model = ! s.empty();
		user->properties.set("rerank_despite_inadaptive_model", rerank_despite_inadaptive_model);
	}
	s = request.getFormData("model");
	if (getSearchModelManager().getModelGen(s)) {
		search_model_name = s;
		user->properties.set("search_model_name", search_model_name);
	}

	params.set("rerank_all", rerank_all);
	params.set("max_results_to_promote", max_results_to_promote);
	params.set("rerank_despite_inadaptive_model", rerank_despite_inadaptive_model);
	params.set("search_model_name", search_model_name);
	ResultListView::render(t_main, request, params);
}

void RerankingResultListView::makeRanking(User &user, const Search &search, vector<int> &ranking){
	promoted_results.clear();

	UserSearchRecord *search_record = user.getSearchRecord(search.getSearchId());
	if (! search_record){
		return;
	}

	string search_model_name = user.properties.get<string>("search_model_name");
	bool rerank_all = user.properties.get<bool>("rerank_all");
	bool rerank_despite_inadaptive_model = user.properties.get<bool>("rerank_despite_inadaptive_model");
	int max_results_to_promote = user.properties.get<int>("max_results_to_promote");

	vector<int> base_ranking;
	ResultListView::makeRanking(user, search, base_ranking);

	if (! util::isASCIIPrintable(search.query.text)) {
		ranking = base_ranking;
		return;
	}

	SearchModel model =	getSearchModelManager().getModel(*search_record, search, search_model_name);
	if (model.probs.empty() || ! rerank_despite_inadaptive_model && ! model.isAdaptive()){
		ranking = base_ranking;
		return;
	}

	util::UniqueList adaptive_ranking;

	if (! rerank_all){
		const util::UniqueList &viewed_results = search_record->getViewedResults();
		copy(viewed_results.begin(), viewed_results.end(), back_inserter<util::UniqueList>(adaptive_ranking));
	}

	vector<pair<int, double> > scores;
	indexing::SimpleKLRetriever retriever(*indexing::ValueMap::from(getIndexManager().getColProbs()), getIndexManager().getDefaultColProb(), dir_prior);
	assert(search_record->getIndex());
	retriever.retrieve(*search_record->getIndex(), *indexing::ValueMap::from(model.probs), scores);

	typedef pair<int, double> P;
	BOOST_FOREACH(const P &p, scores){
		if (! rerank_all && max_results_to_promote >= 0 && (int) promoted_results.size() >= max_results_to_promote){
			break;
		}
		string doc_name = search_record->getIndex()->getDocDict().getName(p.first);
		int result_pos;
		tie(tuples::ignore, result_pos) = parseDocName(doc_name);
		if (result_pos > 0){
			bool inserted = false;
			tie(tuples::ignore, inserted) = adaptive_ranking.push_back(result_pos);
			if (inserted){
				promoted_results.insert(result_pos);
			}
		}
	}

	copy(base_ranking.begin(), base_ranking.end(), back_inserter<util::UniqueList>(adaptive_ranking));

	copy(adaptive_ranking.begin(), adaptive_ranking.end(), back_inserter(ranking));
}

void RerankingResultListView::renderResult(templating::TemplateData &t_result, UserSearchRecord &search_record, const Search &search, const SearchResult &result) {
	ResultListView::renderResult(t_result, search_record, search, result);
	if (promoted_results.find(result.original_rank) != promoted_results.end()){
		t_result.set("additional_search_result_class", "promoted_search_result");
	}
}

} // namespace ucair
