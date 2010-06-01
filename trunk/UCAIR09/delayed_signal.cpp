#include "delayed_signal.h"
#include <boost/bind.hpp>

using namespace std;
using namespace boost;

namespace ucair {

DelayedSignal::DelayedSignal(asio::io_service &io_service):
	timer(io_service),
	fire_time(posix_time::not_a_date_time),
	active(false)
{
}

void DelayedSignal::waitTill(posix_time::ptime t){
	fire_time = t;
	if (! active){
		active = true;
		timer.expires_at(fire_time);
		timer.async_wait(bind(&DelayedSignal::handler, this, asio::placeholders::error));
	}
}

void DelayedSignal::waitFor(posix_time::time_duration t){
	waitTill(posix_time::microsec_clock::universal_time() + t);
}

void DelayedSignal::delayTill(posix_time::ptime t){
	fire_time = t;
}

void DelayedSignal::delayFor(posix_time::time_duration t){
	delayTill(posix_time::microsec_clock::universal_time() + t);
}

void DelayedSignal::handler(const boost::system::error_code &error){
	if (posix_time::microsec_clock::universal_time() >= fire_time){
		active = false;
		if (error != asio::error::operation_aborted){
			sig();
		}
	}
	else{
		timer.expires_at(fire_time);
		timer.async_wait(bind(&DelayedSignal::handler, this, asio::placeholders::error));
	}
}

} // namespace ucair
