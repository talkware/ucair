#ifndef __adaptive_search_ui_h__
#define __adaptive_search_ui_h__

#include "component.h"
#include "ucair_server.h"

namespace ucair {

/// Additional widgets and views to add to the basic search UI.
class AdaptiveSearchUI: public Component {
public:

	bool initialize();

	/// Called when user clicks to force end of session
	void forceSessionEnd(Request &request, Reply &reply);
};

} // namespace

#endif
