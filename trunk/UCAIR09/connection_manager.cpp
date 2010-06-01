#include "connection_manager.h"
#include <algorithm>
#include <boost/bind.hpp>

using namespace std;
using namespace boost;

namespace http {
namespace server {

void ConnectionManager::start(ConnectionPtr c){
	connections.insert(c);
	c->start();
}

void ConnectionManager::stop(ConnectionPtr c){
	connections.erase(c);
	c->stop();
}

void ConnectionManager::stopAll(){
	for_each(connections.begin(), connections.end(), bind(&Connection::stop, _1));
	connections.clear();
}

} // namespace server
} // namespace http
