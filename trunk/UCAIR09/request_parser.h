#ifndef __http_request_parser__
#define __http_request_parser__

#include <string>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>

namespace http {
namespace server {

class Request;

/// Parser for incoming requests.
class RequestParser {
public:
	/*! \brief Constructor
	 *  \param max_content_length exceeding part will be truncated
	 */
	RequestParser(size_t max_content_length);

	/// Resets to initial parser state.
	void reset();

	/*! Parses some data.
	 *  \param req request
	 *  \param begin input begin pos
	 *  \param end input end pos
	 *  \return tribool: true when a complete request has been parsed, false if the data is invalid, indeterminate when more data is required.
	 *          InputIterator: indicates how much of the input has been consumed.
	 */
	template <typename InputIterator>
	boost::tuple<boost::tribool, InputIterator> parse(Request& req, InputIterator begin, InputIterator end){
		while (begin != end){
			boost::tribool result = consume(req, *begin++);
			if (result || !result)
				return boost::make_tuple(result, begin);
		}
		boost::tribool result = boost::indeterminate;
		return boost::make_tuple(result, begin);
	}

private:
	/// Handles the next character of input.
	boost::tribool consume(Request& req, char input);

	/// Checks if a byte is an HTTP character.
	static bool isChar(int c);

	/// Checks if a byte is an HTTP control character.
	static bool isCtrl(int c);

	/// Checks if a byte is defined as an HTTP tspecial character.
	static bool isTspecial(int c);

	/// Checks if a byte is a digit.
	static bool isDigit(int c);

	/// Current parser state
	enum State {
		method_start,
		method,
		uri_start,
		uri,
		http_version_h,
		http_version_t_1,
		http_version_t_2,
		http_version_p,
		http_version_slash,
		http_version_major_start,
		http_version_major,
		http_version_minor_start,
		http_version_minor,
		expecting_newline_1,
		header_line_start,
		header_lws,
		header_name,
		space_before_header_value,
		header_value,
		expecting_newline_2,
		expecting_newline_3,
		entity_body
	} state;

	std::string cur_header_name; ///< current header field name
	std::string cur_header_value; ///< current header field value
	size_t remaining_content_length; ///< unconsumed request content length
	size_t max_content_length; ///< max allowed request content length
};

} // namespace server
} // namespace http

#endif
