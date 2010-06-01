#include "url_encoding.h"
#include <cctype>
#include <sstream>

using namespace std;

namespace http {
namespace util {

void urlEncode(const string &in, string &out){
	static const string escaping_free("-_.~"); // These characters do not need to be escaped.
	out.clear();
	out.reserve(in.size() * 3);
	for (size_t i = 0; i < in.size(); ++ i){
		unsigned char ch = static_cast<unsigned char>(in[i]);
		if (isalnum(ch) || escaping_free.find(ch) != string::npos){
			out.push_back(ch);
		}
		else if (ch == ' '){
			out.push_back('+');
		}
		else{
			ostringstream os;
			os << hex << static_cast<int>(ch);
			out.push_back('%');
			out += os.str();
		}
	}
}

bool urlDecode(const string &in, string &out){
	out.clear();
	out.reserve(in.size());
	for (size_t i = 0; i < in.size(); ++ i){
		if (in[i] == '%'){
			if (i + 3 > in.size()){
				return false;
			}
			int value;
			istringstream is(in.substr(i + 1, 2));
			if (is >> hex >> value){
				out.push_back(static_cast<char>(value));
			}
			else{
				return false;
			}
			i += 2;
		}
		else if (in[i] == '+'){
			out.push_back(' ');
		}
		else{
			out.push_back(in[i]);
		}
	}
	return true;
}

} // namespace util
} // namespace http
