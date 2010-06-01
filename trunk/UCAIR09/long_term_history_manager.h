#ifndef __long_term_history_manager_h__
#define __long_term_history_manager_h__

#include <map>
#include <set>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "main.h"
#include "search_engine.h"
#include "sqlitepp.h"
#include "user.h"

namespace ucair {

/// Defines what to be saved to search history database for a search.
class SearchSaveTask {
public:
	SearchSaveTask(const std::string &user_id, const std::string &search_id);

	void saveAll(sqlite::Connection &conn, bool final_call = false);
	/// When this task can be deleted to save memory.
	bool isFinished() const;

	std::string user_id;
	std::string search_id;

private:
	void saveQuery(sqlite::Connection &conn);
	void saveResults(sqlite::Connection &conn);
	void saveModel(sqlite::Connection &conn, bool final_call);

	bool query_saved;
	std::set<int> results_saved;
	bool model_saved;
};

/// Defines what to be saved to search history database for a user.
class UserSaveTask {
public:
	UserSaveTask(const std::string &user_id);

	void saveAll(sqlite::Connection &conn, bool final_call = false);

	std::string user_id;

private:
	void saveEvents(sqlite::Connection &conn);

	int max_event_id_saved;
};

/// Defines what to be loaded from search history database for a search.
class SearchLoadTask {
public:
	SearchLoadTask(const std::string &user_id, const std::string &search_id);

	Search search;
	boost::shared_ptr<UserSearchRecord> search_record;
	std::map<int, double> model;

	std::string user_id;
	std::string search_id;

	void loadResults(sqlite::Connection &conn);

private:
	bool results_loaded;
};

/*! \brief Manages save/load of long-term user search history.
 *  Note: each user has a separate sqlite database for their search history.
 */
class LongTermHistoryManager: public Component {
public:

	bool initialize();
	bool initializeUser(User &user);
	bool finalizeUser(User &user);

	/// Adds a save task for a search.
	void addSearchSaveTask(const std::string &user_id, const std::string &search_id);
	/// Adds a save task for a user.
	void addUserSaveTask(const std::string &user_id);

	/// Returns the database connection for a user.
	sqlite::Connection& getConnection(const std::string &user_id);

	/*! Returns a search from long-term history.
	 *
	 *  \param search id
	 *  \param load_results whether to load the search results (extra time)
	 *  \return search, NULL if not found in long-term history
	 */
	const Search* getSearch(const std::string &search_id, bool load_results = true);
	/// Returns a user search record (NULL if not found).
	UserSearchRecord* getSearchRecord(const std::string &search_id);
	/// Returns a search model (NULL if not found).
	const std::map<int, double>& getModel(const std::string &search_id) const;

	/// Returns the id of the first search in history.
	std::string getFirstSearchId() const;
	/// Returns the id of the last search in history.
	std::string getLastSearchId() const;

	/// Creates table structures for a new user database.
	static void sqlCreateDatabase(sqlite::Connection &conn);

private:
	bool createDatabase(const std::string &user_id);
	/// Loads all search history of a user.
	void loadHistory(const std::string &user_id);
	/// Loads all searches of a user.
	void loadSearches(sqlite::Connection &conn, const std::string &user_id);
	/// Loads all search models of a user.
	void loadModels(sqlite::Connection &conn);
	/// Loads all search events of a user.
	void loadEvents(sqlite::Connection &conn);
	/// Indexes the search history of a user.
	void buildIndex(const std::string &user_id);

	/*! \brief Saves user history.
	 *  \param user_id user id
	 *  \param final_call true when user exits or server shuts down
	 */
	void saveHistory(const std::string &user_id, bool final_call);

	/// Called when UCAIR server is idel.
	void onIdle();

	/// map from user id to user db connection
	std::map<std::string, boost::shared_ptr<sqlite::Connection> > connections;
	/// map from search id to search save task
	std::map<std::string, SearchLoadTask> search_load_tasks;
	/// map from search id to search save task
	std::map<std::string, SearchSaveTask> search_save_tasks;
	/// map from user id to user save task
	std::map<std::string, UserSaveTask> user_save_tasks;
};

DECLARE_GET_COMPONENT(LongTermHistoryManager);

} // namespace ucair

#endif
