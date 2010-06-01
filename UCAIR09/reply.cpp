#include "reply.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

namespace http {
namespace server {

namespace reply_status {

string toString(int status){
	switch (status){
	case reply_status::ok:
		return "OK";
	case reply_status::created:
		return "Created";
	case reply_status::accepted:
		return "Accepted";
	case reply_status::no_content:
		return "No Content";
	case reply_status::multiple_choices:
		return "Multiple Choices";
	case reply_status::moved_permanently:
		return "Moved Permanently";
	case reply_status::moved_temporarily:
		return "Moved Temporarily";
	case reply_status::not_modified:
		return "Not Modified";
	case reply_status::bad_request:
		return "Bad Request";
	case reply_status::unauthorized:
		return "Unauthorized";
	case reply_status::forbidden:
		return "Forbidden";
	case reply_status::not_found:
		return "Not found";
	case reply_status::internal_server_error:
		return "Internal Server Error";
	case reply_status::not_implemented:
		return "Not Implemented";
	case reply_status::bad_gateway:
		return "Bad Gateway";
	case reply_status::service_unavailable:
		return "Service Unavailable";
	default:
		return "";
	}
}

} // namespace reply_status

ostream& operator<< (ostream &out, const Reply &reply){
	out << reply.status_line << endl;
	for (multimap<string, string>::const_iterator itr = reply.headers.begin(); itr != reply.headers.end(); ++ itr){
		out << itr->first << ": " << itr->second << endl;
	}
	return out;
}

Reply::Reply(): status(reply_status::ok) {
}

void Reply::setStatusLine() {
	string status_str = reply_status::toString(status);
	status_line = str(format("HTTP/1.0 %1% %2%") % status % status_str);
}

void Reply::setStockReply() {
	string status_str = reply_status::toString(status);
	content = str(format(
			"<html>"
			"<head><title>%1%</title></head>"
			"<body><h1>%2% %1%</h1></body>"
			"</html>") % status_str % status);
	headers.insert(make_pair("Content-Type", "text/html"));
}

/*! The buffers do not own the underlying memory blocks,
 *  therefore the reply object must remain valid and not be changed until the write operation has completed.
 *
 *  If status is not OK and reply content is missing, a stock reply is provided.
 *
 *  Content length is automatically calculated.
 */
vector<asio::const_buffer> Reply::toBuffers(){
	setStatusLine();
	if (status != reply_status::ok && content.empty()){
		setStockReply();
	}
	headers.insert(make_pair("Content-Length", lexical_cast<string>(content.size())));

	vector<asio::const_buffer> buffers;
	buffers.push_back(asio::buffer(status_line));
	buffers.push_back(asio::buffer("\r\n", 2));
	for (multimap<string, string>::const_iterator itr = headers.begin(); itr != headers.end(); ++ itr){
		buffers.push_back(asio::buffer(itr->first));
		buffers.push_back(asio::buffer(": ", 2));
		buffers.push_back(asio::buffer(itr->second));
		buffers.push_back(asio::buffer("\r\n", 2));
	}
	buffers.push_back(asio::buffer("\r\n", 2));
	buffers.push_back(asio::buffer(content));
	return buffers;
}

} // namespace server
} // namespace http
