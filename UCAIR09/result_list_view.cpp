#include "result_list_view.h"
#include "logger.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;
using namespace templating;

namespace ucair {

void ResultListView::render(templating::TemplateData &t_main, Request &request, util::Properties &params){
	User *user = getUserManager().getUser(request, false);
	assert(user);

	string search_id;
	int start_pos;
	int result_count;
	try {
		search_id = params.get<string>("search_id");
		start_pos = params.get<int>("start_pos");
		result_count = params.get<int>("result_count");
	}
	catch (bad_any_cast &) {
		getLogger().error("Invalid param for ResultListView");
	}
	const Search *search = user->getSearch(search_id);
	assert(search);
	UserSearchRecord *search_record = user->getSearchRecord(search_id);
	assert(search_record);

	const string property_name = "ranking_" + getModuleId();
	if (! search_record || ! search_record->properties.has(property_name)){
		return;
	}
	vector<int> &ranking = search_record->properties.get<vector<int> >(property_name);
	ranking.clear();
	makeRanking(*user, *search, ranking);

	TemplateData t_view = t_main.addChild("result_list_view"); // to inherit main's template data
	// Only show results on the current page.
	for (int i = start_pos; i < start_pos + result_count && i <= (int) ranking.size(); ++ i){
		int result_pos = ranking[i - 1];
		const SearchResult *result = search->getResult(result_pos);
		if (result){
			TemplateData t_result = t_view.addChild("search_result");
			t_result.set("result_pos", lexical_cast<string>(result_pos));
			renderResult(t_result, *user, *search, *result);
		}
	}

	t_main.set("search_view_content", getTemplateEngine().render(t_view, "result_list_view.htm"), false);
}

void ResultListView::makeRanking(User &user, const Search &search, vector<int> &ranking){
	map<int, SearchResult>::const_reverse_iterator itr = search.results.rbegin();
	if (itr == search.results.rend()){
		return;
	}
	int largest_result_pos = itr->first;
	ranking.reserve(largest_result_pos);
	for (int i = 1; i <= largest_result_pos; ++ i){
		ranking.push_back(i);
	}
}

void ResultListView::renderResult(TemplateData &t_result, User &user, const Search &search, const SearchResult &result){
	t_result.set("original_rank", lexical_cast<string>(result.original_rank));
	if (result.formatted_title.empty()){
		t_result.set("title", result.title);
	}
	else{
		t_result.set("title", result.formatted_title, false);
	}
	if (result.formatted_summary.empty()){
		t_result.set("summary", result.summary);
	}
	else{
		t_result.set("summary", result.formatted_summary, false);
	}
	t_result.set("url", result.url);
	if (result.formatted_display_url.empty()){
		string display_url = result.display_url;
		if (display_url.empty()) {
			display_url = getDisplayUrl(result.url);
		}
		t_result.set("display_url", display_url);
	}
	else{
		t_result.set("display_url", result.formatted_display_url, false);
	}
}

} // namespace ucair
