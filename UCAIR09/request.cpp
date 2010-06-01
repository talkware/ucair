#include "request.h"

using namespace std;

namespace http {
namespace server {

Request::Request(): http_version_major(0), http_version_minor(0) {
}

ostream& operator<< (ostream &out, const Request &request){
	out << request.method << " " << request.url << " HTTP/" << request.http_version_major << "." << request.http_version_minor << endl;
	for (multimap<string, string>::const_iterator itr = request.headers.begin(); itr != request.headers.end(); ++ itr){
		out << itr->first << ": " << itr->second << endl;
	}
	return out;
}

} // namespace http
} // namespace server
