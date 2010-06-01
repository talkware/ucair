#include "search_menu.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace boost;

namespace ucair {

void SearchMenu::render(templating::TemplateData &t_main, const Request &request) const {
	BOOST_FOREACH(const shared_ptr<SearchMenuItem> &item, items) {
		if (! item->isVisible(request)) {
			continue;
		}
		string name = item->getName(request);
		if (name.empty()) {
			continue;
		}
		string link = item->getLink(request);
		bool active = item->isActive(request);
		t_main.addChild("search_menu_item")
			.set("menu_item_name", name)
			.set("menu_item_link", link)
			.set("menu_item_active", active ? "active" : "inactive");
	}
}

} // namespace ucair
