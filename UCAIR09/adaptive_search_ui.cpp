#include "adaptive_search_ui.h"
#include <boost/bind.hpp>
#include "basic_search_ui.h"
#include "reranking_list_view.h"
#include "search_model.h"
#include "search_model_widget.h"
#include "session_widget.h"
#include "ucair_server.h"
#include "ucair_util.h"

using namespace std;
using namespace boost;

namespace ucair {

bool AdaptiveSearchUI::initialize(){
	// Adds the adaptive view.
	shared_ptr<RerankingResultListView> adaptive_search_view(new RerankingResultListView("adaptive", "Adaptive"));
	getPageModuleManager().addPageModule(adaptive_search_view);
	getBasicSearchUI().addSearchView("adaptive");

	// Adds the reranking settings widget to adaptive view.
	shared_ptr<RerankingSettingsWidget> reranking_settings_widget(new RerankingSettingsWidget);
	getPageModuleManager().addPageModule(reranking_settings_widget);
	adaptive_search_view->addChildModule(reranking_settings_widget);

	// Adds the search model widget to adaptive view.
	shared_ptr<SearchModelWidget> search_model_widget(new SearchModelWidget);
	getPageModuleManager().addPageModule(search_model_widget);
	adaptive_search_view->addChildModule(search_model_widget);

	// Adds the session widget to both adaptive view and base view.
	shared_ptr<SessionWidget> session_widget(new SessionWidget);
	getPageModuleManager().addPageModule(session_widget);
	adaptive_search_view->addChildModule(session_widget);
	getPageModuleManager().getPageModule("base")->addChildModule(session_widget);

	return true;
}

} // namespace ucair
