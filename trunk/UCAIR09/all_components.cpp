#include <boost/smart_ptr.hpp>
#include "main.h"

#include "adaptive_search_ui.h"
#include "basic_search_ui.h"
#include "console_ui.h"
#include "doc_stream_manager.h"
#include "doc_stream_ui.h"
#include "index_manager.h"
#include "log_importer.h"
#include "logger.h"
#include "long_term_history_manager.h"
#include "page_module.h"
#include "search_history_ui.h"
#include "search_menu.h"
#include "search_model.h"
#include "search_proxy.h"
#include "static_file_handler.h"
#include "template_engine_wrapper.h"
#include "ucair_server.h"
#include "user_manager.h"
#include "search_topics.h"
#include "search_topics_ui.h"

using namespace std;
using namespace boost;

#define ADD_COMPONENT(name) Main::instance().addComponent(new name());

namespace ucair {

/// List of all UCAIR components. They will be created in the order below.
void addAllComponents() {
	ADD_COMPONENT(Logger);
	ADD_COMPONENT(UserManager);
	ADD_COMPONENT(TemplateEngineWrapper);
	ADD_COMPONENT(SearchProxy);
	ADD_COMPONENT(IndexManager);
	ADD_COMPONENT(UCAIRServer);
	ADD_COMPONENT(StaticFileHandler);
	ADD_COMPONENT(SearchModelManager);
	ADD_COMPONENT(LongTermHistoryManager);
	ADD_COMPONENT(PageModuleManager);
	ADD_COMPONENT(UserSearchTopicManager);
	ADD_COMPONENT(DocStreamManager);

	ADD_COMPONENT(SearchMenu);
	ADD_COMPONENT(BasicSearchUI);
	ADD_COMPONENT(AdaptiveSearchUI);
	ADD_COMPONENT(SearchHistoryUI);
	ADD_COMPONENT(SearchTopicsUI);
	ADD_COMPONENT(DocStreamUI);
	ADD_COMPONENT(ConsoleUI);

	ADD_COMPONENT(LogImporter);
}

} // namespace ucair
