/// \file porter.h

#ifndef __porter_h__
#define __porter_h__

#include <string>

namespace indexing {

/*! \brief Stems a word using Porter's stemmer.
 *  \param s input string
 *  \return stemmed output
 */
std::string stem(const std::string &s);

}

#endif
