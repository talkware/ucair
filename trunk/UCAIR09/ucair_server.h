#ifndef __ucair_server_h__
#define __ucair_server_h__

#include <ctime>
#include <map>
#include <list>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/signal.hpp>
#include <boost/smart_ptr.hpp>
#include "component.h"
#include "delayed_signal.h"
#include "main.h"
#include "request.h"
#include "reply.h"
#include "server.h"
#include "url_components.h"
#include "xml_dom.h"
#include "xml_util.h"

namespace ucair {

/// HTTP request. Provides more information than http::server::Request
class Request {
public:
	/// Takes an http::server::Request to construct.
	explicit Request(http::server::Request &req);

	std::string &method; ///< GET or POST
	std::string &url; ///< URL
	int &http_version_major; ///< HTTP major version
	int &http_version_minor; ///< HTTP minor version
	std::multimap<std::string, std::string> &headers; ///< headers (name to value)
	std::string &content; ///< request body

	http::util::URLComponents url_components; ///< url components

	/// Returns the value for a named header.
	std::string getHeader(const std::string &name) const;

	/// form data stored as name-value mapping, including data sent using GET and POST
	std::multimap<std::string, std::string> form_data;
	/*! \brief Returns single-valued form data
	 *  \param name form data name
	 *  \return form data value, empty string if name not found
	 */
	std::string getFormData(const std::string &name) const;
	/*! \brief Returns multi-valued form data
	 *
	 *  This is suitable for <input type="checkbox"> and other situations.
	 *  \param name form data name
	 *  \return all form data values with the given name, empty if name not found
	 */
	std::vector<std::string> getFormDataMultiValued(const std::string &name) const;

	/// cookie data stored as name-value mapping
	std::map<std::string, std::string> cookie_data;
	/*! \brief Returns cookie
	 *  \param name cookie name
	 *  \return form cookie value, empty string if name not found
	 */
	std::string getCookie(const std::string &name) const;

	time_t timestamp; ///< timestamp of the request

private:

	http::server::Request &req;
};

namespace reply_status = http::server::reply_status;

/// HTTP reply. Provides more information than http::server::Reply.
class Reply {
public:
	/// Takes an http::server::Reply to construct.
	explicit Reply(http::server::Reply &rep);

	int status; ///< reply status
	std::multimap<std::string, std::string> headers; ///< headers (name to value)
	std::string content; ///< reply body

	xml::dom::Node dom_root; ///< root of the DOM tree
	xml::util::HTMLDocType doc_type; ///< doc type

	/*! \brief internal redirect path, no redirect if empty
	 *  \sa UCAIRServer::internalRedirect
	 */
	std::string internal_redirect_path;
	/*! \brief external redirect url, no redirect if empty
	 *  \sa UCAIRServer::externalRedirect
	 */
	std::string external_redirect_url;

	/// Resets reply content (status and headers keep same).
	void reset();

	/*! Adds a header.
	 *
	 *  \param name header name
	 *  \param value header value
	 *  \param allow_duplicate if false, will override existing value if the name exists
	 */
	void setHeader(const std::string &name, const std::string &value,
			bool allow_duplicate = false);

	std::map<std::string, std::string> cookie_data;
	void setCookie(const std::string &name, const std::string &value);

private:
	http::server::Reply &rep;
};

struct RequestHandler {

	/// Interruption during request handling.
	typedef boost::error_info<struct tag_request_handler_interrupt, std::pair<
			int, std::string> > InterruptInfo;
	class Interrupt: public boost::exception, std::exception {
		const char* what() const throw () {
			return "Request handler interrupt";
		}
	};

	/// Whether the handler serves static files or produces dynamic CGI content. OTHER is reserved.
	enum Classification {
		STATIC, CGI_HTML, CGI_OTHER
	} classification;

	typedef boost::function<void(Request&, Reply&)> Callback;
	/// Callback function that accepts a request and produces a reply.
	Callback callback;

	/// This handler will be invoked if the prefix is the longest matching one of request path.
	std::string prefix;
};

class UCAIRServer: public Component {
public:

	UCAIRServer();

	bool initialize();

	/// Stops server.
	void stop();
	/// Starts server.
	void start();

	/*! \brief Registers a page handler at a particular location.
	 *  \param classification page handler type
	 *  \param prefix where to hook the page handler
	 *  \param callback callback function that accepts request and produces reply
	 */
	void registerHandler(
			const RequestHandler::Classification &classification,
			const std::string &prefix,
			const RequestHandler::Callback &callback);

	/*! \brief Redirects to an internal path.
	 *
	 *  This will throw an internal exception and abort execution of the current page handler.
	 */
	void internalRedirect(const std::string &new_path);
	/*! \brief Redirects to an external url.
	 *
	 *  This will throw an internal exception and abort execution of the current page handler.
	 */
	void externalRedirect(const std::string &new_url);

	/*! \brief Aborts execution of the current page handler and goes to an error page.
	 *  \param status HTTP status code to use for the error page
	 *  \param error_message message to show on the error page
	 */
	void err(int status, const std::string &error_message = "");

	/// Signal that indicates dispatch has completed.
	boost::signal<void(Request&, Reply&)> request_handled_signal;
	DelayedSignal idle_signal;

	/// Stops the UCAIR system (not just the server) after handling the current request.
	void shutdownSystem();

	std::string getAddress() const { return server_address; }
	std::string getPort() const {return server_port; }

private:

	/*! \brief Selects a page handler to dispatch for an incoming request.
	 *
	 *  The handler whose prefix has the longest match with the request path is invoked.
	 *  For example, if handler A is registered at /foo and handler B is registered at /foo/bar,
	 *  then for request /foo?x=y A is invoked while for request /foo/bar/fee?x=y B is invoked
	 */
	void dispatchRequest(http::server::Request &request,
			http::server::Reply &reply);

	/*! \brief Parses form data from request.
	 *
	 *  Looks at request URL in the case of GET and request body in the case of POST.
	 *  multipart/form-data is not supported as of now.
	 *  \return reply_status::ok if no parsing error, otherwise an HTTP error status code
	 */
	int parseFormData(Request &request);

	/// Extracts cookies from request.
	void parseCookieData(Request &request);

	/// Does some post-processing to produce an HTML page.
	void makeHTML(Reply &reply);

	std::list<RequestHandler> handlers; ///< list of request handler

	boost::scoped_ptr<http::server::Server> server; ///< wraps http::server::Server

	xml::util::HTMLDocType default_doc_type; ///< default HTML doc type

	std::string server_address;
	std::string server_port;
};

DECLARE_GET_COMPONENT(UCAIRServer)

} // namespace ucair

#endif
