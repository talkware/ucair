#include "ucair_util.h"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tuple/tuple.hpp>
#include "common_util.h"
#include "index_util.h"
#include "search_engine.h"
#include "simple_index.h"

using namespace std;
using namespace boost;

namespace ucair {

bool isHTML(const string &text) {
	return text.find("</") != string::npos;
}

string stripHTML(const string &text) {
	string output;
	bool in_tag = false;
	for (int i = 0; i < (int) text.length(); ++ i) {
		if (text[i] == '<') {
			in_tag = true;
		}
		else if (text[i] == '>') {
			in_tag = false;
		}
		else if (! in_tag) {
			output.push_back(text[i]);
		}
	}
	return output;
}

void indexDocument(indexing::SimpleIndex &index, const Document &doc) {
	if (index.getDocDict().getId(doc.doc_id) > 0) { // already indexed
		return;
	}
	map<int, double> term_counts;
	indexing::countTerms(index.getTermDict(), doc.title + " " + doc.summary, term_counts);
	index.addDoc(doc.doc_id, *indexing::ValueMap::from(term_counts));
}

string buildDocName(const string &search_id, int result_pos){
	return str(format("%1%_%2%") % search_id % result_pos);
}

pair<string, int> parseDocName(const string &doc_name){
	string search_id;
	int result_pos = 0;
	size_t pos = doc_name.find('_');
	if (pos != string::npos){
		search_id = doc_name.substr(0, pos);
		try{
			result_pos = lexical_cast<int>(doc_name.substr(pos + 1));
		}
		catch (bad_lexical_cast &){
		}
	}
	return make_pair(search_id, result_pos);
}

void normalize(indexing::ValueMap &a, bool squared){
	shared_ptr<indexing::ValueIterator> itr = a.iterator();
	double s = 0.0;
	while (itr->ok()){
		s += squared ? itr->get() * itr->get() : itr->get();
		itr->next();
	}
	if (s > 0.0){
		if (squared){
			s = sqrt(s);
		}
		itr->reset();
		while (itr->ok()){
			itr->set(itr->get() / s);
			itr->next();
		}
	}
}

void interpolate(const indexing::ValueMap &a, const indexing::ValueMap &b, map<int, double> &c, double alpha){
	assert(alpha >= 0.0 && alpha <= 1.0);
	c.clear();
	shared_ptr<indexing::ConstValueIterator> itr1 = a.const_iterator();
	while (itr1->ok()){
		c.insert(make_pair(itr1->id(), itr1->get() * alpha));
		itr1->next();
	}
	shared_ptr<indexing::ConstValueIterator> itr2 = b.const_iterator();
	while (itr2->ok()){
		map<int, double>::iterator itr;
		tie(itr, tuples::ignore) = c.insert(make_pair(itr2->id(), 0.0));
		itr->second += itr2->get() * (1.0 - alpha);
		itr2->next();
	}
}

void sortValues(const indexing::ValueMap &a, vector<pair<int, double> > &b){
	b.clear();
	b.reserve(a.size());
	shared_ptr<indexing::ConstValueIterator> itr = a.const_iterator();
	while (itr->ok()){
		b.push_back(make_pair(itr->id(), itr->get()));
		itr->next();
	}
	sort(b.begin(), b.end(), util::cmp2ndReverse<int, double>);
}

void truncate(const indexing::ValueMap &a, vector<pair<int, double> > &b, int max_size, double min_value, double min_sum){
	sortValues(a, b);
	if (max_size > (int) b.size()){
		max_size = b.size();
	}
	int k = 0;
	double sum = 0.0;
	while (k < max_size && b[k].second >= min_value && sum < min_sum){
		sum += b[k].second;
		++ k;
	}
	b.resize(k);
}

void truncate(indexing::ValueMap &a, int max_size, double min_value, double min_sum){
	vector<pair<int, double> > b;
	truncate(a, b, max_size, min_value, min_sum);
	a.clear();
	typedef pair<int, double> P;
	BOOST_FOREACH(const P &p, b) {
		a.set(p.first, p.second);
	}
}

void id2Name(const indexing::ValueMap &m, vector<pair<string, double> > &l, indexing::NameDict &name_dict){
	l.clear();
	shared_ptr<indexing::ConstValueIterator> itr = m.const_iterator();
	while (itr->ok()){
		string s = name_dict.getName(itr->id());
		if (! s.empty()){
			l.push_back(make_pair(s, itr->get()));
		}
		itr->next();
	}
}

void name2Id(const vector<pair<string, double> > &l, indexing::ValueMap &m, indexing::NameDict &name_dict, bool add_id){
	m.clear();
	typedef pair<string, double> P;
	BOOST_FOREACH(const P &p, l){
		int id = name_dict.getId(p.first, add_id);
		m.set(id, p.second, false);
	}
}

void toString(const vector<pair<string, double> > &l, string &s, int precision){
	ostringstream strout;
	typedef pair<string, double> P;
	BOOST_FOREACH(const P &p, l){
		strout << p.first << "\t" << fixed << setprecision(precision) << p.second << "\n";
	}
	s = strout.str();
}

void fromString(const string &s, vector<pair<string, double> > &l){
	size_t pos1 = 0;
	while (true){
		size_t pos2 = s.find('\n', pos1);
		if (pos2 == string::npos){
			break;
		}
		size_t pos3 = s.find('\t', pos1);
		if (pos3 != string::npos && pos3 < pos2){
			try{
				l.push_back(make_pair(s.substr(pos1, pos3 - pos1), lexical_cast<double>(s.substr(pos3 + 1, pos2 - pos3 - 1))));
			}
			catch (bad_lexical_cast &){
			}
		}
		pos1 = pos2 + 1;
	}
}

string getDisplayUrl(const string &url, int truncate_size) {
	string display_url;
	size_t pos = url.find("://");
	if (pos != string::npos) {
		display_url = url.substr(pos + 3);
	}
	else {
		display_url = url;
	}
	int length = (int) display_url.length();
	if (length > 0 && display_url[length - 1] == '/') {
		-- length;
	}
	if (length > truncate_size) {
		return display_url.substr(0, truncate_size) + "...";
	}
	else {
		return display_url.substr(0, length);
	}
}

double getCosSim(const std::map<int, double> &a, const std::map<int, double> &b) {
	if (a.empty() || b.empty()) {
		return 0.0;
	}
	double sa = 0.0, sb = 0.0, sc = 0.0;
	map<int, double>::const_iterator itr1 = a.begin(), itr2 = b.begin();
	while (itr1 != a.end() && itr2 != b.end()) {
		if (itr1->first == itr2->first) {
			sa += itr1->second * itr1->second;
			sb += itr2->second * itr2->second;
			sc += itr1->second * itr2->second;
			++ itr1;
			++ itr2;
		}
		else if (itr1->first < itr2->first) {
			sa += itr1->second * itr1->second;
			++ itr1;
		}
		else {
			sb += itr2->second * itr2->second;
			++ itr2;
		}
	}
	for (; itr1 != a.end(); ++ itr1) {
		sa += itr1->second * itr1->second;
	}
	for (; itr2 != b.end(); ++ itr2) {
		sb += itr2->second * itr2->second;
	}
	if (sa == 0.0 || sb == 0.0) {
		return 0.0;
	}
	return sc / sqrt(sa * sb);
}

string getDateStr(const posix_time::ptime &t, bool useYesterdayToday) {
	if (useYesterdayToday) {
		posix_time::ptime now = posix_time::second_clock::local_time();
		int days_from_today = (now.date() - t.date()).days();
		if (days_from_today == 0) {
			return "Today";
		}
		else if (days_from_today == 1) {
			return "Yesterday";
		}
	}
	return util::timeToString(t, "%b %d, %Y");
}

string getTimeStr(const posix_time::ptime &t) {
	return util::timeToString(t, "%I:%M %p");
}

} // namespace ucair
