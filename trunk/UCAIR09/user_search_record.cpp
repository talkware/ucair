#include "user_search_record.h"
#include <boost/foreach.hpp>
#include "index_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

UserSearchRecord::UserSearchRecord(const string &user_id_, const string &search_id_, const std::string &query_, time_t creation_time_, bool is_from_past_history_) :
	user_id(user_id_),
	search_id(search_id_),
	query(query_),
	session_id(search_id),
	creation_time(creation_time_),
	index(getIndexManager().newIndex()),
	is_from_past_history(is_from_past_history_),
	prev(NULL),
	next(NULL) {
}

int UserSearchRecord::getLastStartPos(const string &view_id) const {
	BOOST_REVERSE_FOREACH(const boost::shared_ptr<UserEvent> &event, events){
		ViewSearchPageEvent *view_search_page_event = dynamic_cast<ViewSearchPageEvent*>(event.get());
		if (view_search_page_event){
			return view_search_page_event->start_pos;
		}
	}
	return 0;
}

time_t UserSearchRecord::getLastEventTime() const {
	if (! events.empty()){
		return events.back()->timestamp;
	}
	return 0;
}

void UserSearchRecord::addEvent(const shared_ptr<UserEvent> &event) {
	UserEvent::insertByTimestamp(events, event);
}

void UserSearchRecord::setResultRating(int pos, const string &rating) {
	if (rating.empty()) {
		rated_results.erase(pos);
	}
	else {
		rated_results[pos] = rating;
	}
}

string UserSearchRecord::getResultRating(int pos) const {
	map<int, string>::const_iterator itr = rated_results.find(pos);
	if (itr != rated_results.end()) {
		return itr->second;
	}
	return "";
}

bool UserSearchRecord::isRatingPositive(const string &rating) {
	if (rating == "Y" || rating == "y") {
		return true;
	}
	return false;
}

} // namespace ucair
