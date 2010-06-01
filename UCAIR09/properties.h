#ifndef __properties_h__
#define __properties_h__

#include <map>
#include <string>
#include <boost/any.hpp>
#include <boost/tuple/tuple.hpp>

namespace util {

/// A property map that can store arbitrary typed data indexed by names.
class Properties {
public:

	/// Adds a value indexed by name. If name already exists, do nothing.
	template <class T>
	void set(const std::string &name, const T &value){
		std::map<std::string, boost::any>::iterator itr;
		bool inserted;
		boost::tie(itr, inserted) = properties.insert(make_pair(name, value));
		if (! inserted) {
			itr->second = value;
		}
	}

	/// Whether there is a value with a given name.
	bool has(const std::string &name) const {
		return properties.find(name) != properties.end();
	}

	/*! \brief Retrieves the value indexed by a given name.
	 *
	 *  Should call has() first to check the existence of the value.
	 *  \throw bad_any_cast if name does not exist.
	 */
	template<class T>
	T& get(const std::string &name){
		std::map<std::string, boost::any>::iterator itr = properties.find(name);
		boost::any &value = itr == properties.end() ? null_value : itr->second;
		return boost::any_cast<T&>(value);
	}

	/*! \brief Retrieves the value indexed by a given name. (const version)
	 *
	 *  Should call has() first to check the existence of the value.
	 *  \throw bad_any_cast if name does not exist.
	 */	template<class T>
	const T& get(const std::string &name) const {
		std::map<std::string, boost::any>::const_iterator itr = properties.find(name);
		const boost::any &value = itr == properties.end() ? null_value : itr->second;
		return boost::any_cast<const T&>(value);
	}

private:
	std::map<std::string, boost::any> properties;
	static boost::any null_value;
};

} // namespace util

#endif
