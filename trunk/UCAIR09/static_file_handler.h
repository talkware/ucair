#ifndef __static_file_handler_h__
#define __static_file_handler_h__

#include <string>
#include "component.h"
#include "ucair_server.h"
#include "main.h"

namespace ucair {

/// Handler of static files for the UCAIR server.
class StaticFileHandler: public Component {
public:

	StaticFileHandler();

	bool initialize();

	/// Page handler. Reads file content and use it for reply body.
	void handleStaticFile(Request &request, Reply &reply);

	std::string static_prefix; ///< where this handler is hooked. default is "/static"

	/*! \brief location of the root directory in file system.
	 *  For example, if static_prefix is /static and doc_root is C:/ucair,
	 *  then /static/foo/bar.htm will map to C:/ucair/foo/bar.htm
	 */
	std::string doc_root;
};

DECLARE_GET_COMPONENT(StaticFileHandler)

} // namespace ucair

#endif
