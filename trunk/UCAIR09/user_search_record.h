#ifndef __user_search_record_h__
#define __user_search_record_h__

#include <ctime>
#include <list>
#include <string>
#include <boost/smart_ptr.hpp>
#include "common_util.h"
#include "properties.h"
#include "simple_index.h"
#include "user_event.h"

namespace ucair {

/// All about a search made by a user.
class UserSearchRecord {
public:
	/*! Constructor.
	 *
	 * @param user_id user id
	 * @param search_id search id
	 * @param query query text
	 * @param creation_time search begin time
	 * @param is_from_past_history whether the search is loaded from past history
	 * @return
	 */
	UserSearchRecord(const std::string &user_id, const std::string &search_id, const std::string &query, time_t creation_time, bool is_from_past_history);

	/// Returns user id.
	std::string getUserId() const { return user_id; }
	/// Returns search id.
	std::string getSearchId() const { return search_id; }
	/// Returns query text.
	std::string getQuery() const { return query; }

	/// Returns session id (a session is a group of related searches).
	std::string getSessionId() const { return session_id; }
	void setSessionId(const std::string &session_id_) { session_id = session_id_; }

	/// Returns the time when this search was started.
	time_t getCreationTime() const { return creation_time; }
	/// Returns the time of the last user event on this search.
	time_t getLastEventTime() const;

	/// Returns the start result pos of the last viewed search page.
	int getLastStartPos(const std::string &view_id) const;
	/// Returns a list of viewed results.
	const util::UniqueList& getViewedResults() const { return viewed_results; }
	/// Returns a list of clicked results.
	const util::UniqueList& getClickedResults() const { return clicked_results; }
	/// Adds a viewed result.
	void addViewedResult(int pos) { viewed_results.push_back(pos); }
	/// Adds a clicked result.
	void addClickedResult(int pos) { clicked_results.push_back(pos); }

	/// Returns all user events on this search.
	const std::list<boost::shared_ptr<UserEvent> >& getEvents() const { return events; }
	/// Adds a user event.
	void addEvent(const boost::shared_ptr<UserEvent> &event);

	/// Returns the index of all results in this search.
	indexing::SimpleIndex* getIndex() const { return index.get(); }

	/// Whether this search was loaded from past history.
	bool isFromPastHistory() const { return is_from_past_history; }

	/// Returns the previous search record of this user. NULL if this is the first.
	UserSearchRecord* getPrevSearchRecord() const { return prev; }
	/// Returns the next search record of this user. NULL if this is the last.
	UserSearchRecord* getNextSearchRecord() const { return next; }

	util::Properties properties; ///< extra fields.

private:
	std::string user_id;
	std::string search_id;
	std::string query;
	std::string session_id;
	time_t creation_time;
	boost::shared_ptr<indexing::SimpleIndex> index;
	std::list<boost::shared_ptr<UserEvent> > events;
	// Unique list ensures results are ordered by insertion order and only occur once.
	util::UniqueList viewed_results;
	util::UniqueList clicked_results;
	bool is_from_past_history;
	UserSearchRecord *prev;
	UserSearchRecord *next;

friend class User;
friend class LongTermHistoryManager;
};

} // namespace ucair

#endif
