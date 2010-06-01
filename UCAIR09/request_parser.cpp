#include "request_parser.h"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "request.h"
#include "url_components.h"

using namespace std;
using namespace boost;

namespace http {
namespace server {

RequestParser::RequestParser(size_t max_content_length_):
	state(method_start),
	remaining_content_length(0),
	max_content_length(max_content_length_)
{
}

void RequestParser::reset(){
	state = method_start;
}

tribool RequestParser::consume(Request& request, char input){
	switch (state){
	case method_start:
		if (!isChar(input) || isCtrl(input) || isTspecial(input)){
			return false;
		}
		else{
			state = method;
			request.method.push_back(input);
			return indeterminate;
		}
	case method:
		if (input == ' '){
			state = uri;
			return indeterminate;
		}
		else if (!isChar(input) || isCtrl(input) || isTspecial(input)){
			return false;
		}
		else{
			request.method.push_back(input);
			return indeterminate;
		}
	case uri_start:
		if (isCtrl(input)){
			return false;
		}
		else{
			state = uri;
			request.url.push_back(input);
			return indeterminate;
		}
	case uri:
		if (input == ' '){
			state = http_version_h;
			return indeterminate;
		}
		else if (isCtrl(input)){
			return false;
		}
		else{
			request.url.push_back(input);
			return indeterminate;
		}
	case http_version_h:
		if (input == 'H'){
			state = http_version_t_1;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_t_1:
		if (input == 'T'){
			state = http_version_t_2;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_t_2:
		if (input == 'T'){
			state = http_version_p;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_p:
		if (input == 'P'){
			state = http_version_slash;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_slash:
		if (input == '/'){
			request.http_version_major = 0;
			request.http_version_minor = 0;
			state = http_version_major_start;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_major_start:
		if (isDigit(input)){
			request.http_version_major = request.http_version_major * 10 + input - '0';
			state = http_version_major;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_major:
		if (input == '.'){
			state = http_version_minor_start;
			return indeterminate;
		}
		else if (isDigit(input)){
			request.http_version_major = request.http_version_major * 10 + input - '0';
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_minor_start:
		if (isDigit(input)){
			request.http_version_minor = request.http_version_minor * 10 + input - '0';
			state = http_version_minor;
			return indeterminate;
		}
		else{
			return false;
		}
	case http_version_minor:
		if (input == '\r'){
			state = expecting_newline_1;
			return indeterminate;
		}
		else if (isDigit(input)){
			request.http_version_minor = request.http_version_minor * 10 + input - '0';
			return indeterminate;
		}
		else{
			return false;
		}
	case expecting_newline_1:
		if (input == '\n'){
			state = header_line_start;
			return indeterminate;
		}
		else{
			return false;
		}
	case header_line_start:
		cur_header_name = cur_header_value = "";
		if (input == '\r'){
			state = expecting_newline_3;
			return indeterminate;
		}
		else if (!request.headers.empty() && (input == ' ' || input == '\t')){
			state = header_lws;
			return indeterminate;
		}
		else if (!isChar(input) || isCtrl(input) || isTspecial(input)){
			return false;
		}
		else{
			state = header_name;
			cur_header_name.push_back(input);
			return indeterminate;
		}
	case header_lws:
		if (input == '\r'){
			request.headers.insert(make_pair(cur_header_name, cur_header_value));
			cur_header_name = cur_header_value = "";
			state = expecting_newline_2;
			return indeterminate;
		}
		else if (input == ' ' || input == '\t'){
			return indeterminate;
		}
		else if (isCtrl(input)){
			return false;
		}
		else{
			state = header_value;
			cur_header_value.push_back(input);
			return indeterminate;
		}
	case header_name:
		if (input == ':'){
			state = space_before_header_value;
			return indeterminate;
		}
		else if (!isChar(input) || isCtrl(input) || isTspecial(input)){
			return false;
		}
		else{
			cur_header_name.push_back(input);
			return indeterminate;
		}
	case space_before_header_value:
		if (input == ' '){
			state = header_value;
			return indeterminate;
		}
		else{
			return false;
		}
	case header_value:
		if (input == '\r'){
			if (cur_header_name == "Content-Length"){
				try{
					remaining_content_length = lexical_cast<size_t>(cur_header_value); 
				}
				catch (bad_lexical_cast){
					cur_header_value = "0";
				}
				if (remaining_content_length > max_content_length){
					remaining_content_length = max_content_length;
				}
			}
			request.headers.insert(make_pair(cur_header_name, cur_header_value));
			cur_header_name = cur_header_value = "";
			state = expecting_newline_2;
			return indeterminate;
		}
		else if (isCtrl(input)){
			return false;
		}
		else{
			cur_header_value.push_back(input);
			return indeterminate;
		}
	case expecting_newline_2:
		if (input == '\n'){
			state = header_line_start;
			return indeterminate;
		}
		else{
			return false;
		}
	case expecting_newline_3:
		if (input == '\n'){
			if (remaining_content_length == 0){
				return true;
			}
			else{
				state = entity_body;
				return indeterminate;
			}
		}
		else{
			return false;
		}
	case entity_body:
		request.content.push_back(input);
		if (-- remaining_content_length <= 0){
			return true;
		}
		else{
			return indeterminate; 
		}
	default:
		return false;
	}
}

bool RequestParser::isChar(int c){
	return c >= 0 && c <= 127;
}

bool RequestParser::isCtrl(int c){
	return c >= 0 && c <= 31 || c == 127;
}

bool RequestParser::isTspecial(int c){
	switch (c){
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '/': case '[': case ']': case '?': case '=':
	case '{': case '}': case ' ': case '\t':
		return true;
	default:
		return false;
	}
}

bool RequestParser::isDigit(int c){
	return c >= '0' && c <= '9';
}

} // namespace server
} // namespace http
