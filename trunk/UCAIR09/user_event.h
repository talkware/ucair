#ifndef __user_event_h__
#define __user_event_h__

#include <ctime>
#include <list>
#include <string>
#include <boost/smart_ptr.hpp>
#include "prototype.h"

namespace ucair {

/// A user search event.
class UserEvent : public util::Prototype {
public:
	UserEvent();
	virtual ~UserEvent() {}

	int event_id; ///< unique event id that increases with time
	time_t timestamp;
	std::string search_id;

	/// Event type
	virtual std::string getType() const = 0;

	/// Saves event value to string.
	virtual std::string saveValue() const { return ""; }
	/// Loads event value from string.
	virtual void loadValue(const std::string &value) {}

	/// Inserts into an event list by timestamp.
	static void insertByTimestamp(std::list<boost::shared_ptr<UserEvent> > &l, const boost::shared_ptr<UserEvent> &e);

private:
	static volatile int last_event_id;
};

/// An event of user clicking on a URL.
class ClickResultEvent: public UserEvent {
public:
	ClickResultEvent();

	IMPLEMENT_CLONE(ClickResultEvent)

	int result_pos; ///< clicked result pos
	std::string url; ///< clicked url

	static const std::string type;
	std::string getType() const { return type; }
	std::string saveValue() const;
	void loadValue (const std::string &value);
};

/// An event of user viewing a search page.
class ViewSearchPageEvent: public UserEvent {
public:
	ViewSearchPageEvent();

	IMPLEMENT_CLONE(ViewSearchPageEvent)

	int start_pos; ///< search page start result pos
	int result_count; ///< how many results on page
	std::string view_id; ///< which view is used to render the page

	static const std::string type;
	std::string getType() const { return type; }
	std::string saveValue() const;
	void loadValue (const std::string &value);
};

} // namespace ucair

#endif
