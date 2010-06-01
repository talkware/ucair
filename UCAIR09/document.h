#ifndef __document_h__
#define __document_h__

#include <ctime>
#include <string>
#include <iostream>
#include "properties.h"

namespace ucair {

class Document {
public:
	Document();
	~Document();

	std::string doc_id;

	std::string title;
	std::string formatted_title; ///< HTML-formatted title (usually with bolded keywords)

	/*! \brief document url
     *
	 *  e.g. http://www.uiuc.edu
	 */
	std::string url;
	/*! \brief url for text display
	 *
	 *  e.g. www.uiuc.edu (notice the skipped http:// part)
	 */
	std::string display_url;
	std::string formatted_display_url; ///< HTML-formatted url (usually with bolded keywords)
	/*! \brief url for clickable link
	 *
	 *  e.g. http://rds.yahoo.com/... which redirects to http://www.uiuc.edu after the search engine intercepts the click
	 */
	std::string click_url;
	std::string cache_url; ///< url of cached result

	std::string summary;
	std::string formatted_summary; ///< HTML-formatted summary (usually with bolded keywords)

	std::string source; ///< document source (where it comes from)
	time_t date;
	std::string mime_type; ///< document mime type (e.g. html, pdf)
	util::Properties attrs;

friend std::ostream& operator << (std::ostream &out, const Document &doc);
};

} // namespace ucair

#endif
