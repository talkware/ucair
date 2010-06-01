#include "search_topics.h"
#include <boost/pending/disjoint_sets.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/tuple/tuple.hpp>
#include "agglomerative_clustering.h"
#include "config.h"
#include "index_manager.h"
#include "logger.h"
#include "long_term_history_manager.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace ucair {

list<string> UserSearchTopic::getAllSortingCriteria() {
	static list<string> result;
	if (result.empty()) {
		result.push_back("session count");
		result.push_back("total query count");
		result.push_back("unique query count");
		result.push_back("total click count");
		result.push_back("unique click count");
	}
	return result;
}

double UserSearchTopic::getSortingScore(const string &criteria) {
	if (criteria == "session count") {
		return sessions.size();
	}
	else if (criteria == "total query count") {
		return searches.size();
	}
	else if (criteria == "unique query count") {
		return queries.size();
	}
	else if (criteria == "total click count") {
		return total_click_count;
	}
	else if (criteria == "unique click count") {
		return clicks.size();
	}
	else {
		return getSortingScore("session count");
	}
}

bool UserSearchTopicManager::initialize() {
	Main &main = Main::instance();
	agglomerative_clustering_stop_sim = util::getParam<double>(main.getConfig(), "agglomerative_clustering_stop_sim");
	join_sessions = util::getParam<bool>(main.getConfig(), "join_sessions");
	join_same_queries = util::getParam<bool>(main.getConfig(), "join_same_queries");
	nontrivial_topic_session_count = util::getParam<int>(main.getConfig(), "nontrivial_topic_session_count");
	return true;
}

bool UserSearchTopicManager::initializeUser(User &user) {
	all_user_search_topics.insert(make_pair(user.getUserId(), map<int, UserSearchTopic>()));
	loadSearchTopics(user, getLongTermHistoryManager().getConnection(user.getUserId()));
	return true;
}

map<int, UserSearchTopic>* UserSearchTopicManager::getAllSearchTopics(const string &user_id) {
	map<string, map<int, UserSearchTopic> >::iterator itr = all_user_search_topics.find(user_id);
	if (itr != all_user_search_topics.end()) {
		return &itr->second;
	}
	return NULL;
}

void UserSearchTopicManager::updateSearchTopics(const string &user_id) {
	User *user = getUserManager().getUser(user_id);
	assert(user);
	map<int, UserSearchTopic>* topics = getAllSearchTopics(user_id);
	assert(topics);
	topics->clear();
	agglomerativeCluster(*user, *topics);

	for (map<int, UserSearchTopic>::iterator itr = topics->begin(); itr != topics->end(); ++ itr) {
		computeProperties(*user, itr->second);
		testTrivial(*user, itr->second);
	}

	saveSearchTopics(*user, getLongTermHistoryManager().getConnection(user_id));
}

