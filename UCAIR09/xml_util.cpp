#include "xml_util.h"
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/regex.hpp>
#include "xml_dom.h"

using namespace std;
using namespace boost;

namespace xml {

namespace util {

typedef iterator_range<string::const_iterator> string_const_range;

namespace entities {

const string characters = "><\"'&";
const string references[] = {"&gt;", "&lt;", "&quot;", "&apos;", "&amp;"};

string_const_range formatter(const string_const_range &match){
	if (! match.empty()){
		for (size_t i = 0; i < characters.size(); ++ i){
			if (match.front() == characters[i]){
				return string_const_range(references[i].begin(), references[i].end());
			}
		}
	}
	return match;
}

} // namespace entities

namespace doc_types {

const string no_doc_type;

const string html_4_01_strict = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">";

const string html_4_01_loose = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">";

const string html_4_01_frameset = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">";

const string xhtml_1_0_strict = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">";

const string xhtml_1_0_transitional = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";

const string xhtml_1_0_frameset = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">";

const string& toString(HTMLDocType doc_type){
	switch (doc_type){
		case HTML_4_01_STRICT:
			return html_4_01_strict;
		case HTML_4_01_LOOSE:
			return html_4_01_loose;
		case HTML_4_01_FRAMESET:
			return html_4_01_frameset;
		case XHTML_1_0_STRICT:
			return xhtml_1_0_strict;
		case XHTML_1_0_TRANSITIONAL:
			return xhtml_1_0_transitional;
		case XHTML_1_0_FRAMESET:
			return xhtml_1_0_frameset;
		default:
			return no_doc_type;
	}
}

} // namespace doc_types

string quote(const string &input){
	return find_format_all_copy(input, token_finder(is_any_of(entities::characters)), entities::formatter);
}

string unquote(const string &input){
	// inefficient
	string output = input;
	for (size_t i = 0; i < entities::characters.size(); ++ i){
		replace_all(output, string_const_range(entities::references[i].begin(), entities::references[i].end()), string_const_range(entities::characters.begin() + i, entities::characters.begin() + i + 1));
	}
	return output;
}

string makeHTML(dom::Node &html_root, HTMLDocType doc_type, bool expand_closing_tag){
	if (doc_type == XHTML_1_0_STRICT || doc_type == XHTML_1_0_TRANSITIONAL || doc_type == XHTML_1_0_FRAMESET){
		html_root.setAttr("xmlns", "http://www.w3.org/1999/xhtml");
	}
	string html_source = html_root.toString();
	if (expand_closing_tag){ // change <x /> into <x></x>
	    static const regex re("<([^>\\s]+)(\\s[^>]*)/>");
		html_source = regex_replace(html_source, re, "<$1$2></$1>");
	}
	if (doc_type != NO_DOC_TYPE){
		html_source = doc_types::toString(doc_type) + "\n" + html_source;
	}
	return html_source;
}

string makeHTML(const string &orig_source, HTMLDocType doc_type){
	string html_source = orig_source;
	if (doc_type == XHTML_1_0_STRICT || doc_type == XHTML_1_0_TRANSITIONAL || doc_type == XHTML_1_0_FRAMESET){
		replace_first(html_source, "<html>", "<html xmlns=\"http://www.w3.org/1999/xhtml\">");
	}
	if (doc_type != NO_DOC_TYPE){
		html_source = doc_types::toString(doc_type) + "\n" + html_source;
	}
	return html_source;
}

} // namespace util
} // namespace xml
