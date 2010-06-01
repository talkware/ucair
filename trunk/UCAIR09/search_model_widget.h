#ifndef __search_model_widget_h__
#define __search_model_widget_h__

#include "page_module.h"

namespace ucair {

/// UI widget for displaying a search model.

class SearchModelWidget : public PageModule {
public:
	SearchModelWidget(): PageModule("search_model", "Search Model") {}

	void render(templating::TemplateData &t_main, Request &request, util::Properties &param);

private:

	bool displayModel(templating::TemplateData &t_main, User &user, const std::string &search_id, const std::string &model_name);
};

} // namespace ucair

#endif
