#ifndef __prototype_h__
#define __prototype_h__

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>

namespace util {

/*! \brief A prototype is an object that can be cloned to produce new objects.
 *
 *  Check the prototype design pattern.
 */
class Prototype {
public:
	virtual ~Prototype() {}
	virtual boost::shared_ptr<Prototype> clone() const = 0;
};

/* Implement the clone method using copy constructor */
#define IMPLEMENT_CLONE(T) boost::shared_ptr<Prototype> clone() const { return boost::shared_ptr<Prototype>(new T(*this)); }

/*! \brief A factory for creating new objects using prototypes.
 *
 *  Prototypes are looked up by name.
 */
class PrototypedFactory {
public:

	/// Registers a prototype by name.
	static void registerPrototype(const std::string &name, const boost::shared_ptr<Prototype> &prototype) {
		prototypes.insert(make_pair(name, prototype));
	}

	/*! \brief Creates a new object by cloning the registered prototype with the given name.
	 *
	 * @param name name of the registered prototype
	 * @return cloned new object. null pointer if not found.
	 */
	static boost::shared_ptr<Prototype> makeInstance(const std::string &name) {
		std::map<std::string, boost::shared_ptr<Prototype> >::const_iterator itr = prototypes.find(name);
		if (itr != prototypes.end()) {
			return itr->second->clone();
		}
		return boost::shared_ptr<Prototype>();
	}

private:
	static std::map<std::string, boost::shared_ptr<Prototype> > prototypes;
};

} // namespace util

#endif
