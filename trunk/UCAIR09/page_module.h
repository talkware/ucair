#ifndef __search_page_module_h__
#define __search_page_module_h__

#include <list>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "main.h"
#include "properties.h"
#include "search_engine.h"
#include "template_engine.h"
#include "ucair_server.h"
#include "user.h"

namespace ucair {

/*! \brief An encapsulated display module in the output HTML page.
 *
 *  This is to modulize page generation.
 *  A page module can include other modules as its children.
 */
class PageModule {
public:
	PageModule(const std::string &id, const std::string &name): page_module_id(id), page_module_name(name) {}
	virtual ~PageModule() {}

	std::string getModuleId() const { return page_module_id; }
	std::string getModuleName() const { return page_module_name; }

	void addChildModule(const boost::shared_ptr<PageModule> &sub_module) { sub_modules.push_back(sub_module); }

	/// Renders this page module.
	virtual void render(templating::TemplateData &t_main, Request &request, util::Properties &params) = 0;
	/// Renders this page module and its sub modules.
	void recursiveRender(templating::TemplateData &t_main, Request &request, util::Properties &params);

protected:
	std::string page_module_id; ///< For identification
	std::string page_module_name; ///< For display

	std::list<boost::shared_ptr<PageModule> > sub_modules;
};

/// Registry of page modules.
class PageModuleManager : public Component {
public:
	void addPageModule(const boost::shared_ptr<PageModule> &module);
	PageModule* getPageModule(const std::string &module_id) const;

private:
	std::map<std::string, boost::shared_ptr<PageModule> > modules;
};

DECLARE_GET_COMPONENT(PageModuleManager);

} // namespace ucair

#endif
