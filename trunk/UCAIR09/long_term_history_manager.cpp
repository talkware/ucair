#include "long_term_history_manager.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "index_manager.h"
#include "logger.h"
#include "prototype.h"
#include "search_model.h"
#include "search_proxy.h"
#include "ucair_server.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

bool LongTermHistoryManager::initialize() {
	getUCAIRServer().idle_signal.sig.connect(1, bind(&LongTermHistoryManager::onIdle, this));
	return true;
}

bool LongTermHistoryManager::initializeUser(User &user) {
	filesystem::path path = getUserManager().getProfileDir(user.getUserId());
	path /= "long_term.db";
	bool database_exists = filesystem::exists(path);
	connections.insert(make_pair(user.getUserId(), shared_ptr<sqlite::Connection>(new sqlite::Connection(path.string()))));
	if (! database_exists){
		if (! createDatabase(user.getUserId())) {
			return false;
		}
	}
	addUserSaveTask(user.getUserId());
	loadHistory(user.getUserId());
	return true;
}

bool LongTermHistoryManager::finalizeUser(User &user) {
	string user_id = user.getUserId();
	/*saveHistory(user_id, true);
	for (map<string, SearchSaveTask>::iterator itr = search_save_tasks.begin(); itr != search_save_tasks.end();) {
		if (itr->second.user_id == user_id) {
			search_save_tasks.erase(itr ++);
		}
		else {
			++ itr;
		}
	}
	for (map<string, SearchLoadTask>::iterator itr = search_load_tasks.begin(); itr != search_load_tasks.end();) {
		if (itr->second.user_id == user_id) {
			search_load_tasks.erase(itr ++);
		}
		else {
			++ itr;
		}
	}*/
	//user_save_tasks.erase(user_id);
	//connections.erase(user_id);
	return true;
}

sqlite::Connection& LongTermHistoryManager::getConnection(const string &user_id) {
	map<string, shared_ptr<sqlite::Connection> >::iterator itr = connections.find(user_id);
	assert(itr != connections.end());
	return *itr->second;
}

void LongTermHistoryManager::sqlCreateDatabase(sqlite::Connection &conn) {
	conn.execute("CREATE TABLE searches(\
search_id TEXT PRIMARY KEY,\
timestamp INTEGER NOT NULL,\
query TEXT NOT NULL,\
search_engine TEXT NOT NULL,\
session_id TEXT NOT NULL)"
);
	conn.execute("CREATE TABLE search_attrs(\
search_id TEXT NOT NULL,\
name TEXT NOT NULL,\
value TEXT NOT NULL)"
);
	conn.execute("CREATE INDEX search_attrs_search_id ON search_attrs(search_id)");
	conn.execute("CREATE TABLE search_results(\
search_id TEXT NOT NULL,\
pos INTEGER NOT NULL,\
title TEXT NOT NULL,\
summary TEXT NOT NULL,\
url TEXT NOT NULL)"
);
	conn.execute("CREATE INDEX search_results_search_id ON search_results(search_id)");
	conn.execute("CREATE TABLE user_events(\
search_id TEXT NOT NULL,\
timestamp INTEGER NOT NULL,\
type TEXT NOT NULL,\
value TEXT NOT NULL)"
);
	conn.execute("CREATE INDEX user_events_search_id ON user_events(search_id)");
}

bool LongTermHistoryManager::createDatabase(const string &user_id){
	getLogger().info("Creating long-term history database");
	sqlite::Connection &conn = getConnection(user_id);
	try {
		sqlCreateDatabase(conn);
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		return false;
	}
	return true;
}

void LongTermHistoryManager::saveHistory(const string &user_id, bool final_call) {
	sqlite::Connection &conn = getConnection(user_id);
	for (map<string, SearchSaveTask>::iterator itr = search_save_tasks.begin(); itr != search_save_tasks.end();) {
		SearchSaveTask &search_save_task = itr->second;
		if (search_save_task.user_id == user_id) {
			search_save_task.saveAll(conn, final_call);
			if (search_save_task.isFinished()) {
				search_save_tasks.erase(itr ++);
				continue;
			}
		}
		++ itr;
	}
	map<string, UserSaveTask>::iterator itr = user_save_tasks.find(user_id);
	itr->second.saveAll(conn, final_call);
}

void LongTermHistoryManager::onIdle() {
	BOOST_FOREACH(const string &user_id, getUserManager().getAllUserIds()) {
		saveHistory(user_id, false);
	}
}

