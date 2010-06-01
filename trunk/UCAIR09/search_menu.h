#ifndef __search_menu_h__
#define __search_menu_h__

#include <list>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "main.h"
#include "template_engine.h"

namespace ucair {

class Request;

/// An item in the main search menu.
class SearchMenuItem {
public:
	virtual ~SearchMenuItem() {}
	// Returns menu item name.
	virtual std::string getName(const Request &request) const = 0;
	// Returns menu item link.
	virtual std::string getLink(const Request &request) const = 0;
	// Whether the menu item is currently selected.
	virtual bool isActive(const Request &request) const = 0;
	// Whether the menu item is visible.
	virtual bool isVisible(const Request &request) const { return true; }
};

/// The main search menu.
class SearchMenu : public Component {
public:
	void addMenuItem(const boost::shared_ptr<SearchMenuItem> &item) { items.push_back(item); }
	const std::list<boost::shared_ptr<SearchMenuItem> >& getMenuItems() const { return items; }
	void render(templating::TemplateData &t_main, const Request &request) const;

private:
	std::list<boost::shared_ptr<SearchMenuItem> > items;
};

DECLARE_GET_COMPONENT(SearchMenu)

} // namespace ucair

#endif
