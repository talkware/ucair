#ifndef __session_widget_h__
#define __session_widget_h__

#include "page_module.h"

namespace ucair {

/// UI widget for displaying session information
class SessionWidget : public PageModule {
public:
	SessionWidget(): PageModule("session", "Session") {}

	void render(templating::TemplateData &t_main, Request &request, util::Properties &param);
};

} // namespace ucair

#endif
