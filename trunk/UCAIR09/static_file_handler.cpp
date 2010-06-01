#include "static_file_handler.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "common_util.h"
#include "config.h"
#include "ucair_util.h"
#include "url_encoding.h"

using namespace std;
using namespace boost;

namespace mime_types {

/// mapping from file extensions to MIME types
struct mapping {
	const char* extension;
	const char* mime_type;
} mappings[] =
{
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "jpg", "image/jpeg" },
	{ "js", "application/javascript" },
	{ "pdf", "application/pdf" },
	{ "png", "image/png" },
	{ "svg", "image/svg+xml" },
	{ "swf", "application/x-shockwave-flash" },
	{ "txt", "text/plain" },
	{ "xml", "text/xml" },
	{ NULL, NULL } // Marks end of list.
};

/*! \brief Returns MIME type given file extension.
 *
 *  \param extension_ file extension with or without leading dot.
 *  \return "application/octet-stream" if extension is not registered.
 */
string extensionToType(const string& extension_){
	string extension = trim_left_copy_if(extension_, is_any_of("."));
	for (mapping* m = mappings; m->extension; ++ m){
		if (m->extension == extension){
			return m->mime_type;
		}
	}

	return "application/octet-stream";
}

} // namespace mime_types

namespace ucair {

StaticFileHandler::StaticFileHandler() : static_prefix("/static") {
}

bool StaticFileHandler::initialize(){
	getUCAIRServer().registerHandler(RequestHandler::STATIC, static_prefix, bind(&StaticFileHandler::handleStaticFile, this, _1, _2));
	doc_root = util::getParam<string>(Main::instance().getConfig(), "doc_root");
	return true;
}

void StaticFileHandler::handleStaticFile(Request &request, Reply &reply){
	// Decode url to path.
	string path_decoded;
	if (! http::util::urlDecode(request.url_components.path, path_decoded)){
		getUCAIRServer().err(reply_status::bad_request);
	}
	if (starts_with(path_decoded, static_prefix)){
		erase_head(path_decoded, static_prefix.length());
	}

	filesystem::path path(path_decoded);

	// Request path must not contain ".." (for security reasons).
	if (find(path.begin(), path.end(), "..") != path.end()){
		getUCAIRServer().err(reply_status::bad_request);
	}

	// If path ends in slash (i.e. is a directory) then add "index.html".
	if (path.filename() == "/" || path.filename() == "."){
		path /= "index.html";
	}

	filesystem::path full_path = doc_root / path;

	// Only serves regular files (for security reasons).
	if (! is_regular_file(full_path)){
		getUCAIRServer().err(reply_status::not_found);
	}

	// Checks If-Modified-Since.
	time_t last_modified = filesystem::last_write_time(full_path);
	string s = request.getHeader("If-Modified-Since");
	if (! s.empty()){
		time_t if_modified_since = util::to_time_t(util::stringToTime(s, util::internet_time_format));
		if (last_modified == if_modified_since){
			reply.status = reply_status::not_modified;
			return;
		}
	}

	// Open the file to send back.
	ifstream fin(full_path.string().c_str(), ios::in | ios::binary);
	if (! fin){
		getUCAIRServer().err(reply_status::not_found);
	}

	// Fill out the reply to be sent to the client.
	util::readFile(fin, reply.content);
	reply.setHeader("Content-Type", mime_types::extensionToType(path.extension()));

	posix_time::ptime t = posix_time::from_time_t(last_modified);
	reply.setHeader("Last-Modified", util::timeToString(t, util::internet_time_format));
}

} // namespace ucair
