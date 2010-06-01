#ifndef __template_engine_h__
#define __template_engine_h__

#include <map>
#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/exception/all.hpp>
#include <boost/smart_ptr.hpp>

namespace templating {

/*! \brief Thrown when an error occurred in the templating engine.
 *
 *  To see error message, use the following code:
 *  \code
 	catch (templating::Error &e) {
		if (const string* error_info = boost::get_error_info<templating::ErrorInfo>(e)){
			cerr << *error_info;
		}
	}
 *  \endcode
 */
class Error: public std::exception, public boost::exception {
public:
	const char* what() const throw () { return "Templating engine error"; }
};
typedef boost::error_info<struct tag_templating_engine_error, std::string> ErrorInfo;

class TemplateDataNode;

class TemplateData {
public:

	TemplateData();

	/*! \brief Sets a mapping from name to value.
	 *  \param name name
	 *  \param value value
	 *  \param xml_escape whether to escape XML entities
	 *  \return self reference, so that set() can be chained
	 */
	TemplateData& set(const std::string &name, const std::string &value, bool xml_escape = true);

	/*! \brief Returns the value associated with a name.
	 *  \param[in] name name
	 *  \param[out] value value
	 *  \return false if name is not found
	 */
	bool get(const std::string &name, std::string &value) const;

	/*! \brief Attaches a child/sub template data.
	 *
	 *  \param name child name
	 *  \return child template data
	 */
	TemplateData addChild(const std::string &name);

private:

	boost::shared_ptr<TemplateDataNode> root;

friend class TemplateSource;
};

class TemplateSourceNode;

/// Template source that knows how to render template data.
class TemplateSource {
public:

	/*! \brief Renders template data.
	 *  \param data template data
	 *  \return rendered string
	 */
	std::string render(const TemplateData &data) const;

private:

	/*! \brief Constructs from a source string.
	 *  \throw Error if template cannot be parsed
	 */
	explicit TemplateSource(const std::string &source_name, const std::string &source);


	/*! \brief Parses the first <template:...> element in a character sequence.
	 *
	 *  If there are child elements, they are parsed recursively.
	 *  \param[in,out] begin sequence begin pos; will be updated to end of element after call
	 *  \param[in] end sequence end pos
	 *  \return the root node (corresponding to the first <template:...> element) of the template source tree
	 */
	boost::shared_ptr<TemplateSourceNode> recursiveParse(std::string::const_iterator &begin, std::string::const_iterator &end);

	std::string source_name;

	boost::shared_ptr<TemplateSourceNode> root; ///< root of the template source tree

friend class TemplateEngine;
friend class IncludeNode;
};

/// Template engine that manages template sources.
class TemplateEngine {
public:

	/*! \brief Constructor.
	 *  \param source_dir if not empty, from which directory to load source files
	 *  \param detect_change whether to reload template source if the source file has been changed
	 */
	TemplateEngine(const std::string &source_dir = "", bool detect_changes = true);

	/*! \brief Returns source parsed from a given file.
	 *  \param source_file_name template source file name
	 *  \return template source
	 *  \throw Error if template cannot be loaded or parsed
	 */
	TemplateSource getSource(const std::string &source_file_name);

	/*! \brief Loads source and uses it to render template data.
	 *
	 *  This is a shorthand for getSource() followed by TemplateSource::render().
	 *  \param data template data
	 *  \param source_file_name template source file name
	 *  \return rendered string
	 *  \throw Error if template cannot be loaded or parsed
	 */
	std::string render(const TemplateData &data, const std::string &source_file_name);

private:

	std::string source_dir;

	std::map<std::string, std::pair<TemplateSource, time_t> > cache;

	bool detect_changes;
};

/// Returns the global template engine.
extern TemplateEngine& getTemplateEngine();

} // namespace templating

#endif
