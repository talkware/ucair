#ifndef __console_ui_h__
#define __console_ui_h__

#include "component.h"
#include "search_menu.h"
#include "ucair_server.h"

namespace ucair {

/// A search menu item that links to this console page.
class ConsoleMenuItem: public SearchMenuItem {
public:
	std::string getName(const Request &request) const;
	std::string getLink(const Request &request) const;
	bool isActive(const Request &request) const;
};

/// UCAIR control console.
class ConsoleUI : public Component {
public:

	bool initialize();

	/// Displays the UCAIR console page.
	void displayConsole(Request &request, Reply &reply);

	/// Logs in a user.
	void userLogIn(Request &request, Reply &reply);

	void downloadOpenSearch(Request &request, Reply &reply);
};

} // namespace ucair

#endif

