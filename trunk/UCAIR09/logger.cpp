#include "logger.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include "common_util.h"
#include "config.h"
#include "ucair_util.h"

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

namespace ucair {

bool Logger::initialize() {
	fs::path path(ucair::getProgramDataDir());
	path /= "log";
	string log_file_name = path.string();
	fout.open(log_file_name.c_str(), ios::out | ios::app);
	if (! fout) {
		return false;
	}
	return true;
}

bool Logger::finalize() {
	fout.close();
	return true;
}

void Logger::log(const string &level, const string &message) {
	posix_time::ptime now = posix_time::microsec_clock::local_time();
	string line = str(format("%s %-5s %s") % util::timeToString(now, "%Y-%m-%d %H:%M:%S") % to_upper_copy(level) % message);
	cerr << line << endl;
	if (fout) {
		fout << line << endl;
		fout.flush();
	}
}

} // namespace ucair
