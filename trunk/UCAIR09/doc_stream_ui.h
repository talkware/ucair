#ifndef __doc_stream_ui_h__
#define __doc_stream_ui_h__

#include "component.h"
#include "doc_stream_manager.h"
#include "search_menu.h"
#include "template_engine.h"

namespace ucair {

class Request;
class Reply;

/// A search menu item that links to this document stream page.
class DocStreamMenuItem: public SearchMenuItem {
public:
	std::string getName(const Request &request) const;
	std::string getLink(const Request &request) const;
	bool isActive(const Request &request) const;
};

/// UI to show document streams and recommendations.
class DocStreamUI: public Component {
public:
	bool initialize();

	void displayDocStream(Request &request, Reply &reply);

private:
	void renderDocList(templating::TemplateData &t_main, const User &user, const DocStream &doc_stream);
};

} // namespace ucair

#endif
