#ifndef __index_util_h__
#define __index_util_h__

#include <map>
#include <string>
#include <vector>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include "value_map.h"

namespace indexing {

/*! \brief A dictionary between integer ids and string names
 *
 *  Ids are assigned automatically, from the range [1 .. number_of_names]
 */
class NameDict{
public:

	/*! \brief Find a string's corresponding id.
	 *  \param name string to look up
	 *  \param insert_if_not_found create a mapping if the string is not in dict
	 *  \return corresponding id, -1 if not found and insert_if_not_found is false
	 */
	int getId(const std::string &name, bool insert_if_not_found = false);

	/// Whether an id exists.
	bool hasId(int id) const { return id > 0 && id <= size(); }

	/*! \brief Find an id's corresponding string. Id must be in valid range.
	 *  \param id id to look up
	 *  \return corresponding string
	 */
	std::string getName(int id);

	/// Returns the number of names.
	int size() const { return (int) name_map.size(); }

	/// Clear all mappings.
	void clear() { name_map.clear(); }

private:

	/// A container allowing array-like random access and string hashmap lookup.
	typedef boost::multi_index::multi_index_container<
		std::string,
		boost::multi_index::indexed_by<
			boost::multi_index::random_access<>, // random access array
			boost::multi_index::hashed_non_unique<boost::multi_index::identity<std::string> > // string hash map
		>
	> NameMap;

	typedef NameMap::nth_index<1>::type NameIndex;

	NameMap name_map;
};

/*! \brief Counts the frequency of different terms in a piece of text.
 *
 *  Terms are also (optionally) stemmed and transformed to ids.
 *  \param[in] term_dict dictionary to transform string to id
 *  \param[in] text input text
 *  \param[out] term_counts mapping from term id to term frequency
 *  \param[in] stem_term whether to append to term dict if new terms are found. if false, new terms are ignored.
 */
void countTerms(NameDict &term_dict, const std::string &text, std::map<int, double> &term_counts, bool stem_term = true, bool update_term_dict = true);

/*! \brief Loads term counts from a file and computes term probabilities.
 *
 *  The file should contain term counts in a background collection.
 *  A term's background prob is calculated as
 *  (term_count_in_collection + 1) / (total_term_count_in_collection + unique_term_count_in_collection)
 *  First line of file contains total_term_count and unique_term_count separated by tab.
 *  Remaining line contains a term and its count separated by tab.
 *
 *  \param[in] file_name filename
 *  \param[in,out] term_dict dictionary to transform string to id
 *  \param[out] term_probs term probabilities
 *  \param[out] default_col_prob default probability for terms not occurring in collection
 */
bool loadColProbs(const std::string &file_name, NameDict &term_dict, ValueMap &term_probs, double &default_col_prob);

} // namespace indexing

#endif
