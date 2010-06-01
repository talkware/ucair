#ifndef __error_h__
#define __error_h__

#include <exception>
#include <string>
#include <boost/exception/all.hpp>

namespace ucair {

typedef boost::error_info<struct tag_error_message, std::string> ErrorMsg;

class Error: public std::exception, public boost::exception {
public:
	const char* what() const throw () { return "UCAIR Error"; }
};

} // namespace ucair

#endif
