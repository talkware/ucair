#ifndef __config_h__
#define __config_h__

#include <exception>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace util {

/* This has some overlap with the Properties class.
 * Main difference is that value is always string typed, so it's easier to read from a config file.
 */

/*! \brief Read config params from input stream.
 *
 *  \param[in] in input stream
 *  \param[out] params param name-value pairs
 */
void readParams(std::istream &fin, std::map<std::string, std::string> &params);

/*! \brief Extracts a param from the config map.
 *
 *  \param[in] params map from param name to param value
 *  \param[in] name param name
 *  \param[out] value param value
 */
template <class T>
void getParam(const std::map<std::string, std::string> &params, const std::string &name, T &value){
	std::map<std::string, std::string>::const_iterator itr = params.find(name);
	if (itr == params.end()){
		throw std::runtime_error("Missing value for param " + name);
	}
	std::istringstream strin(itr->second);
	strin >> value;
}

/// Template specialization for string type.
void getParam(const std::map<std::string, std::string> &params, const std::string &name, std::string &value);

/*! \brief Extracts a param from the config map.
 *
 *  \param params map from param name to param value
 *  \param name param name
 *  \return param value
 */
template <class T>
T getParam(const std::map<std::string, std::string> &params, const std::string &name){
	T value;
	getParam(params, name, value);
	return value;
}

} // namespace util

#endif