void UserSearchTopicManager::agglomerativeCluster(User &user, map<int, UserSearchTopic> &topics) {
	getLogger().debug("Preparing clustering");

	user.updateSearchIndices();

	const vector<string> &all_search_ids = user.getAllSearchIds();

	map<string, vector<int> > session_map;
	map<string, vector<int> > query_map;
	for (int i = 0; i < (int) all_search_ids.size(); ++ i) {
		const UserSearchRecord *search_record = user.getSearchRecord(all_search_ids[i]);
		assert(search_record);
		map<string, vector<int> >::iterator itr;
		tie(itr, tuples::ignore) = session_map.insert(make_pair(search_record->getSessionId(), vector<int>()));
		itr->second.push_back(i);
		tie(itr, tuples::ignore) = query_map.insert(make_pair(search_record->getQuery(), vector<int>()));
		itr->second.push_back(i);
	}

	typedef boost::associative_property_map< std::map<int, int> > PropertyMap;
	typedef boost::disjoint_sets<PropertyMap, PropertyMap> DSets;
	std::map<int, int> ranks;
	std::map<int, int> parents;
	DSets dsets(ranks, parents);

	for (int i = 0; i < (int) all_search_ids.size(); ++ i) {
		dsets.make_set(i);
	}

	if (join_sessions) {
		for (map<string, vector<int> >::const_iterator itr = session_map.begin(); itr != session_map.end(); ++ itr) {
			const vector<int> &v = itr->second;
			for (int i = 1; i < (int) v.size(); ++ i) {
				dsets.union_set(v[0], v[i]);
			}
		}
	}

	if (join_same_queries) {
		for (map<string, vector<int> >::const_iterator itr = query_map.begin(); itr != query_map.end(); ++ itr) {
			const vector<int> &v = itr->second;
			for (int i = 1; i < (int) v.size(); ++ i) {
				dsets.union_set(v[0], v[i]);
			}
		}
	}

	map<int, vector<int> > groups;
	for (int i = 0; i < (int) all_search_ids.size(); ++ i) {
		int j = dsets.find_set(i);
		map<int, vector<int> >::iterator itr;
		tie(itr, tuples::ignore) = groups.insert(make_pair(j, vector<int>()));
		itr->second.push_back(i);
	}

	vector<Cluster> init_clusters;
	init_clusters.reserve(groups.size());
	vector<map<int, double> > init_models;
	init_models.reserve(groups.size());
	for (map<int, vector<int> >::const_iterator itr = groups.begin(); itr != groups.end(); ++ itr) {
		init_clusters.push_back(Cluster());
		init_clusters.back().points = itr->second;
		init_models.push_back(map<int, double>());
		map<int, double> &init_model = init_models.back();
		BOOST_FOREACH(int i, itr->second) {
			const string &search_id = all_search_ids[i];
			map<int, double> model = user.getIndexedSearchModel(search_id);
			for (map<int, double>::const_iterator itr2 = model.begin(); itr2 != model.end(); ++ itr2) {
				map<int, double>::iterator itr3;
				tie(itr3, tuples::ignore) = init_model.insert(make_pair(itr2->first, 0.0));
				itr3->second += itr2->second;
			}
			// model is not normalized
		}
	}

	SimMatrix sim_matrix;
	for (int i = 0; i < (int) init_clusters.size(); ++ i) {
		for (int j = i + 1; j < (int) init_clusters.size(); ++ j) {
			double sim = getCosSim(init_models[i], init_models[j]);
			if (sim > 0.01) {
				sim_matrix.set(i, j, sim);
			}
		}
	}

	getLogger().debug("Clustering started");

	AgglomerativeClusteringMethod method(init_clusters, sim_matrix, agglomerative_clustering_stop_sim);
	vector<Cluster> clusters = method.run();

	getLogger().debug("Clustering finished");

	topics.clear();
	int topic_id = 0;
	BOOST_FOREACH(const Cluster &cluster, clusters) {
		++ topic_id;
		map<int, UserSearchTopic>::iterator itr;
		tie(itr, tuples::ignore) = topics.insert(make_pair(topic_id, UserSearchTopic(topic_id)));
		UserSearchTopic &topic = itr->second;
		BOOST_FOREACH(int i, cluster.points) {
			const string &search_id = all_search_ids[i];
			topic.searches[search_id] = 1.0;
			map<int, double> model = user.getIndexedSearchModel(search_id);
			for (map<int, double>::const_iterator itr2 = model.begin(); itr2 != model.end(); ++ itr2) {
				map<int, double>::iterator itr3;
				tie(itr3, tuples::ignore) = topic.model.insert(make_pair(itr2->first, 0.0));
				itr3->second += itr2->second;
			}
		}
		normalize(*indexing::ValueMap::from(topic.model));
	}
}

void UserSearchTopicManager::computeProperties(User &user, UserSearchTopic &topic) const {
	topic.total_click_count = 0;
	topic.sessions.clear();
	topic.queries.clear();
	topic.clicks.clear();

	for (map<string, double>::const_iterator itr_search = topic.searches.begin(); itr_search != topic.searches.end(); ++ itr_search) {
		const string &search_id = itr_search->first;
		const UserSearchRecord *search_record = user.getSearchRecord(search_id);
		assert(search_record);
		map<string, int>::iterator itr;
		tie(itr, tuples::ignore) = topic.sessions.insert(make_pair(search_record->getSessionId(), 0));
		++ itr->second;
		tie(itr, tuples::ignore) = topic.queries.insert(make_pair(search_record->getQuery(), 0));
		++ itr->second;
		BOOST_FOREACH(const shared_ptr<UserEvent> &event, search_record->getEvents()) {
			shared_ptr<ClickResultEvent> click_result_event = dynamic_pointer_cast<ClickResultEvent>(event);
			if (click_result_event){
				tie(itr, tuples::ignore) = topic.clicks.insert(make_pair(click_result_event->url, 0));
				++ itr->second;
				++ topic.total_click_count;
			}
		}
	}
}

void UserSearchTopicManager::testTrivial(User &user, UserSearchTopic &topic) const {
	topic.trivial = (int) topic.sessions.size() < nontrivial_topic_session_count;
}

