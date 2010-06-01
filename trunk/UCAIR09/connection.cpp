#include "connection.h"
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include "connection_manager.h"

using namespace std;
using namespace boost;

namespace http {
namespace server {

const int MAX_CONTENT_LENGTH = 8 * 1024;

Connection::Connection(asio::io_service& io_service, ConnectionManager& manager, const RequestHandler& handler):
	socket_(io_service),
	connection_manager(manager),
	request_parser(MAX_CONTENT_LENGTH),
	request_handler(handler)
{
}

asio::ip::tcp::socket& Connection::socket(){
	return socket_;
}

void Connection::start(){
	socket_.async_read_some(
			asio::buffer(buffer),
			bind(&Connection::handleRead, shared_from_this(),
			asio::placeholders::error,
			asio::placeholders::bytes_transferred));
}

void Connection::stop(){
	socket_.close();
}

void Connection::handleRead(const boost::system::error_code& e, size_t bytes_transferred){
	if (!e){
		tribool result;
		tie(result, tuples::ignore) = request_parser.parse(request, buffer.data(), buffer.data() + bytes_transferred);

		if (result){
			request_handler(request, reply);
			asio::async_write(
					socket_,
					reply.toBuffers(),
					bind(&Connection::handleWrite, shared_from_this(), asio::placeholders::error));
		}
		else if (!result){
			reply.status = reply_status::bad_request;
			asio::async_write(
					socket_,
					reply.toBuffers(),
					bind(&Connection::handleWrite, shared_from_this(), asio::placeholders::error));
		}
		else{
			socket_.async_read_some(
					asio::buffer(buffer),
					bind(&Connection::handleRead,shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
		}
	}
	else if (e != asio::error::operation_aborted){
		connection_manager.stop(shared_from_this());
	}
}

void Connection::handleWrite(const boost::system::error_code& e){
	if (!e){
		// Initiate graceful connection closure.
		boost::system::error_code ignored_e;
		socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_e);
	}

	if (e != asio::error::operation_aborted){
		connection_manager.stop(shared_from_this());
	}
}

} // namespace server
} // namespace http