void LongTermHistoryManager::addSearchSaveTask(const string &user_id, const string &search_id) {
	search_save_tasks.insert(make_pair(search_id, SearchSaveTask(user_id, search_id)));
}

void LongTermHistoryManager::addUserSaveTask(const string &user_id) {
	user_save_tasks.insert(make_pair(user_id, UserSaveTask(user_id)));
}

const Search* LongTermHistoryManager::getSearch(const string &search_id, bool load_results) {
	map<std::string, SearchLoadTask>::iterator itr = search_load_tasks.find(search_id);
	if (itr == search_load_tasks.end()){
		return NULL;
	}
	SearchLoadTask &search_load_task = itr->second;
	if (load_results) {
		search_load_task.loadResults(getConnection(search_load_task.user_id));
	}
	return &search_load_task.search;
}

UserSearchRecord* LongTermHistoryManager::getSearchRecord(const string &search_id) {
	map<std::string, SearchLoadTask>::iterator itr = search_load_tasks.find(search_id);
	if (itr == search_load_tasks.end()){
		return NULL;
	}
	return itr->second.search_record.get();
}

const map<int, double>& LongTermHistoryManager::getModel(const string &search_id) const {
	map<std::string, SearchLoadTask>::const_iterator itr = search_load_tasks.find(search_id);
	assert(itr == search_load_tasks.end());
	return itr->second.model;
}

string LongTermHistoryManager::getFirstSearchId() const {
	map<std::string, SearchLoadTask>::const_iterator itr = search_load_tasks.begin();
	if (itr != search_load_tasks.end()) {
		return itr->first;
	}
	return "";
}

string LongTermHistoryManager::getLastSearchId() const {
	map<string, SearchLoadTask>::const_reverse_iterator itr = search_load_tasks.rbegin();
	if (itr != search_load_tasks.rend()) {
		return itr->first;
	}
	return "";
}

void LongTermHistoryManager::loadHistory(const string &user_id) {
	sqlite::Connection &conn = getConnection(user_id);
	loadSearches(conn, user_id);
	loadModels(conn);
	loadEvents(conn);
	buildIndex(user_id);
}

void LongTermHistoryManager::buildIndex(const string &user_id) {
	getLogger().info("Building long-term search index for user " + user_id);
	User *user = getUserManager().getUser(user_id);
	assert(user);

	user->long_term_search_index->clear();
	typedef pair<string, SearchLoadTask> P;
	BOOST_FOREACH(const P &p, search_load_tasks) {
		const SearchLoadTask &search_load_task = p.second;
		user->long_term_search_index->addDoc(search_load_task.search_id, *indexing::ValueMap::from(search_load_task.model));
	}
}

void LongTermHistoryManager::loadSearches(sqlite::Connection &conn, const string &user_id) {
	try {
		User* user = getUserManager().getUser(user_id);
		assert(user);

		getLogger().info("Loading searches");
		UserSearchRecord *prev_search_record = NULL;
		sqlite::PreparedStatementPtr stmt = conn.prepare("SELECT search_id, timestamp, query, search_engine, session_id FROM searches");
		while (stmt->step()) {
			string search_id = stmt->getString(0);
			time_t timestamp = (time_t) stmt->getLong(1);
			string query_text = stmt->getString(2);
			string search_engine_id = stmt->getString(3);
			string session_id = stmt->getString(4);

			map<string, SearchLoadTask>::iterator itr;
			tie(itr, tuples::ignore) = search_load_tasks.insert(make_pair(search_id, SearchLoadTask(user_id, search_id)));
			SearchLoadTask &search_load_task = itr->second;

			if (prev_search_record) {
				prev_search_record->next = search_load_task.search_record.get();
				search_load_task.search_record->prev = prev_search_record;
			}
			prev_search_record = search_load_task.search_record.get();

			Search &search = search_load_task.search;
			search.setSearchId(search_id);
			search.query.text = query_text;
			search.query.parseKeywords();
			search.setSearchEngineId(search_engine_id);

			search_load_task.search_record.reset(new UserSearchRecord(user_id, search_id, query_text, timestamp, true));
			search_load_task.search_record->setSessionId(session_id);

			user->all_search_ids.push_back(search_id);
		}
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		return;
	}
}

