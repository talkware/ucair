#include "log_importer.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/regex.hpp>
#include <boost/tuple/tuple.hpp>
#include "common_util.h"
#include "config.h"
#include "index_manager.h"
#include "index_util.h"
#include "long_term_history_manager.h"
#include "search_model.h"
#include "ucair_util.h"
#include "user_manager.h"
#include "yahoo_boss_api.h"
#include "yahoo_search_api.h"

using namespace std;
using namespace boost;

namespace ucair {

bool LogImporter::initialize() {
	Main &main = Main::instance();
	user_id = util::getParam<string>(main.getConfig(), "log_importer_user_id");
	db_path = util::getParam<string>(main.getConfig(), "log_importer_db_path");
	search_log_file_path = util::getParam<string>(main.getConfig(), "log_importer_search_log_file_path");
	search_page_dir_path = util::getParam<string>(main.getConfig(), "log_importer_search_page_dir_path");
	search_page_format = util::getParam<string>(main.getConfig(), "log_importer_search_page_format");
	default_search_engine_id = util::getParam<string>(main.getConfig(), "log_importer_default_search_engine_id");
	session_expiration_time = util::getParam<time_t>(main.getConfig(), "session_expiration");
	min_session_sim = util::getParam<double>(main.getConfig(), "min_session_sim");
	return true;
}

LogImporter::~LogImporter() {
	endLog();
}

string LogImporter::makeSearchId(time_t timestamp) {
	static int last_id = 0;
	return str(format("%1%.%2%") % timestamp % (++ last_id));
}

void LogImporter::beginLog() {
	bool database_exists = filesystem::exists(db_path);
	conn.reset(new sqlite::Connection(db_path));
	if (! database_exists){
		LongTermHistoryManager::sqlCreateDatabase(*conn);
	}
}

void LogImporter::endLog() {
	if (conn) {
		conn.reset();
	}
}

void LogImporter::beginSearch(const string &search_id_, time_t &timestamp, const string &query, const string &search_engine_id) {
	search_id = search_id_;
	search.reset(new Search);
	search->setSearchId(search_id);
	search->query.text = query;
	search->query.parseKeywords();
	search->setSearchEngineId(search_engine_id);
	search_record.reset(new UserSearchRecord(user_id, search_id, query, timestamp, true));
}

void LogImporter::endSearch() {
	if (search_id.empty()) {
		return;
	}

	if (util::isASCIIPrintable(search->query.text)) {
		for (map<int, SearchResult>::const_iterator itr = search->results.begin(); itr != search->results.end(); ++ itr) {
			indexDocument(*search_record->getIndex(), itr->second);
		}
	}

	conn->beginTransaction();

	sqlite::PreparedStatementPtr stmt = conn->prepare(
			"INSERT INTO searches(search_id, timestamp, query, search_engine, session_id) VALUES(?, ?, ?, ?, ?)");
	stmt->bind(1, search_id);
	stmt->bind(2, (long long) search_record->getCreationTime());
	stmt->bind(3, search->query.text);
	stmt->bind(4, search->getSearchEngineId());
	stmt->bind(5, search_record->getSessionId());
	stmt->step();

	stmt = conn->prepare(
			"INSERT INTO search_results(search_id, pos, title, summary, url) VALUES(?, ?, ?, ?, ?)");
	typedef pair<int, SearchResult> P;
	BOOST_FOREACH(const P &p, search->results) {
		stmt->bind(1, search_id);
		stmt->bind(2, p.first);
		stmt->bind(3, p.second.title);
		stmt->bind(4, p.second.summary);
		stmt->bind(5, p.second.url);
		stmt->step();
		stmt->reset();
	}

	stmt = conn->prepare("INSERT INTO user_events(search_id, timestamp, type, value) VALUES(?, ?, ?, ?)");
	BOOST_FOREACH(const shared_ptr<UserEvent> &event, search_record->getEvents()) {
		stmt->bind(1, search_id);
		stmt->bind(2, (long long) event->timestamp);
		stmt->bind(3, event->getType());
		stmt->bind(4, event->saveValue());
		stmt->step();
		stmt->reset(true);
	}

	computeModel();
	vector<pair<string, double> > model_with_term_str;
	id2Name(*indexing::ValueMap::from(model), model_with_term_str, getIndexManager().getTermDict());
	string model_str;
	toString(model_with_term_str, model_str, 3);

	stmt = conn->prepare("INSERT INTO search_attrs(search_id, name, value) VALUES (?, ?, ?)");
	stmt->bind(1, search_id);
	stmt->bind(2, "model");
	stmt->bind(3, model_str);
	stmt->step();

	conn->commit();

	import_records.push_back(ImportRecord());
	ImportRecord &import_record = import_records.back();
	import_record.search_id = search_id;
	import_record.user_id = user_id;
	import_record.session_id = search_id;
	import_record.query = search_record->getQuery();
	import_record.timestamp = search_record->getCreationTime();
	import_record.model = model;

	search_id.clear();
}

void LogImporter::addResult(int pos, const string &title, const string &summary, const string &url) {
	map<int, SearchResult>::iterator itr;
	tie(itr, tuples::ignore) = search->results.insert(make_pair(pos, SearchResult()));
	SearchResult &result = itr->second;
	result.search_id = search->getSearchId();
	result.original_rank = pos;
	result.doc_id = buildDocName(result.search_id, result.original_rank);
	result.title = title;
	result.summary = summary;
	result.url = url;
}

void LogImporter::addEvent(const shared_ptr<UserEvent> &event) {
	event->search_id = search_id;
	search_record->addEvent(event);
}

void LogImporter::addViewPageEvent(time_t timestamp, int start_pos, int result_count, const string &view_id) {
	shared_ptr<ViewSearchPageEvent> event(new ViewSearchPageEvent);
	event->timestamp = timestamp;
	event->start_pos = start_pos;
	event->result_count = result_count;
	event->view_id = view_id;
	addEvent(event);
}

void LogImporter::addClickResultEvent(time_t timestamp, int pos, const string &url) {
	shared_ptr<ClickResultEvent> event(new ClickResultEvent);
	event->timestamp = timestamp;
	event->result_pos = pos;
	event->url = url;
	addEvent(event);
}

void LogImporter::computeModel() {
	SearchModelGen *model_gen = getSearchModelManager().getModelGen("short-term");
	assert(model_gen);
	model = model_gen->getModel(*search_record, *search).probs;
}

void LogImporter::run() {
	ifstream fin(search_log_file_path.c_str());
	if (! fin) {
		cerr << "Cannot open " << search_log_file_path << endl;
		exit(1);
	}
	if (! filesystem::exists(filesystem::path(search_page_dir_path))) {
		cerr << "Directory does not exist: " << search_page_dir_path << endl;
	}

	try {
		beginLog();
		string line;
		int line_no = 0;
		while (getline(fin, line)) {
			++ line_no;
			if (line.empty() || starts_with(line, "#")) {
				continue;
			}

			static const regex re1("(\\d+)\tquery\t(.*)");
			static const regex re2("(\\d+)\tclick\t(\\d+)");

			smatch results;
			if (regex_match(line, results, re1)) {
				time_t timestamp = lexical_cast<time_t>(results[1]);
				string query = results[2];
				string search_id = makeSearchId(timestamp);
				cout << timestamp << endl;

				string search_page_file = (filesystem::path(search_page_dir_path) / string(results[1])).string();
				ifstream fin2(search_page_file.c_str());
				if (! fin2) {
					cerr << "Cannot open " << search_page_file << endl;
					continue;
				}

				endSearch();
				beginSearch(search_id, timestamp, query, default_search_engine_id);

				string page_content;
				util::readFile(fin2, page_content);
				Search search;
				bool parsing_ok;
				if (search_page_format == "boss") {
					parsing_ok = YahooBossAPI::parsePage(search, page_content);
				}
				else if (search_page_format == "yahoo") {
					parsing_ok = YahooSearchAPI::parsePage(search, page_content);
				}
				else {
					cerr << "Valid search page formats are boss|yahoo" << endl;
					exit(1);
				}
				if (! parsing_ok) {
					cerr << "Failed to parse " << search_page_file << endl;
					exit(1);
				}
				typedef pair<int, SearchResult> P;
				BOOST_FOREACH(const P &p, search.results) {
					addResult(p.first, p.second.title, p.second.summary, p.second.url);
				}
			}
			else if (regex_match(line, results, re2)) {
				time_t timestamp = lexical_cast<time_t>(results[1]);
				int click_pos = lexical_cast<int>(results[2]);
				string url;
				const SearchResult *result = search->getResult(click_pos);
				if (result) {
					url = result->url;
				}
				addClickResultEvent(timestamp, click_pos, url);
			}
			else {
				cerr << str(format("Parsing error in line %1%") % line_no) << endl;
				exit(1);
			}
		}
		endSearch();

		updateSessions();
	}
	catch (sqlite::Error &e) {
		cerr << "Database error:" << endl;
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			cerr << *error_info << endl;
		}
		exit(1);
	}
}

