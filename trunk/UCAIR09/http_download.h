#ifndef __http_download_h__
#define __http_download_h__

#include <string>

namespace http {
namespace util {

/*! \brief Downloads a document from a given URL.
 *
 *  Only implements an HTTP/1.0 client.
 *  \param[in] url URL
 *  \param[out] content document content
 *  \param[out] error_msg error message, empty if no error
 *  \param[in] follow_redirect whether to follow redirect (just once)
 *  \return true if succeeded
 */
bool downloadPage(const std::string &url, std::string &content, std::string &err_msg, bool allow_redirect = true);

} // namespace util
} // namespace http

#endif
