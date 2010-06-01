#ifndef __http_connection_manager__
#define __http_connection_manager__

#include <set>
#include <boost/noncopyable.hpp>
#include "connection.h"

namespace http {
namespace server {

/// Manages open connections so that they may be cleanly stopped when the server needs to shut down.
class ConnectionManager: private boost::noncopyable {
public:
	/// Adds the specified connection to the manager and start it.
	void start(ConnectionPtr c);

	/// Stops the specified connection.
	void stop(ConnectionPtr c);

	/// Stops all connections.
	void stopAll();

private:
	/// The managed connections.
	std::set<ConnectionPtr> connections;
};

} // namespace server
} // namespace http

#endif
