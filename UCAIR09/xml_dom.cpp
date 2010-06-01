#include "xml_dom.h"
#include <cassert>
#include <boost/algorithm/string.hpp>
using namespace boost;
using namespace std;

namespace xml {

namespace dom {

Node::Node(): _node(NULL), self_destroy(false) {}

Node::Node(mxml_node_t *node): _node(node), self_destroy(false) {}

Node::Node(const string &s): _node(NULL), self_destroy(false) {
	fromString(s);
}

Node::~Node(){
	if (self_destroy){
		destroy();
	}
}

Node Node::newElement(const string &name){
	return Node(mxmlNewElement(MXML_NO_PARENT, name.c_str()));
}

Node Node::newElement(Node &parent, const string &name){
	return Node(mxmlNewElement(parent._node, name.c_str()));
}

Node Node::newText(const string &text, bool useCDATA){
	return Node(useCDATA ? mxmlNewCDATA(MXML_NO_PARENT, text.c_str()) : mxmlNewOpaque(MXML_NO_PARENT, text.c_str()));
}

Node Node::newText(Node &parent, const string &text, bool useCDATA){
	return Node(useCDATA ? mxmlNewCDATA(parent._node, text.c_str()) : mxmlNewOpaque(parent._node, text.c_str()));
}

Node Node::newXML(){
	return Node(mxmlNewXML("1.0"));
}

Node::NodeType Node::getNodeType() const {
	if (!_node){
		return NONE;
	}
	if (_node->type == MXML_ELEMENT){
		string data = _node->value.element.name;
		if (starts_with(data, "![CDATA[") && ends_with(data, "]]")){
			return TEXT;
		}
		else{
			return ELEMENT;
		}
	}
	else if (_node->type == MXML_OPAQUE){
		return TEXT;
	}
	else{
		return OTHER;
	}
}

void Node::setElementName(const string &name){
	assert(getNodeType() == ELEMENT);
	mxmlSetElement(_node, name.c_str());
}

void Node::setText(const string &text){
	assert(getNodeType() == TEXT);
	if (_node->type == MXML_OPAQUE){
		mxmlSetOpaque(_node, text.c_str());
	}
	else{
		mxmlSetCDATA(_node, text.c_str());
	}
}

string Node::getElementName() const {
	assert(getNodeType() == ELEMENT);
	return _node->value.element.name;
}

string Node::getText() const {
	assert(getNodeType() == TEXT);
	if (_node->type == MXML_OPAQUE){
		return _node->value.opaque;	
	}
	else{
		string s = _node->value.element.name;	
		return s.substr(8, s.size() - 10); // remove ![CDATA[ and ]]
	}
}

void Node::fromString(const string &s){
	assert(!_node);
	string t = trim_copy_if(s, is_any_of(" \t\n\r"));
	_node = mxmlLoadString(NULL, t.c_str(), MXML_OPAQUE_CALLBACK);
}

string Node::toString() const{
	assert(_node);
	string s;
	char *p = mxmlSaveAllocString(_node, MXML_NO_CALLBACK);	
	if (p){
		s = p;
		free(p);
	}
	return s;
}

void Node::addChild(Node &child, AddType add_type){
	assert(getNodeType() == ELEMENT);
	mxmlAdd(_node, add_type, MXML_ADD_TO_PARENT, child._node);
}

void Node::addChild(Node &child, Node &reference, AddType add_type){
	assert(getNodeType() == ELEMENT);
	mxmlAdd(_node, add_type, reference._node, child._node);
}

void Node::removeChild(Node &child){
	assert(child.parent() == *this);
	mxmlRemove(child._node);
}

void Node::destroy(){
	if (_node){
		mxmlDelete(_node);
		_node = NULL;
	}
}

void Node::removeAttr(const string &name){
	assert(getNodeType() == ELEMENT);
	mxmlElementDeleteAttr(_node, name.c_str());
}

bool Node::getAttr(const string &name, string &value) const {
	assert(getNodeType() == ELEMENT);
	const char *p = mxmlElementGetAttr(_node, name.c_str());
	if (p){
		value = p;
		return true;
	}
	return false;
}

void Node::setAttr(const string &name, const string &value){
	assert(getNodeType() == ELEMENT);
	mxmlElementSetAttr(_node, name.c_str(), value.c_str());
}

Node Node::walkNext(const Node &current, DescendType descend) const {
	assert(_node);
	return Node(mxmlWalkNext(current._node, _node, descend));
}

Node Node::walkPrev(const Node &current, DescendType descend) const {
	assert(_node);
	return Node(mxmlWalkPrev(current._node, _node, descend));
}

Node Node::findElement(const Node &current, const string &name, const string &attr_name, const string &attr_value, DescendType descend) const {
	assert(_node);
	const char *p_name = name.empty() ? NULL : name.c_str();
	const char *p_attr_name = attr_name.empty() ? NULL : attr_name.c_str();
	const char *p_attr_value = attr_value.empty() ? NULL : attr_value.c_str();
	return Node(mxmlFindElement(current._node, _node, p_name, p_attr_name, p_attr_value, descend));
}

string Node::getInnerText() const {
	string text;
	Node child = walkNext(*this, DESCEND);
	while (child){
		if (child.getNodeType() == TEXT){
			text += child.getText();
		}
		child = walkNext(child, DESCEND);
	}
	return text;
}

} // namespace dom
} // namespace xml
