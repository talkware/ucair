#include "http_download.h"
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include "common_util.h"
#include "url_components.h"

using namespace std;
using namespace boost;

namespace http {
namespace util {

const string DEFAULT_USER_AGENT = "Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US; rv:1.9.0.10) Gecko/2009042316 Firefox/3.0.10 (.NET CLR 3.5.30729)";

bool downloadPage(const string &url, string &content, string &err_msg, bool follow_redirect){
	content.clear();
	err_msg.clear();

	if (starts_with(url, "file://")) {
		string path = url.substr(7);
		ifstream fin(path.c_str());
		if (! fin) {
			err_msg = "File does not exist: " + path;
			return false;
		}
		::util::readFile(fin, content);
		return true;
	}

	URLComponents url_components(url);

	if (! starts_with(url_components.scheme, "http")){
		err_msg = "Not HTTP";
		return false;
	}

	if (url_components.path.empty()){
		url_components.path = "/";
	}

	asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
	asio::ip::tcp::resolver resolver(io_service);
	asio::ip::tcp::resolver::query query(url_components.host, "http");
	asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	asio::ip::tcp::resolver::iterator end;

	// Try each endpoint until we successfully establish a connection.
	asio::ip::tcp::socket socket(io_service);
	boost::system::error_code error = asio::error::host_not_found;
	while (error && endpoint_iterator != end){
		socket.close();
		socket.connect(*endpoint_iterator ++, error);
	}
	if (error){
		err_msg = "Failed to connect";
		return false;
	}

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	asio::streambuf request;
	ostream request_stream(&request);
	request_stream << "GET " << url_components.path_query_fragment << " HTTP/1.0\r\n";
	request_stream << "Host: " << url_components.host << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n";
	request_stream << "User-Agent: " << DEFAULT_USER_AGENT << "\r\n";
	request_stream << "\r\n";

	// Send the request.
	asio::write(socket, request);

	// Read the response status line.
	asio::streambuf response;
	asio::read_until(socket, response, "\r\n");

	// Check that response is OK.
	istream response_stream(&response);
	string http_version;
	response_stream >> http_version;
	string status_code;
	response_stream >> status_code;
	string status_message;
	getline(response_stream, status_message);
	if (! response_stream || ! starts_with(http_version, "HTTP/")){
		err_msg = "Invalid reply header";
		return false;
	}
	if (status_code != "200" && status_code != "301" && status_code != "302"){
		return false;
	}

	// Read the response headers, which are terminated by a blank line.
	asio::read_until(socket, response, "\r\n\r\n");

	// Process the response headers.
	string header;
	string location;
	while (getline(response_stream, header) && header != "\r"){
		if (starts_with(header, "Location: ")){
			size_t pos = header.find(":");
			if (pos != string::npos){
				location = header.substr(pos + 1);
				trim(location);
			}
		}
	}

	// Read until EOF, writing data to output as we go.
	asio::read(socket, response, boost::asio::transfer_all(), error);
	if (error != asio::error::eof){
		err_msg = "Failed to read content";
		return false;
	}

	if (follow_redirect && (status_code == "301" || status_code == "302") && ! location.empty()){
		return downloadPage(url, content, err_msg, false); // only redirect once
	}

	size_t content_length = response.size();
	content.reserve(content_length);
	for (size_t i = 0; i < content_length && response_stream.good(); ++ i){
		content.push_back(response_stream.get());
	}
	return true;
}

} // namespace util
} // namespace http
