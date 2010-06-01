#ifndef __xml_dom_h__
#define __xml_dom_h__

#include <string>
#include <mxml.h>

namespace xml{

namespace dom{

/*! \brief XML DOM node.
 *
 *  This is just a wrapper around mxml_node_t, so it's cheap to create.
 *  Check http://www.minixml.org for the Mini-XML API.
 */
class Node{
public:

	/// Constructs an empty node.
	Node();

	/// Wraps an mxml_node_t pointer.
	explicit Node(mxml_node_t *node);

	/*! \brief Constructs a node rooted at the DOM tree created from parsing a string.
	 *  \param s string to parse
	 */
	explicit Node(const std::string &s);

	~Node();

	enum NodeType {
		ELEMENT,
		TEXT,
		OTHER,
		NONE
	};

	enum AddType {
		ADD_BEFORE = MXML_ADD_BEFORE,
		ADD_AFTER = MXML_ADD_AFTER
	};

	enum DescendType {
		DESCEND = MXML_DESCEND,
		DESCEND_FIRST = MXML_DESCEND_FIRST,
		NO_DESCEND = MXML_NO_DESCEND
	};

	/// Returns true only if the underlying mxml_node_t pointers are equal.
	bool operator == (const Node &other) const { return _node == other._node; }

	/// Returns parent node, or null.
	Node parent() const { return Node(_node->parent); }
	/// Returns previous sibling, or null
	Node prevSibling() const { return Node(_node->prev); }
	/// Returns next sibling, or null
	Node nextSibling() const { return Node(_node->next); }
	/// Returns first child, or null
	Node firstChild() const { return Node(_node->child); }
	/// Returns last child, or null
	Node lastChild() const { return Node(_node->last_child); }

	/// Returns node type.
	NodeType getNodeType() const;

	/// Whether the DOM tree rooted at this node will get destroyed when the node gets destructed.
	void setSelfDestroy() { self_destroy = true; }
	/*! \brief Destroys the DOM tree rooted at this node.
	 *
	 *  If the node is already destroyed, later calls have no effect.
	 */
	void destroy();

	/*! \brief Adds a child node
	 *  \param child child node
	 *  \param add_type whether to insert as the first or last child node
	 */
	void addChild(Node &child, AddType add_type = ADD_AFTER);
	/*! \brief Adds a child node at a reference position.
	 *  \param child child node
	 *  \param reference an existing child node to serve as reference
	 *  \param add_type whether to insert before or after the reference
	 */
	void addChild(Node &child, Node &reference, AddType add_type = ADD_AFTER);
	/// Detaches a child (child is not destroyed).
	void removeChild(Node &child);

	/// Removes an attribute with a given name.
	void removeAttr(const std::string &name);
	/*! \brief Returns the attribute value given the attribute name.
	 *  \param[in] name attribute name
	 *  \param[out] value attribute value
	 *  \return false if attribute with given name does not exist
	 */
	bool getAttr(const std::string &name, std::string &value) const;
	/*! \brief Assigns the attribute value given the attribute name.
	 *  \param name attribute name
	 *  \param value attribute value
	 */
	void setAttr(const std::string &name, const std::string &value);

	/*! \brief Searches for an element within the DOM tree rooted at this node. 
	 *
	 *  If descend type is MXML_DESCEND, nodes are visited (and checked for match) in DFS order starting at \a current.
	 *  If descend type is MXML_DESCEND_FIRST, all indirect children (of \a current) are skipped.
	 *  If descend type is MXML_NO_DESCEND, all children (of \a current) are skipped.
	 *  If next node to visit is outside the tree rooted at "this", return null.
	 *  \param current search position
	 *  \param name if nonempty, searches for an element with given name
	 *  \param attr_name if nonempty, searches for an element with a given attribute name
	 *  \param attr_value if nonempty, searches for an element with a given attribute value
	 *  \param descend descend type
	 *  \return found node, or null
	 */
	Node findElement(const Node &current, const std::string &name = "", const std::string &attr_name = "", const std::string &attr_value = "", DescendType descend = DESCEND) const;

	/*! \brief Returns the next node within the DOM tree rooted at this node.
	 *
	 *  If descend type is MXML_DESCEND, nodes are visited in DFS order starting at \a current.
	 *  If descend type is MXML_DESCEND_FIRST, indirect children (of \a current) are skipped.
	 *  Unlike findElement, walkNext only visits one node each time, so MXML_DESCEND_FIRST is same as MXML_DESCEND in effect.
	 *  If descend type is MXML_NO_DESCEND, children (of \a current) are skipped.
	 *  If next node to visit is outside the tree rooted at "this", return null.
	 *  \param current search position
	 *  \param name if nonempty, searches for an element with given name
	 *  \param attr_name if nonempty, searches for an element with a given attribute name
	 *  \param attr_value if nonempty, searches for an element with a given attribute value
	 *  \param descend descension type
	 *  \return found node, or null
	 */
	Node walkNext(const Node &current, DescendType descend = DESCEND) const;

	/*! \brief Returns the next node within the DOM tree rooted at this node.
	 *
	 *  Nodes are visited in reverse DFS order.
	 *  \param current search position
	 *  \param name if nonempty, searches for an element with given name
	 *  \param attr_name if nonempty, searches for an element with a given attribute name
	 *  \param attr_value if nonempty, searches for an element with a given attribute value
	 *  \param descend descension type
	 *  \return found node, or null
	 */
	Node walkPrev(const Node &current, DescendType descend = DESCEND) const;

	/*! \brief Creates a new element.
	 *  \param name element name
	 *  \return created element
	 */
	static Node newElement(const std::string &name);
	/*! \brief Creates a new element and gives it a parent.
	 *
	 *  Added to the end of \a parent's children list.
	 *  \param parent parent node
	 *  \param name element name
	 *  \return created element
	 */
	static Node newElement(Node &parent, const std::string &name);
	/*! \brief Creates a text node.
	 *  \param text text contained in the node (entities will be escaped)
	 *  \param useCDATA whether to enclose the text in a CDATA section
	 *  \return created text node.
	 */
	static Node newText(const std::string &text, bool useCDATA = false);
	/*! \brief Creates a text node and gives it a parent.
	 *
	 *  Added to the end of \a parent's children list.
	 *  \param parent parent node
	 *  \param text text contained in the node (entities will be escaped)
	 *  \param useCDATA whether to enclose the text in a CDATA section
	 *  \return created text node.
	 */
	static Node newText(Node &parent, const std::string &text, bool useCDATA = false);
	/// \brief Creates a new XML DOM tree with XML declaration.
	static Node newXML();

	/// Assigns element name. Node must be element.
	void setElementName(const std::string &name);
	/// Assigns text. Node must be text.
	void setText(const std::string &text);
	/// Returns element name. Node must be element.
	std::string getElementName() const;
	/// Returns text. Node must be text.
	std::string getText() const;
	/// Returns all text under (and including) this node concatenated.
	std::string getInnerText() const;

	class NestedClass;
	/*! \brief Safe Bool Idiom.
	 *
	 *  You can treat a Node object as a boolean expression and test whether it is null.
	 */
	operator const NestedClass*() const{
		return _node ? reinterpret_cast<const NestedClass*>(this) : NULL;
	}

	/// Parses an XML string and loads the DOM tree at this node (must be null or destroyed first).
	void fromString(const std::string &s);
	/// Serializes the DOM tree into XML snippet
	std::string toString() const;

private:
	mxml_node_t *_node;
	bool self_destroy; ///< Whether the DOM tree rooted at this node will get destroyed when the node gets destructed.
};

} // namespace node
} // namespace xml

#endif
