#ifndef __search_history_ui_h__
#define __search_history_ui_h__

#include <map>
#include <string>
#include <vector>
#include "component.h"
#include "search_menu.h"

namespace ucair {

class Request;
class Reply;

/// Search menu item that links to this search history page.
class SearchHistoryMenuItem: public SearchMenuItem {
public:
	std::string getName(const Request &request) const;
	std::string getLink(const Request &request) const;
	bool isActive(const Request &request) const;
};

/// UI that allows a user to view and find in their search history.
class SearchHistoryUI: public Component {
public:

	bool initialize();

	/// Page handler to list searches from the history.
	void displaySearchHistory(Request &request, Reply &reply);

private:

	std::map<std::string, std::vector<std::string> > cached_searches;
};

}// namespace ucair

#endif
