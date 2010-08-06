#ifndef __ucair_util_h__
#define __ucair_util_h__

#include <map>
#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/smart_ptr.hpp>
#include "document.h"
#include "index_util.h"
#include "simple_index.h"
#include "value_map.h"

namespace ucair {

class Search;

/// Whether there are HTML tags in the input string.
bool isHTML(const std::string &text);
/// Strips HTML tags from the input string.
std::string stripHTML(const std::string &text);

/// Indexes a document's title and summary. Already indexed documents are skipped.
void indexDocument(indexing::SimpleIndex &index, const Document &doc);

/*! \brief Generates a doc name from search id and result pos.
 */
std::string buildDocName(const std::string &search_id, int result_pos);
/*! \brief Parses search id and result pos from a doc name.
 *  \return a pair of (search id, result pos). if parsing fails, search id is empty and result pos is 0.
 */
std::pair<std::string, int> parseDocName(const std::string &doc_name);

/*! \brief Normalizes the values in a map so that their sum (or squared sum) is 1.
 *  \param squared if ture, use squared sum.
 */
void normalize(indexing::ValueMap &m, bool squared = false);

/*! \brief Linearly interpolates two value maps.
 *  \param[in] a first input value map
 *  \param[in] b second input value map
 *  \param[out] c output value map
 *  \param[in] alpha weight on the first input, i.e., c = a*alpha + b*(1-alpha)
 */
void interpolate(const indexing::ValueMap &a, const indexing::ValueMap &b, std::map<int, double> &c, double alpha);

/*! \brief Sorts the value map entries in decreasing order of values.
 *  \param[in] a value map
 *  \param[out] b vector of value map entries sorted
 */
void sortValues(const indexing::ValueMap &a, std::vector<std::pair<int, double> > &b);

/*! \brief Truncates a value map.
 *  \param[in] a original value map
 *  \param[out] b truncated value map
 *  \param[in] max_size output value map's size should not exceed this
 *  \param[in] min_value all values in output value map should be greater than this
 */
void truncate(const indexing::ValueMap &a, std::vector<std::pair<int, double> > &b, int max_size, double min_value, double min_sum = 1.0);
/*! \brief Truncates a value map in place.
 *  \param[in, out] a value map
 *  \param[in] max_size output value map's size should not exceed this
 *  \param[in] min_value all values in output value map should be greater than this
 */
void truncate(indexing::ValueMap &a, int max_size, double min_value, double min_sum = 1.0);

/*! \brief Converts a value map to a vector of (name, value) pairs using a NameDict.
 *
 *  Ids not found in NameDict are skipped.
 *  \param[in] m value map from id to value
 *  \param[out] l a vector of (name, value) pairs
 *  \param[in] name_dict translator between id and name
 */
void id2Name(const indexing::ValueMap &m, std::vector<std::pair<std::string, double> > &l, indexing::NameDict &name_dict);
/*! \brief Converts a vector of (name, value) pairs to a value map using a NameDict.
 *
 *  Names not found in NameDict are skipped.
 *  \param[in] l a vector of (name, value) pairs
 *  \param[out] m value map from id to value
 *  \param[in] name_dict translator between id and name
 */
void name2Id(const std::vector<std::pair<std::string, double> > &l, indexing::ValueMap &m, indexing::NameDict &name_dict, bool add_id = true);

/*! \brief Prints a vector of (name, value) pairs.
 *
 *  Each (name, value) will occupy a line, separated by a tab
 *  \param[in] l a vector of (name, value) pairs
 *  \param[out] s output string
 *  \param[in] precision how many digits to keep after the point
 */
void toString(const std::vector<std::pair<std::string, double> > &l, std::string &s, int precision = 4);
/*! \brief Parses a vector of (name, value) pairs from the output of toString.
 *
 *  \param[in] s output string
 *  \param[out] l a vector of (name, value) pairs
 */
void fromString(const std::string &s, std::vector<std::pair<std::string, double> > &l);

/*! \brief Transforms the original url into a string suitable for display.
 *
 *  Removes the protocol part and truncate the string if the size exceeds max limit.
 */
std::string getDisplayUrl(const std::string &url, int truncate_size = 50);

/// Returns cosine similarity between two models. Note: this does not consider IDF; could be improved.
double getCosSim(const std::map<int, double> &a, const std::map<int, double> &b);

/*! Returns the date part of a ptime as string.
 *
 * @param useYesterdayToday Whether to use "Yesterday"/"Today" where appropriate.
 */
std::string getDateStr(const boost::posix_time::ptime &t, bool useYesterdayToday = false);
/// Returns the time part of a ptime as string.
std::string getTimeStr(const boost::posix_time::ptime &t);

/*! \brief Returns the directory in which to store program data.
 *  This is specified in the config entry "program_data_dir".
 */
std::string getProgramDataDir();

} // namespace ucair

#endif