void LongTermHistoryManager::loadModels(sqlite::Connection &conn) {
	getLogger().info("Loading models");

	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("SELECT search_id, value FROM search_attrs WHERE name = 'model'");
		while (stmt->step()) {
			string search_id = stmt->getString(0);
			string model_str = stmt->getString(1);

			map<std::string, SearchLoadTask>::iterator itr = search_load_tasks.find(search_id);
			if (itr == search_load_tasks.end()){
				getLogger().error("Model found for missing search " + search_id);
				continue;
			}

			vector<pair<string, double> > model;
			fromString(model_str, model);
			name2Id(model, *indexing::ValueMap::from(itr->second.model), getIndexManager().getTermDict());
		}
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
	}
}

void LongTermHistoryManager::loadEvents(sqlite::Connection &conn) {
	getLogger().info("Loading events");

	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("SELECT search_id, timestamp, type, value FROM user_events");
		while (stmt->step()) {
			string search_id = stmt->getString(0);
			time_t timestamp = (time_t) stmt->getLong(1);
			string event_type = stmt->getString(2);
			string event_value = stmt->getString(3);

			map<std::string, SearchLoadTask>::iterator itr = search_load_tasks.find(search_id);
			if (itr == search_load_tasks.end()){
				getLogger().error("Event found for missing search " + search_id);
				continue;
			}
			UserSearchRecord &search_record = *(itr->second.search_record);

			shared_ptr<UserEvent> event = dynamic_pointer_cast<UserEvent>(util::PrototypedFactory::makeInstance(event_type));
			if (event) {
				event->search_id = search_id;
				event->timestamp = timestamp;
				event->loadValue(event_value);
				search_record.addEvent(event);
				do {
					shared_ptr<ClickResultEvent> click_result_event = dynamic_pointer_cast<ClickResultEvent>(event);
					if (click_result_event){
						search_record.addClickedResult(click_result_event->result_pos);
						break;
					}
					shared_ptr<RateResultEvent> rate_result_event = dynamic_pointer_cast<RateResultEvent>(event);
					if (rate_result_event) {
						search_record.setResultRating(rate_result_event->result_pos, rate_result_event->rating);
						break;
					}
				} while (false);
			}
		}
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

SearchSaveTask::SearchSaveTask(const string &user_id_, const std::string &search_id_) :
	user_id(user_id_),
	search_id(search_id_),
	query_saved(false),
	model_saved(false) {
}

void SearchSaveTask::saveAll(sqlite::Connection &conn, bool final_call) {
	saveQuery(conn);
	saveResults(conn);
	saveModel(conn, final_call);
}

bool SearchSaveTask::isFinished() const {
	if (query_saved && model_saved) {
		return true;
	}
	return false;
}

void SearchSaveTask::saveQuery(sqlite::Connection &conn) {
	if (query_saved) {
		return;
	}

	getLogger().info("Saving query for search " + search_id);

	const Search *search = getSearchProxy().getSearch(search_id);
	assert(search);
	User *user = getUserManager().getUser(user_id);
	assert(user);
	UserSearchRecord *search_record = user->getSearchRecord(search_id);
	assert(search_record);

	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare(
				"INSERT INTO searches(search_id, timestamp, query, search_engine, session_id) VALUES(?, ?, ?, ?, ?)");
		stmt->bind(1, search_id);
		stmt->bind(2, (long long) search_record->getCreationTime());
		stmt->bind(3, search->query.text);
		stmt->bind(4, search->getSearchEngineId());
		stmt->bind(5, search_record->getSessionId());
		stmt->step();
		query_saved = true;
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
	}
}

void SearchSaveTask::saveResults(sqlite::Connection &conn) {
	const Search *search = getSearchProxy().getSearch(search_id);
	assert(search);

	User *user = getUserManager().getUser(user_id);
	assert(user);
	UserSearchRecord *search_record = user->getSearchRecord(search_id);
	assert(search_record);

	set<int> results_to_save;
	for (int i = 1; i <= 10 && i <= (int) search->results.size(); ++ i) {
		results_to_save.insert(i);
	}
	const util::UniqueList &viewed_results = search_record->getViewedResults();
	copy(viewed_results.begin(), viewed_results.end(), inserter(results_to_save, results_to_save.end()));

	if (results_to_save.size() <= results_saved.size()) {
		return;
	}

	getLogger().info("Saving results for search " + search_id);

	conn.beginTransaction();
	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare(
				"INSERT INTO search_results(search_id, pos, title, summary, url) VALUES(?, ?, ?, ?, ?)");
		BOOST_FOREACH(int result_pos, results_to_save) {
			if (results_saved.find(result_pos) != results_saved.end()) {
				continue;
			}
			const SearchResult *result = search->getResult(result_pos);
			assert(result);
			stmt->bind(1, search_id);
			stmt->bind(2, result_pos);
			stmt->bind(3, result->title);
			stmt->bind(4, result->summary);
			stmt->bind(5, result->url);
			stmt->step();
			stmt->reset(true);
		}
	}
	catch (sqlite::Error &e) {
		conn.rollback();
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		return;
	}
	conn.commit();

	BOOST_FOREACH(int result_pos, results_to_save) {
		results_saved.insert(result_pos);
	}
}

