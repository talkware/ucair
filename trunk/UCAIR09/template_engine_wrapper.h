#ifndef __template_engine_wrapper_h__
#define __template_engine_wrapper_h__

#include <boost/smart_ptr.hpp>
#include "component.h"
#include "template_engine.h"

namespace ucair {

/// Wraps TemplateEngine as a UCAIR component.
class TemplateEngineWrapper : public Component {
public:

	bool initialize();
	bool finalize();

	templating::TemplateEngine& getTemplateEngine() { return *template_engine; }

private:
	boost::scoped_ptr<templating::TemplateEngine> template_engine;
};

} // namespace ucair

#endif
