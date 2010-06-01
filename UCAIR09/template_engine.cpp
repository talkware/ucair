#include "template_engine.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "common_util.h"
#include "xml_util.h"

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

namespace templating {

////////////////////////////////////////////////////////////////////////////////

class TemplateContext {
public:
	bool get(const string &name, string &value) const;

	map<string, string> vars;
	multimap<string, shared_ptr<TemplateDataNode> > data_nodes;
};

bool TemplateContext::get(const string &name, string &value) const {
	map<string, string>::const_iterator itr = vars.find(name);
	if (itr != vars.end()) {
		value = itr->second;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

/*! \brief The hierarchy of template data is implemented as a tree of TemplateDataNodes, where child nodes correspond to template child data.
 *
 *  Smart pointers are used so that the tree releases memory itself.
 */
class TemplateDataNode {
public:
	TemplateDataNode(): parent(NULL) {}

	void set(const string &name, const string &value);
	bool get(const string &name, string &value) const;
	void addChild(const string &name, const shared_ptr<TemplateDataNode> &child);

	TemplateContext getContext(const TemplateContext &parent_context) const;
	TemplateContext getContext() const;

private:
	map<string, string> vars; /// mapping from names to values
	multimap<string, shared_ptr<TemplateDataNode> > children; /// mapping from names to children nodes
	TemplateDataNode *parent; /// parent node
	std::string node_name;
};

void TemplateDataNode::set(const string &name, const string &value){
	vars[name] = value;
}

bool TemplateDataNode::get(const string &name, string &value) const {
	map<string, string>::const_iterator itr = vars.find(name);
	if (itr != vars.end()){
		value = itr->second;
		return true;
	}
	return false;
}

void TemplateDataNode::addChild(const string &name, const shared_ptr<TemplateDataNode> &child) {
	assert(! child->parent); // only one parent allowed
	child->node_name = name;
	child->parent = this;
	children.insert(make_pair(name, child));
}

TemplateContext TemplateDataNode::getContext(const TemplateContext &parent_context) const {
	TemplateContext context;
	context.vars = parent_context.vars;
	for (map<string, string>::const_iterator itr = vars.begin(); itr != vars.end(); ++ itr) {
		context.vars[itr->first] = itr->second;
	}
	context.data_nodes = children;
	for (multimap<string, shared_ptr<TemplateDataNode> >::const_iterator itr = parent_context.data_nodes.begin(); itr != parent_context.data_nodes.end(); ++ itr) {
		if (itr->first != node_name && children.find(itr->first) == children.end()) {
			context.data_nodes.insert(*itr);
		}
	}
	return context;
}

TemplateContext TemplateDataNode::getContext() const {
	TemplateContext parent_context;
	if (parent) {
		parent_context = parent->getContext();
	}
	return getContext(parent_context);
}

////////////////////////////////////////////////////////////////////////////////

TemplateData::TemplateData() : root(new TemplateDataNode) {
}

TemplateData& TemplateData::set(const string &name, const string &value, bool xml_quote) {
	root->set(name, xml_quote ? xml::util::quote(value) : value);
	return *this;
}

bool TemplateData::get(const string &name, string &value) const {
	return root->get(name, value);
}

TemplateData TemplateData::addChild(const string &name) {
	TemplateData child;
	root->addChild(name, child.root);
	return child;
}

////////////////////////////////////////////////////////////////////////////////

/*! \brief A node in the template source tree.
 *
 *  Template source is read from file and parsed into a tree of template source nodes.
 *  For different template source constructs there are different types of template source nodes,
 *  all deriving from this base class.
 */
class TemplateSourceNode {
public:
	virtual ~TemplateSourceNode() {}

	/*! \brief Renders template data into an output string.
	 *
	 *  The base implementation renders its children sequentially.
	 *  \param data template data
	 *  \return output string
	 */
	virtual string render(const TemplateContext &context) const;

	vector<shared_ptr<TemplateSourceNode> > children; ///< children nodes
};

string TemplateSourceNode::render(const TemplateContext &context) const {
	ostringstream strout;
	BOOST_FOREACH(const shared_ptr<TemplateSourceNode> &child, children) {
		strout << child->render(context);
	}
	return strout.str();
}

////////////////////////////////////////////////////////////////////////////////

/// Template source node for plain text that does not contain template specials.
class TextNode : public TemplateSourceNode {
public:
	TextNode(const string &text_) : text(text_) {}
	/// Returns the stored text.
	string render(const TemplateContext &context) const { return text; }

	string text;
};

////////////////////////////////////////////////////////////////////////////////

/// Template source node that holds a template variable (e.g. ${color})
class VarNode : public TemplateSourceNode {
public:
	VarNode(const string &name_) : name(name_) {}
	string render(const TemplateContext &context) const;

	string name; ///< variable name
};

/*! \brief Returns the named variable's value in the template data mapping.
 *
 *  If variable is not found, just return an empty string.
 */
string VarNode::render(const TemplateContext &context) const {
	string value;
	if (context.get(name, value)) {
		return value;
	}
	if (starts_with(name, "$")) {

	}
	return "";
}

////////////////////////////////////////////////////////////////////////////////

/// Template source node that corresponds to a "foreach" construct.
class ForeachNode : public TemplateSourceNode {
public:
	ForeachNode(const string &name_) : name(name_) {}
	string render(const TemplateContext &context) const;

	string name; ///< name of sub template data to look up
};

/*! \brief Returns the output of a "foreach" construct.
 *
 *  For each sub template data baring the given name,
 *  their output will be generated and concatenated together.
 */
string ForeachNode::render(const TemplateContext &context) const {
	ostringstream strout;

	typedef multimap<string, shared_ptr<TemplateDataNode> >::const_iterator Iterator;
	pair<Iterator, Iterator> p = context.data_nodes.equal_range(name);
	for (Iterator itr = p.first; itr != p.second; ++ itr) {
		const TemplateDataNode &data_node = *itr->second;
		TemplateContext child_context = data_node.getContext(context);
		strout << TemplateSourceNode::render(child_context);
	}
	return strout.str();
}

////////////////////////////////////////////////////////////////////////////////

class IfNode: public TemplateSourceNode {
public:
	IfNode(const string &name_, const string &value_, const string &op_) : name(name_), value(value_), test(op_) {}
	string render(const TemplateContext &context) const;
	bool doTest(const TemplateContext &context) const;

	string name;
	string value;
	string test;
};

bool IfNode::doTest(const TemplateContext &context) const {
	string actual_value;
	bool has_value = context.get(name, actual_value);
	bool has_data_node = context.data_nodes.find(name) != context.data_nodes.end();
	if (test == "eq") {
		return actual_value == value;
	}
	else if (test == "neq") {
		return actual_value != value;
	}
	else if (test == "exist") {
		return has_value || has_data_node;
	}
	else if (test == "nexist") {
		return ! has_value && ! has_data_node;
	}
	else if (test == "empty") {
		return ! has_value || actual_value.empty();
	}
	else if (test == "nempty") {
		return has_value && ! actual_value.empty();
	}
	else {
		throw Error() << ErrorInfo("Unknown test: " + test);
	}
}

string IfNode::render(const TemplateContext &context) const {
	if (doTest(context)) {
		return TemplateSourceNode::render(context);
	}
	return "";
}

////////////////////////////////////////////////////////////////////////////////

/*! \brief Template source node that corresponds to a clause in the "switch" construct.
 *
 *  It can be either a "case" clause or a "default" clause.
 *  CaseNode should always have a SwitchNode as its parent.
 */
class CaseNode : public TemplateSourceNode {
public:

	/// If no input param, constructs a "default" clause.
	CaseNode() : default_case(true) {}

	/// If given a value, constructs a "when" clause conditioned on that value.
	CaseNode(const string &value_) : value(value_), default_case(false) {}

	string value; ///< value expected by this case
	bool default_case; ///< if true, this is a "default" case
};

/*! \brief Template source node that corresponds to a "switch" construct.
 *
 *  SwitchNode should have only CaseNode as its children.
 */
class SwitchNode : public TemplateSourceNode {
public:
	SwitchNode(const string &name_) : name(name_) {}
	string render(const TemplateContext &context) const;

	string name; /// name of the variable whose value is checked in the "switch" clauses.
};

/*! \brief Returns the output of a "switch" construct.
 *
 *  The control is much like the C switch statement,
 *  except it automatically "break" out of the switch statement after executing a matching case.
 *  If variable is not found and there is no default case, just return an emtpy string.
 */
string SwitchNode::render(const TemplateContext &context) const {
	string value;
	bool has_value = context.get(name, value);
	BOOST_FOREACH(const shared_ptr<TemplateSourceNode> &child_, children) {
		shared_ptr<CaseNode> child = dynamic_pointer_cast<CaseNode, TemplateSourceNode>(child_);
		if (child && (child->default_case || has_value && value == child->value)) {
			return child->render(context);
		}
	}
	return "";
}

////////////////////////////////////////////////////////////////////////////////

class IncludeNode : public TemplateSourceNode {
public:
	IncludeNode(const string &source) : source_name(source) {}
	string render(const TemplateContext &context) const;

	string source_name;
};

string IncludeNode::render(const TemplateContext &context) const {
	TemplateSource source = getTemplateEngine().getSource(source_name);
	if (source.root) {
		return source.root->render(context);
	}
	return "";
}

////////////////////////////////////////////////////////////////////////////////

/*! \brief Parses the "attr_name = attr_value" pairs in a character sequence.
 *  \param begin sequence begin pos
 *  \param end sequence end pos
 *  \return mapping from attr name to attr value
 */
map<string, string> parseAttrs(string::const_iterator begin, string::const_iterator end){
	map<string, string> attrs;
	static const regex re("\\s+(\\w+)\\s*=\\s*\"([^\"]*?)\"");
	smatch results;
	while (true){
		if (! regex_search(begin, end, results, re)){
			break;
		}
		attrs[results[1]] = xml::util::unquote(results[2]);
		begin = results.suffix().first;
	}
	return attrs;
}

/*! \brief Trims unnecessary whitespaces in template source.
 *
 *  If a template tag occupies a whole line, then the white spaces (blanks, tabs, new lines) are removed.
 *  \param source input
 *  \return trimed output
 */
string trimSource(const string &source) {
	static const regex re("^\\s*(</?template:\\w+[^>]*?>|\\$\\{(\\w+)\\})\\s*$");
	istringstream strin(source);
	ostringstream strout;
	string line;
	while (getline(strin, line)) {
		smatch results;
		if (regex_match(line, results, re)) {
			strout << trim_copy_if(line, is_any_of(" \t"));
		}
		else {
			strout << line << endl;
		}
	}
	return strout.str();
}

////////////////////////////////////////////////////////////////////////////////

TemplateSource::TemplateSource(const string &source_name_, const string &source) : source_name(source_name_) {
	static const regex re("<template:main\\s?[^>]*?>");
	smatch results;
	if (! regex_search(source, results, re)) {
		throw Error() << ErrorInfo(source_name + ": Cannot find <template:main>");
	}
	string trimmed_source = trimSource(source);
	string::const_iterator begin = trimmed_source.begin(), end = trimmed_source.end();
	root = recursiveParse(begin, end);
}

string TemplateSource::render(const TemplateData &data) const {
	TemplateContext context = data.root->getContext();
	return root->render(context);
}

shared_ptr<TemplateSourceNode> TemplateSource::recursiveParse(string::const_iterator &begin, string::const_iterator &end) {
	// matches an open tag
	static const regex re1("<(template:\\w+)[^>]*?(/)?>");
	// matches an open tag, a closing tag, or a ${var}
	static const regex re2("(<(template:\\w+)[^>]*?>)|"
						   "(</(template:\\w+)>)|"
						   "(\\$\\{(\\w+)\\})");
	// Find the first open tag.
	smatch results;
	if (! regex_search(begin, end, results, re1)){
		throw Error() << ErrorInfo(source_name + ": Cannot find <template:...>");
	}
	string open_tag_name = results[1];
	map<string, string> attrs = parseAttrs(results[0].first, results[0].second);
	shared_ptr<TemplateSourceNode> node;
	/// Choose the correct TemplateSourceNode type according to tag name
	if (open_tag_name == "template:main"){
		node.reset(new TemplateSourceNode());
	}
	else if (open_tag_name == "template:if") {
		map<string, string>::const_iterator itr = attrs.find("name");
		if (itr == attrs.end()) {
			throw Error() << ErrorInfo(source_name + ": <template:if> requires attribute \"name\"");
		}
		const string &name = itr->second;
		itr = attrs.find("test");
		if (itr == attrs.end()) {
			throw Error() << ErrorInfo(source_name + ": <template:if> requires attribute \"test\"");
		}
		const string &test = itr->second;
		string value;
		itr = attrs.find("value");
		if (itr != attrs.end()) { // value is optional
			value = itr->second;
		}
		node.reset(new IfNode(name, value, test));
	}
	else if (open_tag_name == "template:switch") {
		map<string, string>::const_iterator itr = attrs.find("name");
		if (itr == attrs.end()) {
			throw Error() << ErrorInfo(source_name + ": <template:switch> requires attribute \"name\"");
		}
		const string &name = itr->second;
		node.reset(new SwitchNode(name));
	}
	else if (open_tag_name == "template:case") {
		map<string, string>::const_iterator itr = attrs.find("value");
		if (itr == attrs.end()) {
			throw Error() << ErrorInfo(source_name + ": <template:case> requires attribute \"value\"");
		}
		const string &value = itr->second;
		node.reset(new CaseNode(value));
	}
	else if (open_tag_name == "template:default") {
		node.reset(new CaseNode());
	}
	else if (open_tag_name == "template:foreach") {
		map<string, string>::const_iterator itr = attrs.find("name");
		if (itr == attrs.end()) {
			throw Error() << ErrorInfo(source_name + ": <template:foreach> requires attribute \"name\"");
		}
		const string &name = itr->second;
		node.reset(new ForeachNode(name));
	}
	else if (open_tag_name == "template:include") {
		map<string, string>::const_iterator itr = attrs.find("source");
		if (itr == attrs.end()) {
			throw Error() << ErrorInfo(source_name + ": <template:include> requires attribute \"source\"");
		}
		const string &source_file_name = itr->second;
		node.reset(new IncludeNode(source_file_name));
	}
	else {
		throw Error() << ErrorInfo(source_name + ": <" + open_tag_name + "> is unknown");
	}
	begin = results.suffix().first; // right side of <template:...>

	if (results[2].matched) {
		// <template:... />
		return node;
	}

	while (true){
		if (! regex_search(begin, end, results, re2)){
			throw Error() << ErrorInfo(source_name + ": <" + open_tag_name + "> is not closed");
		}
		// Before handling the next open/closing tag or variable, we need to handle the text that comes before them.
		string text = results.prefix();
		if (! text.empty()){
			node->children.push_back(shared_ptr<TemplateSourceNode>(new TextNode(text)));
			begin = results.prefix().second; // right side of text
		}
		if (results[1].matched){
			// another open tag: recursively parse it
			shared_ptr<TemplateSourceNode> child = recursiveParse(begin, end);
			if (child) {
				node->children.push_back(child);
			}
			// begin is now at the right side of the close tag of the child element
		}
		else if (results[3].matched){
			// closing tag
			string closing_tag_name = results[4];
			if (open_tag_name != closing_tag_name){
				throw Error() << ErrorInfo(source_name + ": Mismatch between <" + open_tag_name + "> and </" + closing_tag_name + ">");
			}
			begin = results.suffix().first; // right side of </template:...>
			return node;
		}
		else if (results[5].matched){
			/// variable
			string var_name = results[6];
			node->children.push_back(shared_ptr<TemplateSourceNode>(new VarNode(var_name)));
			begin = results.suffix().first; // right side of ${var}
		}
		else{
			// Should not reach here.
			assert(false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

TemplateEngine::TemplateEngine(const string &source_dir_, bool detect_changes_) :
	source_dir(source_dir_),
	detect_changes(detect_changes_)
{
}

TemplateSource TemplateEngine::getSource(const string &source_name) {
	fs::path source_path(source_dir);
	source_path /= source_name;
	time_t last_write_time;
	map<string, pair<TemplateSource, time_t> >::iterator itr = cache.find(source_name);
	try {
		if (itr != cache.end()) {
			// found in cache
			if (! detect_changes) {
				// not detecting file changes: just return cached copy
				return itr->second.first;
			}
			last_write_time = fs::last_write_time(source_path);
			if (itr->second.second == last_write_time) {
				// file has not been changed since last time: just return cached copy
				return itr->second.first;
			}
		}
		else {
			last_write_time = fs::last_write_time(source_path);
		}
	}
	catch (fs::filesystem_error &e) {
		throw Error() << ErrorInfo(e.what());
	}
	ifstream fin(source_path.string().c_str());
	if (! fin) {
		throw Error() << ErrorInfo(source_name + ": Cannot find template file at " + source_path.string());
	}
	string content;
	util::readFile(fin, content);
	TemplateSource source(source_name, content);
	if (itr != cache.end()) {
		// update cache entry
		itr->second = make_pair(source, last_write_time);
	}
	else {
		// create new cache entry
		cache.insert(make_pair(source_name, make_pair(source, last_write_time)));
	}
	return source;
}

string TemplateEngine::render(const TemplateData &data, const string &source_name) {
	TemplateSource source = getSource(source_name);
	return source.render(data);
}

} // namespace templating
