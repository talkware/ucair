#include "user_event.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "error.h"

using namespace std;
using namespace boost;

namespace ucair {

volatile int UserEvent::last_event_id = 0;

UserEvent::UserEvent() :
	event_id(++ last_event_id),
	timestamp(time(NULL)) {
}

void UserEvent::insertByTimestamp(list<shared_ptr<UserEvent> > &l, const shared_ptr<UserEvent> &e){
	for (list<shared_ptr<UserEvent> >::reverse_iterator itr = l.rbegin(); ; ++ itr){
		if (itr == l.rend() || (*itr)->timestamp <= e->timestamp){
			l.insert(itr.base(), e);
			break;
		}
	}
}

ClickResultEvent::ClickResultEvent() :
	result_pos(0) {
}

const string ClickResultEvent::type("click result");

string ClickResultEvent::saveValue() const {
	return str(format("%d\t%s") % result_pos % url);
}

void ClickResultEvent::loadValue (const string &value) {
	size_t pos = value.find('\t');
	try {
		result_pos = lexical_cast<int>(value.substr(0, pos));
	}
	catch (bad_lexical_cast &) {
		throw Error() << ErrorMsg("Failed to load ClickResultEvent from string");
	}
	url = value.substr(pos + 1);
}

ViewSearchPageEvent::ViewSearchPageEvent() :
	start_pos(0),
	result_count(0) {
}

const string ViewSearchPageEvent::type("view search page");

string ViewSearchPageEvent::saveValue() const {
	return str(format("%s\t%d\t%d") % view_id % start_pos % result_count);
}

void ViewSearchPageEvent::loadValue (const string &value) {
	size_t pos1 = value.find('\t');
	size_t pos2 = value.find('\t', pos1 + 1);
	view_id = value.substr(0, pos1);
	try {
		start_pos = lexical_cast<int>(value.substr(pos1 + 1, pos2 - pos1 - 1));
		result_count = lexical_cast<int>(value.substr(pos2 + 1));
	}
	catch (bad_lexical_cast &) {
		throw Error() << ErrorMsg("Failed to load ViewSearchPageEvent from string");
	}
}

} // namespace ucair
