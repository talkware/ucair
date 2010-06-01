#include "search_model_widget.h"
#include <cassert>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include "common_util.h"
#include "logger.h"
#include "search_model.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

void SearchModelWidget::render(templating::TemplateData &t_main, Request &request, util::Properties &params) {
	User *user = getUserManager().getUser(request, false);
	assert(user);

	string search_id;
	string search_model_name;
	try {
		search_id = params.get<string>("search_id");
		search_model_name = params.get<string>("search_model_name");
	}
	catch (bad_any_cast &) {
		getLogger().error("Invalid param for SearchModelWidget");
	}

	templating::TemplateData t_model = t_main.addChild("search_models");;
	if (displayModel(t_model, *user, search_id, search_model_name)) {
		string content = templating::getTemplateEngine().render(t_model, "language_model_widget.htm");
		t_main.addChild("in_right_pane").set("content", content, false);
	}
}

bool SearchModelWidget::displayModel(templating::TemplateData &t_model, User &user, const string &search_id, const string &model_name) {
	const Search *search = user.getSearch(search_id);
	if (! search) {
		return false;
	}

	if (! util::isASCIIPrintable(search->query.text)) {
		return false;
	}

	const UserSearchRecord *search_record = user.getSearchRecord(search_id);
	if (! search_record){
		return false;
	}

	if (! getSearchModelManager().getModelGen(model_name)) {
		return false;
	}

	const map<int, double> &model = getSearchModelManager().getModel(*search_record, *search, model_name).probs;
	vector<pair<int, double> > truncated_model;
	truncate(*indexing::ValueMap::from(model), truncated_model, 20, 0.001);

	t_model.set("search_id", search_id)
		.set("widget_name", getModuleName())
		.set("widget_id", getModuleId());

	list<string> model_names = getSearchModelManager().getAllModelGens();
	BOOST_FOREACH(const string &name, model_names) {
		string description = getSearchModelManager().getModelGen(name)->generateModelDescription();
		if (description.empty()) {
			description = name;
		}
		t_model.addChild("model")
			.set("model_name", name)
			.set("model_description", description)
			.set("selected", name == model_name ? "true" : "false");
	}

	typedef pair<int, double> P;
	BOOST_FOREACH(const P &p, truncated_model){
		indexing::SimpleIndex *index = search_record->getIndex();
		assert(index);
		string term = index->getTermDict().getName(p.first);
		t_model.addChild("language_model_term_info")
			.set("term", term)
			.set("prob", str(format("%1$.3f") % p.second));
	}
	if (truncated_model.size() < model.size()) {
		t_model.addChild("language_model_term_info")
			.set("term", "...")
			.set("prob", "...");
	}
	return true;
}

} // namespace ucair
