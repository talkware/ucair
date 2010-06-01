/*! \file url_encoding.h
 *  \brief Utilities to perform URL percent encoding/decoding.
 *
 *  http://en.wikipedia.org/wiki/Percent-encoding
 */

#ifndef __url_encoding_h__
#define __url_encoding_h__

#include <string>

namespace http {
namespace util {

/*! \brief URL percent encoding.
 *  \param[in] in raw URL
 *  \param[out] out encoded URL
 */
void urlEncode(const std::string &in, std::string &out);

/*! \brief URL percent decoding.
 *  \param[in] in encoded URL
 *  \param[out] out raw URL
 *  \return false if encoding is invalid
 */
bool urlDecode(const std::string &in, std::string &out);

} // namespace util
} // namespace http

#endif
