/*! \file XML utility functions */

#ifndef __xml_util_h__
#define __xml_util_h__

#include <string>

namespace xml {

namespace dom {

class Node;

} // namespace dom

namespace util {

/// Escapes XML entities.
std::string quote(const std::string &input);
/// Unescapes XML entities.
std::string unquote(const std::string &input);

/// HTML doc type
enum HTMLDocType {
	NO_DOC_TYPE,
	HTML_4_01_LOOSE,
	HTML_4_01_STRICT,
	HTML_4_01_FRAMESET,
	XHTML_1_0_TRANSITIONAL,
	XHTML_1_0_STRICT,
	XHTML_1_0_FRAMESET
};

/*! \brief Returns HTML string from a DOM tree.
 *  
 *  This invokes dom::Node::toString, but adds some extra processing.
 *  \param html_root root node of the DOM tree
 *  \param doc_type HTML doc type to prepend to the string
 *  \param expand_closing_tag whether to expand <x /> to <x></x> (needed for the <script> tag)
 *  \return HTMl string
 */
std::string makeHTML(dom::Node &html_root, HTMLDocType doc_type = NO_DOC_TYPE, bool expand_closing_tag = true);

/// Adds HTML doc type to HTML source.
std::string makeHTML(const std::string &orig_source, HTMLDocType doc_type);

} // namespace util
} // namespace xml

#endif
