#ifndef __logger_h__
#define __logger_h__

#include <fstream>
#include <string>
#include "component.h"
#include "main.h"

namespace ucair {

/*! \brief Logs information to file.
 *
 *  I could have used log4cxx, but it's too complex.
 */
class Logger : public Component {
public:
	bool initialize();
	bool finalize();

	void debug(const std::string &message) { log("debug", message); }
	void info(const std::string &message) { log("info", message); }
	void error(const std::string &message) { log("error", message); }
	void fatal(const std::string &message) { log("fatal", message); }

private:
	void log(const std::string &level, const std::string &message);
	std::fstream fout;
};

DECLARE_GET_COMPONENT(Logger);

} // namespace ucair

#endif
