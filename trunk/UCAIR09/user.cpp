#include "user.h"
#include <algorithm>
#include <cassert>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include "common_util.h"
#include "config.h"
#include "index_manager.h"
#include "logger.h"
#include "long_term_history_manager.h"
#include "search_model.h"
#include "search_proxy.h"
#include "ucair_util.h"
#include "user_manager.h"

using namespace std;
using namespace boost;

namespace {
// I hate to put them in UserManager or as static variables.
time_t search_expiration_time = 0;
time_t session_expiration_time = 0;
double min_session_sim = 0.0;
double dir_prior = 1.0;
}

namespace ucair {

User::User(const string &user_id_): user_id(user_id_), index_outdated(true) {
	short_term_search_index.reset(new indexing::SimpleIndex(getIndexManager().getTermDict()));
	long_term_search_index.reset(new indexing::SimpleIndex(getIndexManager().getTermDict()));
	static bool loadConfig = true;
	if (loadConfig) {
		// one time
		search_expiration_time = util::getParam<time_t>(Main::instance().getConfig(), "search_expiration");
		session_expiration_time = util::getParam<time_t>(Main::instance().getConfig(), "session_expiration");
		min_session_sim = util::getParam<double>(Main::instance().getConfig(), "min_session_sim");
		dir_prior = util::getParam<double>(Main::instance().getConfig(), "search_model_dir_prior");
		loadConfig = false;
	}
}

void User::addEvent(const shared_ptr<UserEvent> &event){
	UserEvent::insertByTimestamp(short_term_events, event);
	if (! event->search_id.empty()){
		UserSearchRecord *search_record = getSearchRecord(event->search_id);
		if (search_record){
			search_record->addEvent(event);
		}
	}
	// Fire event.
	getUserManager().user_event_signal(*this, *event);
	index_outdated = true;
}

const list<shared_ptr<UserEvent> >& User::getEvents() const {
	return short_term_events;
}

UserSearchRecord* User::getSearchRecord(const string &search_id) {
	map<std::string, UserSearchRecord>::iterator itr = short_term_search_records.find(search_id);
	if (itr != short_term_search_records.end()){
		return &itr->second;
	}
	return getLongTermHistoryManager().getSearchRecord(search_id);
}

const UserSearchRecord* User::getSearchRecord(const string &search_id) const {
	map<std::string, UserSearchRecord>::const_iterator itr = short_term_search_records.find(search_id);
	if (itr != short_term_search_records.end()){
		return &itr->second;
	}
	return getLongTermHistoryManager().getSearchRecord(search_id);
}

UserSearchRecord* User::addSearchRecord(const string &search_id) {
	const Search *search = getSearchProxy().getSearch(search_id);
	assert(search);

	const string &query = search->query.text;
	time_t creation_time = time(NULL);

	map<string, UserSearchRecord>::iterator itr;
	tie(itr, tuples::ignore) = short_term_search_records.insert(make_pair(search_id, UserSearchRecord(user_id, search_id, query, creation_time, false)));
	UserSearchRecord &search_record = itr->second;

	if (! all_search_ids.empty()) {
		UserSearchRecord *prev_search_record = getSearchRecord(all_search_ids.back());
		prev_search_record->next = &search_record;
		search_record.prev = prev_search_record;
	}
	all_search_ids.push_back(search_id);

	getUserManager().search2user[search_id] = user_id;
	index_outdated = true;
	getLongTermHistoryManager().addSearchSaveTask(user_id, search_id);

	return &search_record;
}

bool User::isSearchExpired(const string &search_id) const {
	const UserSearchRecord *search_record = getSearchRecord(search_id);
	if (! search_record) {
		return false;
	}
	if (! search_record->isFromPastHistory()){
		time_t now = time(NULL);
		if (now - search_record->getLastEventTime() < search_expiration_time){
			return false;
		}
	}
	return true;
}

string User::getQuery(const string &search_id) const {
	const Search* search = getSearch(search_id);
	if (search) {
		return search->query.text;
	}
	return "";
}

const Search* User::getSearch(const string &search_id) const {
	// Short-term searches can be found at SearchProxy.
	const Search* search = getSearchProxy().getSearch(search_id);
	if (search) {
		return search;
	}
	// Long-term ones need to be looked up in LongTermHistoryManager.
	return getLongTermHistoryManager().getSearch(search_id);
}

void User::updateSearchIndices(bool force_update) {
	if (! index_outdated && ! force_update) {
		return;
	}
	// Because the search models can change, it's easier for short_term_search_index to be rebuilt from scratch.
	short_term_search_index->clear();
	typedef pair<string, UserSearchRecord> P;
	BOOST_FOREACH(const P &p, short_term_search_records) {
		const string &search_id = p.first;
		const UserSearchRecord &search_record = p.second;
		const Search *search = getSearchProxy().getSearch(search_id);
		map<int, double> model = getSearchModelManager().getModel(search_record, *search, "short-term").probs;
		if (! isSearchExpired(search_id)) {
			// Search model may change in the future.
			short_term_search_index->addDoc(search_id, *indexing::ValueMap::from(model));
		}
		else if (long_term_search_index->getDocDict().getId(search_id) != -1) {
			// Search is expired, so model won't change. Only add it when not found.
			long_term_search_index->addDoc(search_id, *indexing::ValueMap::from(model));
		}
	}
	index_outdated = false;
}

vector<pair<string, double> > User::searchInHistory(const indexing::ValueMap &query_terms) {
	updateSearchIndices();
	// Search both short-term and long-term.
	vector<pair<string, double> > search_scores;
	indexing::SimpleKLRetriever retriever(*indexing::ValueMap::from(getIndexManager().getColProbs()), getIndexManager().getDefaultColProb(), dir_prior);
	vector<pair<int, double> > scores;
	retriever.retrieve(*short_term_search_index, query_terms, scores);
	typedef pair<int, double> P;
	BOOST_FOREACH(const P &p, scores) {
		string search_id = short_term_search_index->getDocDict().getName(p.first);
		search_scores.push_back(make_pair(search_id, p.second));
	}
	retriever.retrieve(*long_term_search_index, query_terms, scores);
	BOOST_FOREACH(const P &p, scores) {
		string search_id = long_term_search_index->getDocDict().getName(p.first);
		search_scores.push_back(make_pair(search_id, p.second));
	}
	sort(search_scores.begin(), search_scores.end(), util::cmp2ndReverse<string, double>);
	return search_scores;
}

map<int, double> User::getIndexedSearchModel(const string &search_id) {
	updateSearchIndices();
	map<int, double> result;
	const vector<pair<int, float> > *term_list = NULL;
	int doc_id = long_term_search_index->getDocDict().getId(search_id);
	if (doc_id > 0) {
		term_list = long_term_search_index->getTermList(doc_id);
	}
	else {
		doc_id = short_term_search_index->getDocDict().getId(search_id);
		if (doc_id > 0) {
			term_list = short_term_search_index->getTermList(doc_id);
		}
	}
	if (term_list) {
		typedef pair<int, float> P;
		BOOST_FOREACH(const P &p, *term_list) {
			result[p.first] = p.second;
		}
	}
	return result;
}

string User::getShortTermFirstSearchId() const {
	map<string, UserSearchRecord>::const_iterator itr = short_term_search_records.begin();
	if (itr != short_term_search_records.end()) {
		return itr->first;
	}
	return "";
}

string User::getShortTermLastSearchId() const {
	map<string, UserSearchRecord>::const_reverse_iterator itr = short_term_search_records.rbegin();
	if (itr != short_term_search_records.rend()) {
		return itr->first;
	}
	return "";
}

set<string> User::getSession(const std::string &session_id) const {
	// Session id equals the search id of the first search in the session.
	const UserSearchRecord *search_record = getSearchRecord(session_id);
	assert(search_record);
	set<string> session;
	session.insert(session_id);
	// A session ends when there is no activity for some time.
	// Thus we only need to scan a small number of searches.
	time_t session_last_activity_time = search_record->getLastEventTime();
	while (true) {
		 search_record = search_record->getNextSearchRecord();
		 if (! search_record) {
			 break;
		 }
		 if (search_record->getCreationTime() - session_last_activity_time > session_expiration_time) {
			 break;
		 }
		 if (search_record->getSessionId() == session_id) {
			 session.insert(search_record->getSearchId());
			 session_last_activity_time = search_record->getLastEventTime();
		 }
	}
	return session;
}

set<string> User::getTimeBasedSession(const std::string &this_search_id) const {
	const UserSearchRecord *this_search_record = getSearchRecord(this_search_id);
	assert(this_search_record);
	set<string> session;
	session.insert(this_search_id);

	const UserSearchRecord *search_record = this_search_record;
	while (true) {
		 search_record = search_record->getNextSearchRecord();
		 if (! search_record) {
			 break;
		 }
		 if (search_record->getCreationTime() - this_search_record->getLastEventTime() > session_expiration_time) {
			 break;
		 }
		 session.insert(search_record->getSearchId());
	}

	search_record = this_search_record;
	while (true) {
		 search_record = search_record->getPrevSearchRecord();
		 if (! search_record) {
			 break;
		 }
		 if (this_search_record->getCreationTime() - search_record->getLastEventTime() > session_expiration_time) {
			 break;
		 }
		 session.insert(search_record->getSearchId());
	}
	return session;
}

void User::updateSession(const std::string &this_search_id) {
	UserSearchRecord *this_search_record = getSearchRecord(this_search_id);
	assert(this_search_record);
	map<int, double> this_model = getIndexedSearchModel(this_search_id);

	set<string> old_session_ids;
	old_session_ids.insert(this_search_record->getSessionId());

	UserSearchRecord *search_record = this_search_record;
	while (true) {
		search_record = search_record->getNextSearchRecord();
		if (! search_record) {
			break;
		}
		if (search_record->getCreationTime() - this_search_record->getCreationTime() > session_expiration_time) {
			break;
		}
		if (old_session_ids.find(search_record->getSessionId()) == old_session_ids.end()) {
			map<int, double> model = getIndexedSearchModel(search_record->getSearchId());
			if (getCosSim(this_model, model) >= min_session_sim) {
				old_session_ids.insert(search_record->getSessionId());
			}
		}
	}

	search_record = this_search_record;
	string short_term_first_search_id = getShortTermFirstSearchId();
	while (true) {
		if (search_record->getSearchId() == short_term_first_search_id) {
			break;
		}
		search_record = search_record->getPrevSearchRecord();
		if (! search_record) {
			break;
		}
		if (this_search_record->getCreationTime() - search_record->getCreationTime() > session_expiration_time) {
			break;
		}
		if (old_session_ids.find(search_record->getSessionId()) == old_session_ids.end()) {
			map<int, double> model = getIndexedSearchModel(search_record->getSearchId());
			if (getCosSim(this_model, model) >= min_session_sim) {
				old_session_ids.insert(search_record->getSessionId());
			}
		}
	}

	string new_session_id = *old_session_ids.begin();
	BOOST_FOREACH(const string &session_id, old_session_ids) {
		if (session_id != new_session_id) {
			set<string> search_ids = getSession(session_id);
			BOOST_FOREACH(const string &search_id, search_ids) {
				UserSearchRecord *search_record = getSearchRecord(search_id);
				search_record->setSessionId(new_session_id);
			}
		}
	}
}

} // namespace ucair
