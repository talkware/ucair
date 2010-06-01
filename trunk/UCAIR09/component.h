#ifndef __component_h__
#define __component_h__

#include <string>
#include <boost/utility.hpp>

namespace ucair {

class User;

/*! \brief A component in the UCAIR framework.
 *
 *  During its lifetime, a component will receive the following calls:
 *  constructor
 *  ...
 *  initialize
 *  ...
 *  initializeUser(user1)
 *  ...
 *  initializeUser(user2)
 *  ...
 *  finalizeUser(user1)
 *  ...
 *  finalizeUser(user2)
 *  ...
 *  finalize
 *  ...
 *  destructor
 */
class Component: private boost::noncopyable {
public:

	virtual ~Component() {}

	/*! Initializes the component.
	 *  \return true if initialization succeeds
	 */
	virtual bool initialize() { return true; }

	/*! Finalizes the component.
	 *  \return true if finalization succeeds
	 */
	virtual bool finalize() { return true; }

	/*! Initializes the component for a user.
	 *  \param user
	 *  \return true if initialization succeeds
	 */
	virtual bool initializeUser(User &user) { return true; }

	/*! Finalizes the component.
	 *  \param user
	 *  \return true if finalization succeeds
	 */
	virtual bool finalizeUser(User &user) { return true; }
};

} // namespace ucair

#endif