void UserSearchTopicManager::sqlCreateTables(sqlite::Connection &conn) {
	conn.execute("CREATE TABLE topics(\
user_id TEXT NOT NULL,\
topic_id INTEGER NOT NULL,\
model TEXT NOT NULL)");
	conn.execute("CREATE INDEX topics_user_id ON topics(user_id)");
	conn.execute("CREATE TABLE topic_searches(\
user_id TEXT NOT NULL,\
topic_id INTEGER NOT NULL,\
search_id TEXT NOT NULL,\
weight REAL NOT NULL)");
	conn.execute("CREATE INDEX topic_searches_user_id ON topic_searches(user_id)");
	conn.execute("CREATE INDEX topic_searches_search_id ON topic_searches(search_id)");
	conn.execute("CREATE TABLE topic_attrs(\
user_id TEXT NOT NULL,\
topic_id INTEGER NOT NULL,\
name TEXT NOT NULL,\
value TEXT NOT NULL)");
	conn.execute("CREATE INDEX topic_attrs_user_id ON topic_attrs(user_id)");
}

void UserSearchTopicManager::saveSearchTopics(User &user, sqlite::Connection &conn) {
	try {
		sqlCreateTables(conn);
		getLogger().info("Created topic tables");
	}
	catch (sqlite::Error &) {
		// maybe table already exists
	}

	const map<int, UserSearchTopic>* topics = getAllSearchTopics(user.getUserId());
	assert(topics);

	getLogger().info("Saving topics");

	conn.beginTransaction();
	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("DELETE FROM topics WHERE user_id = ?");
		stmt->bind(1, user.getUserId());
		stmt->step();

		stmt = conn.prepare("DELETE FROM topic_searches WHERE user_id = ?");
		stmt->bind(1, user.getUserId());
		stmt->step();

		stmt = conn.prepare("DELETE FROM topic_attrs WHERE user_id = ?");
		stmt->bind(1, user.getUserId());
		stmt->step();

		for (map<int, UserSearchTopic>::const_iterator itr = topics->begin(); itr != topics->end(); ++ itr) {
			const UserSearchTopic &topic = itr->second;
			if (topic.trivial) {
				continue;
			}

			stmt = conn.prepare("INSERT INTO topics(user_id, topic_id, model) VALUES(?, ?, ?)");
			stmt->bind(1, user.getUserId());
			stmt->bind(2, topic.topic_id);

			vector<pair<string, double> > model_with_term_str;
			id2Name(*indexing::ValueMap::from(topic.model), model_with_term_str, getIndexManager().getTermDict());
			string model_str;
			toString(model_with_term_str, model_str, 3);
			stmt->bind(3, model_str);
			stmt->step();

			stmt = conn.prepare("INSERT INTO topic_searches(user_id, topic_id, search_id, weight) VALUES(?, ?, ?, ?)");
			typedef pair<string, double> P;
			BOOST_FOREACH(const P &p, topic.searches) {
				stmt->bind(1, user.getUserId());
				stmt->bind(2, topic.topic_id);
				stmt->bind(3, p.first);
				stmt->bind(4, p.second);
				stmt->step();
				stmt->reset();
			}
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
}

void UserSearchTopicManager::loadSearchTopics(User &user, sqlite::Connection &conn) {
	map<int, UserSearchTopic>* topics = getAllSearchTopics(user.getUserId());
	assert(topics);
	topics->clear();

	getLogger().info("Loading topics");
	try {
		sqlite::PreparedStatementPtr stmt = conn.prepare("SELECT topic_id, model FROM topics WHERE user_id = ?");
		stmt->bind(1, user.getUserId());
		while (stmt->step()) {
			int topic_id = stmt->getInt(0);
			map<int, UserSearchTopic>::iterator itr;
			tie(itr, tuples::ignore) = topics->insert(make_pair(topic_id, UserSearchTopic(topic_id)));
			UserSearchTopic &topic = itr->second;

			string model_str = stmt->getString(1);
			vector<pair<string, double> > model;
			fromString(model_str, model);
			name2Id(model, *indexing::ValueMap::from(topic.model), getIndexManager().getTermDict());
		}

		stmt = conn.prepare("SELECT topic_id, search_id, weight FROM topic_searches WHERE user_id = ?");
		stmt->bind(1, user.getUserId());
		while (stmt->step()) {
			int topic_id = stmt->getInt(0);
			map<int, UserSearchTopic>::iterator itr;
			tie(itr, tuples::ignore) = topics->insert(make_pair(topic_id, UserSearchTopic(topic_id)));
			UserSearchTopic &topic = itr->second;

			string search_id = stmt->getString(1);
			double weight = stmt->getDouble(2);
			topic.searches[search_id] = weight;
		}
	}
	catch (sqlite::Error &e) {
		if (const string* error_info = boost::get_error_info<sqlite::ErrorInfo>(e)){
			getLogger().error(*error_info);
		}
	}

	for (map<int, UserSearchTopic>::iterator itr = topics->begin(); itr != topics->end(); ++ itr) {
		computeProperties(user, itr->second);
	}
}

} // namespace ucair
