#ifndef __log_importer_h__
#define __log_importer_h__

#include <ctime>
#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "search_engine.h"
#include "sqlitepp.h"
#include "user.h"

namespace ucair {

class ImportRecord {
public:
	std::string user_id;
	std::string search_id;
	std::string session_id;
	std::string query;
	time_t timestamp;
	std::map<int, double> model;
};

/*! \brief Library to import external search log into the sqlite database.
 *  You can either use run() on search logs having a particular format,
 *  or use the SAX-like API to work on custom log formats.
 */
class LogImporter : public Component {
public:
	~LogImporter();

	/// Creates a search id based on timestamp
	static std::string makeSearchId(time_t timestamp);

	/// Call this when beginning the log.
	void beginLog();
	/// Call this when ending the log.
	void endLog();

	/*! \brief Call this when beginning a search.
	 *
	 *  \param search_id search id
	 *  \param timestamp search time
	 *  \param query query
	 *  \param search_engine_id search engine id
	 */
	void beginSearch(const std::string &search_id, time_t &timestamp, const std::string &query, const std::string &search_engine_id);
	/// Call this when ending a search.
	void endSearch();

	/*! \brief Call this for each result in the search.
	 *
	 *  \param pos result start pos
	 *  \param title title
	 *  \param summary summary
	 *  \param url url
	 */
	void addResult(int pos, const std::string &title, const std::string &summary, const std::string &url);

	/*! \brief Call this for the event of user viewing a page.
	 *
	 *  \param timestamp view time
	 *  \param start_pos start pos of first result on page
	 *  \param result_count count of results on page
	 *  \param view_id view id
	 */
	void addViewPageEvent(time_t timestamp, int start_pos, int result_count, const std::string &view_id = "base");
	/*! \brief Call this for the event of user clicking on a result.
	 *
	 *  \param timestamp click time
	 *  \param pos start pos of clicked result
	 *  \param url url
	 */
	void addClickResultEvent(time_t timestamp, int pos, const std::string &url);

	/// Group searches to sessions (assign session id).
	void updateSessions();

	bool initialize();
	void run();

protected:
	void addEvent(const boost::shared_ptr<UserEvent> &event);
	void computeModel();

	boost::scoped_ptr<sqlite::Connection> conn;
	std::string search_id; ///< current search id
	boost::scoped_ptr<Search> search; ///< current search
	boost::scoped_ptr<UserSearchRecord> search_record; ///< current search record
	std::map<int, double> model; /// current search model

	std::string user_id; ///< user of the log
	std::string db_path; /// path of the sqlite database
	time_t session_expiration_time; /// session expiration time
	double min_session_sim; /// min similarity to be in same session

	// only used in run()
	std::string search_log_file_path; /// path of the search log
	std::string search_page_dir_path; /// path of the search pages
	std::string search_page_format; /// format of search pages
	std::string default_search_engine_id; /// default search engine id to use

	std::vector<ImportRecord> import_records;
};

} // namespace ucair

#endif
