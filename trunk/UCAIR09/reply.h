#ifndef __http_reply__
#define __http_reply__

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <boost/asio.hpp>

namespace http {
namespace server {

/// A list of response status codes.
namespace reply_status {
	const int ok = 200;
	const int created = 201;
	const int accepted = 202;
	const int no_content = 204;
	const int multiple_choices = 300;
	const int moved_permanently = 301;
	const int moved_temporarily = 302;
	const int not_modified = 304;
	const int bad_request = 400;
	const int unauthorized = 401;
	const int forbidden = 403;
	const int not_found = 404;
	const int internal_server_error = 500;
	const int not_implemented = 501;
	const int bad_gateway = 502;
	const int service_unavailable = 503;

	/// Converts a status code to its associated textual phrase.
	std::string toString(int status);
};

/// HTTP response message.
class Reply {
public:

	Reply();

	int status; ///< status code

	std::multimap<std::string, std::string> headers; ///< map from header field name to field value

	std::string content; ///< response body

	/// Converts the reply into a vector of buffers.
	std::vector<boost::asio::const_buffer> toBuffers();

	/// Prints reply header.
	friend std::ostream& operator << (std::ostream &out, const Reply &rep);

private:

	/// Sets status line from status code.
	void setStatusLine();

	/// Sets reply content from a stock template.
	void setStockReply();

	std::string status_line; ///< status line
};

} // namespace server
} // namespace http

#endif
