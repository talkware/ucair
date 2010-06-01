#ifndef __user_manager_h__
#define __user_manager_h__

#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/signal.hpp>
#include "component.h"
#include "main.h"
#include "user.h"

namespace ucair {

class Request;
class Reply;

class UserManager: public Component {
public:

	bool initialize();

	/// Returns the user by id.
	User* getUser(const std::string &user_id) const;

	/// Returns all user ids.
	std::vector<std::string> getAllUserIds(bool logged_on_only = true) const;

	/// Whether a user was created before.
	bool existsUser(const std::string &user_id) const;

	/// Returns the user associated with a search.
	User* getUserBySearchId(const std::string &search_id) const;

	/*! Returns the user associated with an incoming request.
	 *
	 *  If user is not found, redirects to the login page.
	 */
	User* getUser(const Request &request, bool log_on_automatically);

	/// Returns the profile directory of a user.
	std::string getProfileDir(const std::string &user_id) const;

	/// Creates user files if they do not exist yet.
	void createUserFiles(const std::string &user_id) const;

	/// Initializes a user.
	bool initializeUser(const std::string &user_id = "");

	/// Finalizes a user.
	bool finalizeUser(const std::string &user_id);

	/// Signal that is fired whenever there is a user event.
	boost::signal<void (User &, const UserEvent&)> user_event_signal;

private:

	std::string default_user_id;

	std::map<std::string, boost::shared_ptr<User> > users;

	std::string profiles_dir;

	std::map<std::string, std::string> search2user;

friend class User;
friend class LogImporter;
};

DECLARE_GET_COMPONENT(UserManager);

} // namespace ucair

#endif
