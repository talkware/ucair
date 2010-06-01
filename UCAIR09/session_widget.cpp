#include "session_widget.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "logger.h"
#include "common_util.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

void SessionWidget::render(templating::TemplateData &t_main, Request &request, util::Properties &params) {
	User *user = getUserManager().getUser(request, false);
	assert(user);
	string search_id;
	try {
		search_id = params.get<string>("search_id");
	}
	catch (bad_any_cast &) {
		getLogger().error("Invalid param for SearchModelWidget");
	}

	const UserSearchRecord *search_record = user->getSearchRecord(search_id);
	assert(search_record);
	user->updateSession(search_record->getSessionId());
	set<string> session = user->getSession(search_record->getSessionId());
	set<string> time_based_session = user->getTimeBasedSession(search_id);
	copy(session.begin(), session.end(), inserter(time_based_session, time_based_session.end()));

	// We'll be showing everything in the current session (getSession) and close in time (getTimeBasedSession)
	// but only those in the actual session will be highlighted (marked as related)

	templating::TemplateData t_session = t_main.addChild("session");
	t_session.set("widget_name", getModuleName());
	BOOST_REVERSE_FOREACH(const string &search_id, time_based_session) {
		const UserSearchRecord *search_record = user->getSearchRecord(search_id);
		if (search_record) {
			posix_time::ptime search_time_utc = posix_time::from_time_t(search_record->getCreationTime());
			posix_time::ptime search_time_local = util::utc_to_local(search_time_utc);
			string time_str = getTimeStr(search_time_local);
			t_session.addChild("session_search")
				.set("search_id", search_record->getSearchId())
				.set("query", search_record->getQuery())
				.set("time", time_str)
				.set("related", session.find(search_id) == session.end() ? "unrelated" : "related");
		}
	}

	string content = templating::getTemplateEngine().render(t_session, "session.htm");
	t_main.addChild("in_right_pane").set("content", content, false);
}

} // namespace ucair
