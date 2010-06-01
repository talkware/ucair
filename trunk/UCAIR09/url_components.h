/*! \file url_components.h
 *  \brief Utilities to parse and build URLs and queries.
 */

#ifndef __url_components_h__
#define __url_components_h__

#include <map>
#include <string>
#include <boost/regex.hpp>
#include "url_encoding.h"

namespace http {
namespace util {

/*! \brief URL parser
 *
 *  http://www.w3.org/Protocols/HTTP/1.0/draft-ietf-http-spec.html
 */
class URLComponents{
public:

	/*! \brief Parses query params, i.e. the name=value pairs.
	 *
	 *  URL percent decoding is applied.
	 *	\param[in] query query part of a URL
	 *	\param[out] params a container of (name, value) pairs
	 *	\returns false if query cannot be parsed
	 */
	template <class Container>
	static bool parseQuery(const std::string &query, Container &params){
		static const boost::regex re("([^=&]+)=([^=&]*)");
		boost::sregex_iterator end;
		for (boost::sregex_iterator itr = boost::make_regex_iterator(query, re); itr != end; ++ itr){
			const boost::smatch &results = *itr;
			std::string name, value;
			if (! urlDecode(results[1], name)){
				return false;
			}
			if (! urlDecode(results[2], value)){
				return false;
			}
			params.insert(params.end(), make_pair(name, value));
		}
		return true;
	}

	/*! \brief Builds a query from a list of (name, value) pairs.
	 *
	 * 	URL percent encoding is applied.
	 * 	Example: if input is [(n1, v1), (n2, v2)], output is "n1=v1&n2=v2"
	 *  \param params a container of (name, value) pairs
	 *  \return query built from the params
	 */
	template <class Container>
	static std::string buildQuery(const Container &params){
		std::string query;
		for (typename Container::const_iterator itr = params.begin(); itr != params.end(); ++ itr){
			if (! query.empty()){
				query.push_back('&');
			}
			std::string name, value;
			urlEncode(itr->first, name);
			urlEncode(itr->second, value);
			query += name + "=" + value;
		}
		return query;
	}

	/*! \brief Constructor
	 *  \param url URL to parse. Components are stored in various fields.
	 */
	explicit URLComponents(const std::string &url);

	std::string url; ///< complete url
	std::string scheme; ///< scheme (e.g. http)
	std::string host; ///< host (e.g. www.uiuc.edu)
	std::string port; ///< port (e.g. 8080)
	std::string path; ///< path (e.g. /cgi/lookup)
	std::string query; ///< query (e.g. name=bintan&id=1)
	std::string fragment; ///< fragment (e.g. top)
	std::string path_query_fragment; ///< concatenation of path, query, fragment (e.g. /cgi/lookup?name=bintan&id=1\#top)

	/*! \brief Rebuilds a URL with the components.
	 *  \return rebuilt URL
	 */
	std::string rebuildURL() const;

	/// Adds / updates a query param.
	void setQueryParam(const std::string &name, const std::string &value);
	/// Returns the value of a query params.
	std::string getQueryParam(const std::string &name) const;
	/// Erases a query param.
	void eraseQueryParam(const std::string &name);

private:
	bool query_params_modified;
	std::list<std::pair<std::string, std::string> > query_params;
};

} // namespace util
} // namespace http

#endif
