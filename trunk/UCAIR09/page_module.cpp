#include "page_module.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace boost;

namespace ucair {

void PageModule::recursiveRender(templating::TemplateData &t_main, Request &request, util::Properties &params) {
	render(t_main, request, params);
	BOOST_FOREACH(const shared_ptr<PageModule> &sub_module, sub_modules) {
		sub_module->recursiveRender(t_main, request, params);
	}
}

void PageModuleManager::addPageModule(const shared_ptr<PageModule> & module) {
	modules[module->getModuleId()] = module;
}

PageModule* PageModuleManager::getPageModule(const string &module_id) const {
	map<string, shared_ptr<PageModule> >::const_iterator itr = modules.find(module_id);
	if (itr == modules.end()) {
		return NULL;
	}
	return itr->second.get();
}

} // namespace ucair
