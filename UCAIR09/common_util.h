#ifndef __common_util_h__
#define __common_util_h__

#include <fstream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace util {

/*! \brief Reads the content of a file into a string.
 *  \param fin [in]
 *  \param content [out]
 */
void readFile(std::ifstream &fin, std::string &content);

/*! \brief Creates a (hopefully) unique id.
 *
 *  The id is created using a timestamp and an increasing sequence number.
 */
std::string makeId();

/*! \brief Compares two ids based on their order of creation.
 *  \return 0 if equal, negative if first id is earlier, positve if second id is earlier
 */
int cmpId(const std::string &a, const std::string &b);

/// RFC 822 format e.g. Sun, 06 Nov 1994 08:49:37 GMT
const std::string internet_time_format("%a, %d %b %Y %H:%M:%S GMT");

/*! \brief Converts time value to string.
 *  \param t posix time
 *  \param format time format
 *	\return formatted time string
 */
std::string timeToString(const boost::posix_time::ptime &t, const std::string &format);

/*! \brief Parses a time string.
 *  \param s time string
 *	\param format time format
 *  \return posix time (not_a_date_time in case of parsing error)
 */
boost::posix_time::ptime stringToTime(const std::string &s, const std::string &format);

/// Converts UTC time to local time.
boost::posix_time::ptime utc_to_local(const boost::posix_time::ptime &utc_time);

/// Converts local time to UTC time.
boost::posix_time::ptime local_to_utc(const boost::posix_time::ptime &local_time);

/// Converts ptime to time_t
time_t to_time_t(const boost::posix_time::ptime &utc_time);

/// An FIFO list that ignores insertion of duplicate elements, implemented using boost::multi_index.
typedef boost::multi_index::multi_index_container<
	int,
	boost::multi_index::indexed_by<
		boost::multi_index::sequenced<>,
		boost::multi_index::ordered_unique<boost::multi_index::identity<int> >
	>
> UniqueList;

/// Compares the first element of a pair.
template<class T, class U>
bool cmp1st(const std::pair<T, U> &a, const std::pair<T, U> &b){
	return a.first < b.first;
}

/// Compares the first element of a pair, in reverse order.
template<class T, class U>
bool cmp1stReverse(const std::pair<T, U> &a, const std::pair<T, U> &b){
	return a.first > b.first;
}

/// Compares the second element of a pair.
template<class T, class U>
bool cmp2nd(const std::pair<T, U> &a, const std::pair<T, U> &b){
	return a.second < b.second;
}

/// Compares the second element of a pair, in reverse order.
template<class T, class U>
bool cmp2ndReverse(const std::pair<T, U> &a, const std::pair<T, U> &b){
	return a.second > b.second;
}

/// Returns the class name (without namespace) of an object.
template <class T>
std::string getClassName(const T &x) {
	std::string s = typeid(x).name();
	size_t pos = s.rfind("::");
	if (pos != std::string::npos) {
		return s.substr(pos + 2);
	}
	pos = s.find(' ');
	return s.substr(pos + 1);
}

/// Whether a character is whitespace (blank, tab, linebreak).
inline bool isWhitespace(char ch) {
	return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

/// Whether a string contains only ASCII printable characters (32-127).
bool isASCIIPrintable(const std::string &s);

/*! \brief Tokenizes a string into words using whitespace as separators.
 *
 * @param text input string
 * @param quote_phrase whether to treat everything inside double quotes as a single word.
 * @return tokens
 */
std::vector<std::string> tokenizeWithWhitespace(const std::string &text, bool quote_phrase = false);

/// Tokenizes a string into words using non-alphanumeric characters as separators.
std::vector<std::string> tokenizeWithPunctuation(const std::string &text);

} // namespace util

#endif
