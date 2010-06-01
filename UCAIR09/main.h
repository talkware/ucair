#ifndef __main_h__
#define __main_h__

#include <cassert>
#include <list>
#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/utility.hpp>

namespace ucair{

class Component;

/// Main execution. Used in exe_main.cpp
class Main: private boost::noncopyable {
public:
	Main(int argc, char *argv[]);
	~Main();

	/// Returns a singleton.
	static Main& instance() { return *_instance; }

	/// Reads config options, inits components and starts running.
	void start();

	/// Finalizes components and stops running.
	void stop();

	void interrupt();

	/// HTTP server needs this.
	boost::asio::io_service io_service;

	/*! \brief Adds a component.
	 *  The component will be able to readconfig options, get initialized and finalized.
     */
	void addComponent(Component *component) {
		components.push_back(component);
	}

	/// Returns a previously added component by type.
	template<class T>
	T& getComponent() {
		T *p = NULL;
		BOOST_FOREACH(Component *component, components) {
			p = dynamic_cast<T*>(component);
			if (p) {
				return *p;
			}
		}
		assert(false);
		return *p; // just to supress compiler warning
	}

	std::list<Component*>& getAllComponents() { return components; }

	std::map<std::string, std::string>& getConfig() { return config; }

private:

	bool getOptions(int argc, char *argv[]);

	std::list<Component*> components;
	static Main* _instance;
	int argc;
	char **argv;

	std::map<std::string, std::string> config;

	std::string start_mode;
	volatile bool started;
};

/// Use this to declare a global component getter. For example, DECLARE_GET_COMPONENT(SearchProxy) declares getSearchProxy()
#define DECLARE_GET_COMPONENT(name)	inline name& get##name() { return Main::instance().getComponent<name>(); }

} // namespace ucair

#endif
