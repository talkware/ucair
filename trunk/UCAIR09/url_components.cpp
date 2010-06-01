#include "url_components.h"
#include <boost/regex.hpp>

using namespace std;
using namespace boost;

namespace http {
namespace util {

URLComponents::URLComponents(const string &_url): url(_url), query_params_modified(false) {
	// This regular expression could be stricter.
	static const regex re(
		"("
			"(([^:/]+)://)?"	// scheme
			"([^:/]+)"			// host
			"(:(\\d+))?"		// port
		")?"
		"("						// path_query_fragment
			"(/[^?#]+)"			// path
			"(\\?([^#]+))?"		// query
			"(#(.*))?"			// fragment
		")?"
	);
	smatch results;
	if (regex_match(url, results, re)){
		scheme = results[3];
		host = results[4];
		port = results[6];
		path_query_fragment = results[7];
		path = results[8];
		query = results[10];
		fragment = results[12];
	}
	parseQuery(query, query_params);
}

string URLComponents::rebuildURL() const {
	string url;
	if (! host.empty()){
		if (! scheme.empty()){
			url = scheme + "://";	
		}
		url += host;
		if (! port.empty()){
			url += ":" + host;	
		}
	}
	if (! path.empty()){
		url += path;
		string new_query = query_params_modified ? buildQuery(query_params) : query;
		if (! new_query.empty()){
			url += "?" + new_query;
		}
		if (! fragment.empty()){
			url += "#" + fragment;
		}
	}
	return url;
}

void URLComponents::setQueryParam(const string &name, const string &value) {
	bool found_name = false;
	for (list<pair<string, string> >::iterator itr = query_params.begin(); itr != query_params.end(); ++ itr){
		if (itr->first == name){
			itr->second = value;
			found_name = true;
			break;
		}
	}
	if (! found_name) {
		query_params.push_back(make_pair(name, value));
	}
	query_params_modified = true;
}

string URLComponents::getQueryParam(const string &name) const {
	for (list<pair<string, string> >::const_iterator itr = query_params.begin(); itr != query_params.end(); ++ itr){
		if (itr->first == name){
			return itr->second;
		}
	}
	return "";
}

void URLComponents::eraseQueryParam(const string &name) {
	for (list<pair<string, string> >::iterator itr = query_params.begin(); itr != query_params.end(); ++ itr){
		if (itr->first == name){
			query_params.erase(itr);
			break;
		}
	}
	query_params_modified = true;
}

} // namespace util
} // namespace http
