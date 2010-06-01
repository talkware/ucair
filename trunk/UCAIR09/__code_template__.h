///////////////////h

#ifndef __code_template_h__
#define __code_template_h__

namespace ucair {

class CodeTemplate {
public:
	CodeTemplate();
	~CodeTemplate();
};

} // namespace ucair

#endif

///////////////////cpp

#include "code_template.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace boost;

namespace ucair {

CodeTemplate::CodeTemplate() {
}

CodeTemplate::~CodeTemplate() {
}

} // namespace ucair