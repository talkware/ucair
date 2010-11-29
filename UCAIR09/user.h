#ifndef __user_h__
#define __user_h__

#include <ctime>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include "properties.h"
#include "search_engine.h"
#include "simple_index.h"
#include "user_search_record.h"
#include "value_map.h"

namespace ucair {

// Special notes:
// Short-term means current login session (i.e. since the user last logged in / server last started).
// Long-term includes every login sessions in the past (usually excluding the current one).
// Do not confuse a login session with a search session. The latter refers to similar searches in a short time range.

class User {
public:
	User(const std::string &user_id);

	/// Returns user id.
	std::string getUserId() const { return user_id; }

	/// Returns name of the default search engine for the user.
	std::string getDefaultSearchEngine() const { return default_search_engine_id; }
	/// Sets the default search engine for the user.
	void setDefaultSearchEngine(const std::string &default_search_engine_id_) { default_search_engine_id = default_search_engine_id_; }

	/// Returns name of the default search view for the user.
	std::string getDefaultSearchView() const { return default_search_view_id; }
	/// Sets the default search view for the user.
	void setDefaultSearchView(const std::string &default_search_view_id_) { default_search_view_id = default_search_view_id_; }

	/*! \brief Creates a new user search record for a given search id.
	 *
	 *  \param search_id search id
	 *  \return newly created user search record
	 */
	UserSearchRecord* addSearchRecord(const std::string &search_id);
	/// Returns the user search record for a given search id (short/long term).
	UserSearchRecord* getSearchRecord(const std::string &search_id);
	/// Returns the user search record for a given search id (short/long term).
	const UserSearchRecord* getSearchRecord(const std::string &search_id) const;

	/// Returns query text for a given search id (short/long term).
	std::string getQuery(const std::string &search_id) const;
	/// Returns the search with a given search id (short/long term).
	const Search* getSearch(const std::string &search_id) const;

	/// Adds a new user event. A signal will be fired.
	void addEvent(const boost::shared_ptr<UserEvent> &event);
	/// Returns all events for the user in the short term.
	const std::list<boost::shared_ptr<UserEvent> >& getEvents() const;

	/*! \brief Whether the search is considered expired (i.e. no longer active).
	 *
	 *  If it is beyond the current login session, or more than search_expiration_time seconds ago,
	 *  it is considered inactive.
	 */
	bool isSearchExpired(const std::string &search_id) const;

	/// Returns the first search id in the short term.
	std::string getShortTermFirstSearchId() const;
	/// Returns the last search id in the short term.
	std::string getShortTermLastSearchId() const;
	/// Returns search ids in all login sessions (short/long term)
	const std::vector<std::string>& getAllSearchIds() const { return all_search_ids; }

	/*! Update the search indices according to the latest search models.
	 *
	 *  \param force_update force rebuilding the indices even when it may not be necessary
	 */
	void updateSearchIndices(bool force_update = false);
	/*! \brief Search short-term and long-term history for a given query.
	 *
	 *  \param query_terms query model
	 *  \return vector of (search id, relevance score) pairs
	 */
	std::vector<std::pair<std::string, double> > searchInHistory(const indexing::ValueMap &query_terms);
	/// Returns the search model for a given search if it has been indexed.
	std::map<int, double> getIndexedSearchModel(const std::string &search_id);

	/// Extra user properties.
	util::Properties properties;

	/*! \brief Returns all search ids in a given search session.
	 *
	 *  \param session_id session id (not just search id)
	 *  \return all search ids in the session.
	 */
	std::set<std::string> getSession(const std::string &session_id) const;
	/*! \brief Returns all search ids close to the given search in time.
	 *
	 *  Does not consider search similarity.
	 *  \param search_id search id
	 *  \return all search ids close in time.
	 */
	std::set<std::string> getTimeBasedSession(const std::string &search_id) const;
	void updateSession(const std::string &search_id);

	/// Sets the last viewed doc stream.
	void setLastViewedDocStreamId(const std::string &id) { last_viewed_doc_stream_id = id; }
	/// Returns name of the last viewed doc stream.
	std::string getLastViewedDocStreamId() const { return last_viewed_doc_stream_id; }

	/// Returns name-value pairs from user config file.
	std::map<std::string, std::string>& getConfig() { return config; }

	/// Manually force session to end.
	void forceSessionEnd();
	time_t getForcedSessionEndTime() const { return forced_session_end_time; }

private:

	std::string user_id;
	std::string default_search_engine_id;
	std::string default_search_view_id;
	std::map<std::string, UserSearchRecord> short_term_search_records;
	std::list<boost::shared_ptr<UserEvent> > short_term_events;
	std::vector<std::string> all_search_ids;

	// For each search, its search model is indexed in either short_term_search_index or long_term_search_index.
	// short_term_search_index only includes active short-term searches.
	boost::shared_ptr<indexing::SimpleIndex> short_term_search_index;
	// long_term_search_index includes long-term searches and inactive short-term searches.
	boost::shared_ptr<indexing::SimpleIndex> long_term_search_index;
	bool index_outdated;

	time_t forced_session_end_time;

	std::string last_viewed_doc_stream_id;

	std::map<std::string, std::string> config;

friend class LongTermHistoryManager;
};

} // namespace ucair

#endif
