#ifndef __http_request_h__
#define __http_request_h__

#include <iostream>
#include <map>
#include <string>

namespace http {
namespace server {

/// HTTP request message.
class Request {
public:
	Request();

	std::string method; ///< GET / POST / HEAD
	std::string url; ///< request URI
	int http_version_major; ///< HTTP major version
	int http_version_minor; ///< HTTP minor version
	std::multimap<std::string, std::string> headers; ///< map from header field name to field value
	std::string content; ///< request body

	/// Prints request header.
	friend std::ostream& operator << (std::ostream &out, const Request &req);
};

} // namespace server
} // namespace http

#endif
