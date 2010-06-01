#ifndef __search_topics_ui_h__
#define __search_topics_ui_h__

#include "component.h"
#include "search_menu.h"
#include "ucair_server.h"

namespace ucair {

/// A search menu item that links to this search topics page.
class SearchTopicsMenuItem: public SearchMenuItem {
public:
	std::string getName(const Request &request) const;
	std::string getLink(const Request &request) const;
	bool isActive(const Request &request) const;
};

class SearchTopicsUI : public Component {
public:

	bool initialize();

	/// Page handler for listing all search topics.
	void listSearchTopics(Request &request, Reply &reply);
	/// Page handler for showing details about a search topic.
	void displaySearchTopic(Request &request, Reply &reply);
};

} // namespace ucair

#endif