void SearchSaveTask::saveModel(sqlite::Connection &conn, bool final_call) {
	if (model_saved) {
		return;
	}

	User *user = getUserManager().getUser(user_id);
	assert(user);
	const UserSearchRecord *search_record = user->getSearchRecord(search_id);
	assert(search_record);
	const Search *search = getSearchProxy().getSearch(search_id);
	assert(search);

	if (! final_call && ! user->isSearchExpired(search_id)) {
		return;
	}

	getLogger().info("Saving model for search " + search_id);

	map<int, double> model_with_term_id = getSearchModelManager().getModel(*search_record, *search, "single-search").probs;
	vector<pair<string, double> > model_with_term_str;
	id2Name(*indexing::ValueMap::from(model_with_term_id), model_with_term_str, getIndexManager().getTermDict());
	string model_str;
	toString(model_with_term_str, model_str, 3);
	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("INSERT INTO search_attrs(search_id, name, value) VALUES (?, ?, ?)");
		stmt->bind(1, search_id);
		stmt->bind(2, "model");
		stmt->bind(3, model_str);
		stmt->step();

		stmt = conn.prepare("UPDATE searches SET session_id = ? WHERE search_id = ?");
		stmt->bind(1, search_record->getSessionId());
		stmt->bind(2, search_id);
		stmt->step();
	}
	catch (sqlite::Error &e) {
		conn.rollback();
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		return;
	}
	model_saved = true;
}

////////////////////////////////////////////////////////////////////////////////

UserSaveTask::UserSaveTask(const string &user_id_) :
	user_id(user_id_),
	max_event_id_saved(0) {
}

void UserSaveTask::saveAll(sqlite::Connection &conn, bool final_call) {
	saveEvents(conn);
}

void UserSaveTask::saveEvents(sqlite::Connection &conn) {
	User *user = getUserManager().getUser(user_id);
	assert(user);

	list<shared_ptr<UserEvent> > events_to_save;
	BOOST_REVERSE_FOREACH(const shared_ptr<UserEvent> &event, user->getEvents()) {
		if (event->event_id <= max_event_id_saved) {
			break;
		}
		events_to_save.push_front(event);
	}

	if (events_to_save.empty()) {
		return;
	}

	getLogger().info("Saving events for user " + user_id);

	conn.beginTransaction();
	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("INSERT INTO user_events(search_id, timestamp, type, value) VALUES(?, ?, ?, ?)");
		BOOST_FOREACH(const shared_ptr<UserEvent> &event, events_to_save) {
			stmt->bind(1, event->search_id);
			stmt->bind(2, (long long) event->timestamp);
			stmt->bind(3, event->getType());
			stmt->bind(4, event->saveValue());
			stmt->step();
			stmt->reset(true);
		}
	}
	catch (sqlite::Error &e) {
		conn.rollback();
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		return;
	}
	conn.commit();

	max_event_id_saved = events_to_save.back()->event_id;
}

////////////////////////////////////////////////////////////////////////////////

SearchLoadTask::SearchLoadTask(const string &user_id_, const string &search_id_) :
	user_id(user_id_),
	search_id(search_id_),
	results_loaded(false) {
}

void SearchLoadTask::loadResults(sqlite::Connection &conn) {
	if (results_loaded) {
		return;
	}

	getLogger().info("Loading results for search " + search_id);

	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("SELECT pos, title, summary, url FROM search_results WHERE search_id = ?");
		stmt->bind(1, search_id);
		while (stmt->step()) {
			SearchResult result;
			result.search_id = search.getSearchId();
			result.original_rank = stmt->getInt(0);
			result.doc_id = buildDocName(result.search_id, result.original_rank);
			result.title = stmt->getString(1);
			result.summary = stmt->getString(2);
			result.url = stmt->getString(3);
			search.results.insert(make_pair(result.original_rank, result));
		}
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
		return;
	}
	results_loaded = true;
}

} // namespace ucair
