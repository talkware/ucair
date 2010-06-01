#ifndef __http_server_h__
#define __http_server_h__

#include <string>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include "connection_manager.h"

namespace http {
namespace server {

/// The top-level class of the HTTP server.
class Server: private boost::noncopyable {
public:
	/*! Constructs the server to listen on the specified TCP address and port, and serve up files from the given directory.
	 *  \param io_service IO service
	 *  \param address IP address to bind to
	 *  \param port port to bind to
	 *  \param request_handler handler that processes requests to produce replies
	 */
	explicit Server(boost::asio::io_service &io_service, const std::string& address, const std::string& port, const RequestHandler &request_handler);

	/// Runs the server's io_service loop.
	void run();

	/// Stops the server.
	void stop(bool immediately);

private:
	/// Handles completion of an asynchronous accept operation.
	void handleAccept(const boost::system::error_code& e);

	/// Handles a request to stop the server.
	void handleStop();

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service &io_service_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	/// Handler that processes requests to produce replies.
	RequestHandler request_handler;

	/// The connection manager which owns all live connections.
	ConnectionManager connection_manager;

	/// The next connection to be accepted.
	ConnectionPtr new_connection;
};

} // namespace server
} // namespace http

#endif