void LogImporter::updateSessions() {
	typedef boost::associative_property_map< std::map<int, int> > PropertyMap;
	typedef boost::disjoint_sets<PropertyMap, PropertyMap> DSets;
	std::map<int, int> ranks;
	std::map<int, int> parents;
	DSets dsets(ranks, parents);

	for (int i = 0; i < (int) import_records.size(); ++ i){
		dsets.make_set(i);
	}

	for (int i = 0; i < (int) import_records.size(); ++ i) {
		for (int j = i + 1; j < (int) import_records.size(); ++ j) {
			if (import_records[j].timestamp > import_records[i].timestamp + session_expiration_time) {
				break;
			}
			if (ucair::getCosSim(import_records[i].model, import_records[j].model) >= min_session_sim) {
				dsets.union_set(i, j);
			}
		}
	}

	map<int, list<int> > groups;
	for (int i = 0; i < (int) import_records.size(); ++ i) {
		int j = dsets.find_set(i);
		map<int, list<int> >::iterator itr;
		tie(itr, tuples::ignore) = groups.insert(make_pair(j, list<int>()));
		itr->second.push_back(i);
	}

	for (map<int, list<int> >::const_iterator itr = groups.begin(); itr != groups.end(); ++ itr) {
		assert(! itr->second.empty());
		int min = -1;
		BOOST_FOREACH(int i, itr->second) {
			if (min == -1 || i < min) {
				min = i;
			}
		}
		BOOST_FOREACH(int i, itr->second) {
			import_records[i].session_id = import_records[min].search_id;
		}
	}

	sqlite::PreparedStatementPtr stmt = conn->prepare("UPDATE searches SET session_id = ? WHERE search_id = ?");
	BOOST_FOREACH(const ImportRecord &import_record, import_records) {
		stmt->bind(1, import_record.session_id);
		stmt->bind(2, import_record.search_id);
		stmt->step();
		stmt->reset();
	}
}

} // namespace ucair
