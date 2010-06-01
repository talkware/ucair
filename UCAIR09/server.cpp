#include "server.h"
#include <boost/bind.hpp>

using namespace std;
using namespace boost;

namespace http {
namespace server {

Server::Server(asio::io_service &io_service, const string& address, const string& port, const RequestHandler &handler):
	io_service_(io_service),
	acceptor_(io_service_),
	request_handler(handler),
	connection_manager(),
	new_connection(new Connection(io_service_, connection_manager, request_handler))
{
	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	asio::ip::tcp::resolver resolver(io_service_);
	asio::ip::tcp::resolver::query query(address, port);
	asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();
	acceptor_.async_accept(
			new_connection->socket(),
			bind(&Server::handleAccept, this, asio::placeholders::error));
}

void Server::run(){
	// The io_service::run() call will block until all asynchronous operations have finished.
	// While the server is running, there is always at least one asynchronous operation outstanding:
	//  the asynchronous accept call waiting for new incoming connections.
	io_service_.run();
}

void Server::stop(bool immediately){
	if (immediately) {
		handleStop();
	}
	else {
		// Post a call to the stop function so that server::stop() is safe to call from any thread.
		io_service_.post(bind(&Server::handleStop, this));
	}
}

void Server::handleAccept(const boost::system::error_code& e){
	if (!e){
		connection_manager.start(new_connection);
		new_connection.reset(new Connection(io_service_, connection_manager, request_handler));
		acceptor_.async_accept(
				new_connection->socket(),
				bind(&Server::handleAccept, this,
				asio::placeholders::error));
	}
}

void Server::handleStop(){
	// The server is stopped by cancelling all outstanding asynchronous operations.
	// Once all operations have finished the io_service::run() call will exit.
	acceptor_.close();
	connection_manager.stopAll();
}

} // namespace server
} // namespace http
