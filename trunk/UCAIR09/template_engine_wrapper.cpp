#include "template_engine_wrapper.h"
#include <string>
#include "config.h"
#include "main.h"

using namespace std;
using namespace boost;

namespace templating {

TemplateEngine& getTemplateEngine() {
	return ucair::Main::instance().getComponent<ucair::TemplateEngineWrapper>().getTemplateEngine();
}

} // namespace templating

namespace ucair {

bool TemplateEngineWrapper::initialize() {
	Main &main = Main::instance();
	string template_src_dir = util::getParam<string>(main.getConfig(), "template_src_dir");
	bool use_template_cache = util::getParam<bool>(main.getConfig(), "cache_templates");
	template_engine.reset(new templating::TemplateEngine(template_src_dir, use_template_cache));
	return true;
}

bool TemplateEngineWrapper::finalize() {
	template_engine.reset();
	return true;
}

} // namespace ucair
