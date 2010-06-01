#include "common_util.h"
#include <cctype>
#include <locale>
#include <sstream>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

namespace util {

void readFile(ifstream &fin, string &content){
	content.clear();
	fin.seekg(0, ios::end);
	size_t length = fin.tellg();
	fin.seekg(0, ios::beg);
	content.reserve(length);
	while (true){
		int ch = fin.get();
		if (! fin){
			break;
		}
		content.push_back(ch);
	}
}

string makeId(){
	static int last_id = 0;
	return str(format("%1%.%2%") % time(NULL) % (++ last_id));
}

int cmpId(const string &a, const string &b){
	size_t pos1 = a.find('.');
	size_t pos2 = b.find('.');
	if (pos1 != string::npos && pos2 != string::npos){
		try {
			long long n1 = lexical_cast<long long>(a.substr(0, pos1));
			long long n2 = lexical_cast<long long>(b.substr(0, pos2));
			if (n1 == n2){
				n1 = lexical_cast<long long>(a.substr(pos1 + 1));
				n2 = lexical_cast<long long>(b.substr(pos2 + 1));
				return (int)(n1 - n2); // difference won't overflow normally
			}
			else{
				return (int)(n1 - n2);
			}
		}
		catch (bad_lexical_cast &){
		}
	}
	// ids were not from makeId()
	return a.compare(b);
}

string timeToString(const posix_time::ptime &t, const string &format){
	ostringstream strout;
	posix_time::time_facet *output_facet = new posix_time::time_facet;
	output_facet->format(format.c_str());
	strout.imbue(locale(locale::classic(), output_facet));
	strout << t;
	return strout.str();
}

posix_time::ptime stringToTime(const string &s, const string &format){
	istringstream strin(s);
	posix_time::time_input_facet *input_facet = new posix_time::time_input_facet;
	input_facet->format(format.c_str());
	strin.imbue(locale(locale::classic(), input_facet));
	posix_time::ptime t(posix_time::not_a_date_time);
	strin >> t;
	return t;
}

posix_time::ptime utc_to_local(const posix_time::ptime &utc_time) {
	typedef boost::date_time::c_local_adjustor<posix_time::ptime> local_adj;
	return local_adj::utc_to_local(utc_time);
}

posix_time::ptime local_to_utc(const posix_time::ptime &local_time) {
	// You would think there should be an equivalent local_to_utc,
	// but for some stupid reason it's missing in the boost library since 2005
	// http://lists.boost.org/boost-users/2005/07/12789.php
	// What the hell...
	// I'm skipping the implementation unless it's needed.
	assert(false);
	return posix_time::ptime();
	//typedef boost::date_time::c_local_adjustor<posix_time::ptime> local_adj;
	//return local_adj::local_to_utc(local_time);
}

time_t to_time_t(const posix_time::ptime &utc_time) {
	posix_time::ptime epoch(gregorian::date(1970,1,1));
	posix_time::time_duration diff = utc_time - epoch;
	return diff.total_seconds();
}

vector<string> tokenizeWithWhitespace(const string &text, bool quote_phrase) {
	vector<string> words;
	string word;
	// state 0: whitespace
	// state 1: inside word
	// state 2: inside quoted phrase
	int state = 0;
	for (int i = 0; i < (int) text.length(); ++ i) {
		char ch = text[i];
		if (state == 0) {
			if (quote_phrase && ch == '\"') {
				state = 2;
				word.push_back(ch);
			}
			else if (! isWhitespace(ch)) {
				state = 1;
				word.push_back(ch);
			}
		}
		else if (state == 1) {
			if (isWhitespace(ch)) {
				words.push_back(word);
				word.clear();
				state = 0;
			}
			else {
				word.push_back(ch);
			}
		}
		else if (state == 2) {
			if (ch == '\"') {
				word.push_back(ch);
				words.push_back(word);
				word.clear();
				state = 0;
			}
			else {
				word.push_back(ch);
			}
		}
	}
	if (! word.empty()) {
		words.push_back(word);
	}
	return words;
}

vector<string> tokenizeWithPunctuation(const std::string &text) {
	vector<string> words;
	string word;
	// state 0: punctuation
	// state 1: inside word
	int state = 0;
	for (int i = 0; i < (int) text.length(); ++ i) {
		char ch = text[i];
		if (state == 0) {
			if (ch < 0 || isalnum(ch)) {
				word.push_back(ch);
				state = 1;
			}
		}
		else if (state == 1) {
			if (ch < 0 || isalnum(ch)) {
				word.push_back(ch);
			}
			else {
				words.push_back(word);
				word.clear();
				state = 0;
			}
		}
	}
	if (! word.empty()) {
		words.push_back(word);
	}
	return words;
}

bool isASCIIPrintable(const string &s) {
	for (string::const_iterator itr = s.begin(); itr != s.end(); ++ itr){
		const char &ch = *itr;
		if (ch < 32){
			return false;
		}
	}
	return true;
}

} // namespace util
