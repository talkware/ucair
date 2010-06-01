#include "document.h"

using namespace std;

namespace ucair {

Document::Document() {
}

Document::~Document() {
}

ostream& operator << (ostream &out, const Document &doc) {
	out << doc.doc_id << "\t" << doc.source << "\t" << doc.date << endl
		<< doc.title << endl
		<< doc.summary << endl
		<< doc.url << endl;
	return out;
}

} // namespace ucair
