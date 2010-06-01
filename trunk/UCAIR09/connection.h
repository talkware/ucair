#ifndef __http_connection__
#define __http_connection__

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "reply.h"
#include "request.h"
#include "request_parser.h"

namespace http {
namespace server {

/// Handler type: takes in a request and sends out a reply
typedef boost::function<void (Request& request, Reply& reply)> RequestHandler;

class ConnectionManager;

/// Represents a single connection from a client.
class Connection: public boost::enable_shared_from_this<Connection>, private boost::noncopyable {
public:
	/*! \brief Constructs a connection.
	 *  \param io_service IO service
	 *  \param manager connection manager
	 *  \param RequestHandler handler that processes requests to produce replies
	 */
	explicit Connection(boost::asio::io_service& io_service, ConnectionManager& manager, const RequestHandler& request_handler);

	/// Returns the socket associated with the connection.
	boost::asio::ip::tcp::socket& socket();

	/// Starts the first asynchronous operation for the connection.
	void start();

	/// Stops all asynchronous operations associated with the connection.
	void stop();

private:
	/// Handles completion of a read operation.
	void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);

	/// Handles completion of a write operation.
	void handleWrite(const boost::system::error_code& e);

	/// Socket for the connection.
	boost::asio::ip::tcp::socket socket_;

	/// Buffer for incoming data.
	boost::array<char, 8 * 1024> buffer;

	/// The manager for this connection.
	ConnectionManager& connection_manager;

	/// The request received from the client.
	Request request;

	/// The parser for the incoming request.
	RequestParser request_parser;

	/// The handler used to process the incoming request.
	const RequestHandler& request_handler;

	/// The reply to be sent back to the client.
	Reply reply;
};

typedef boost::shared_ptr<Connection> ConnectionPtr;

} // namespace server
} // namespace http

#endif
