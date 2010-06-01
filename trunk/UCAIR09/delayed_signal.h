#ifndef __delayed_signal_h__
#define __delayed_signal_h__

#include <boost/asio.hpp>
#include <boost/signal.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace ucair {

/// Extension of boost::signal, in that the signal can be scheduled to fire at a future time or pushed off.
class DelayedSignal{
public:
	DelayedSignal(boost::asio::io_service &io_service);

	/// Signal will be activated and fire at a given time.
	void waitTill(boost::posix_time::ptime t);
	/// Signal will be activated and fire after a given time duration.
	void waitFor(boost::posix_time::time_duration t);

	/// Signal will fire at a given time (if already activated).
	void delayTill(boost::posix_time::ptime t);
	/// Signal will fire after a given time duration (if already activated).
	void delayFor(boost::posix_time::time_duration t);

	boost::signal<void ()> sig;

private:
	void handler(const boost::system::error_code& error);
	boost::asio::deadline_timer timer;
	boost::posix_time::ptime fire_time;
	volatile bool active;
};

} // namespace ucair

#endif
