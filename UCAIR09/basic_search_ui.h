#ifndef __basic_search_ui_h__
#define __basic_search_ui_h__

#include <list>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "page_module.h"
#include "search_menu.h"
#include "template_engine.h"
#include "ucair_server.h"

namespace ucair {

/// A search menu item that is used to select a search view.
class SearchViewMenuItem: public SearchMenuItem {
public:
	SearchViewMenuItem(const std::string &search_view_id_) : search_view_id(search_view_id_) {}
	std::string getName(const Request &request) const;
	std::string getLink(const Request &request) const;
	bool isActive(const Request &request) const;

private:
	std::string search_view_id;
};

/// Basic elements of the search UI.
class BasicSearchUI: public Component {
public:
	bool initialize();
	bool initializeUser(User &user);

	/// Called when a user views a search page.
	void displaySearchPage(Request &request, Reply &reply);

	/// Called when a user clicks on a search result.
	void clickResult(Request &request, Reply &reply);

	/// Called when a user gives explicit rating of a search result.
	void rateResult(Request &request, Reply &reply);

	/// Shows an external search engine page.
	void useExternalSearchEngine(Request &request, Reply &reply);

	/// Registers a search view.
	void addSearchView(const std::string &view_id);
	/// Returns ids of all search views.
	const std::list<std::string>& getAllSearchViews() const { return search_view_ids; }

	/// Render the search result numbers.
	static void renderSearchCounts(templating::TemplateData &t_main, long long total_result_count, int start_pos, int result_count);
	/// Render the Previous/Next page links.
	static void renderPrevNextPage(templating::TemplateData &t_main, long long total_result_count, int start_pos, int result_count);

private:
	std::list<std::string> search_view_ids;
	int first_page_fetch_result_count; ///< Number of results to fetch when you start a search
	int next_pages_fetch_result_count; ///< Number of results to fetch when you click "Next"
};

DECLARE_GET_COMPONENT(BasicSearchUI)

}// namespace ucair

#endif
