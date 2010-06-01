#ifndef __value_map_h__
#define __value_map_h__

#include <list>
#include <map>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace indexing {

/*! \brief Const iterator for ValueMap.
 *  \sa ValueIterator
 *  \sa ValueMap
 */
class ConstValueIterator {
public:
	virtual ~ConstValueIterator() {}

	/// Whether there is id/value at current position.
	virtual bool ok() const = 0;

	/// Move to the next position.
	virtual void next() = 0;

	/// Id at the current position.
	virtual int id() const = 0;

	/// Returns value at the current position.
	virtual double get() const = 0;

	/// Move to the initial position.
	virtual void reset() = 0;
};

/*! \brief Iterator for ValueMap.
 *  \sa ConstValueIterator
 *  \sa ValueMap
 */
class ValueIterator {
public:
	virtual ~ValueIterator() {}

	/// Whether there is id/value at current position.
	virtual bool ok() const = 0;

	/// Move to the next position.
	virtual void next() = 0;

	/// Id at the current position.
	virtual int id() const = 0;

	/// Returns value at the current position.
	virtual double get() const = 0;

	/// Assigns value at the current position.
	virtual void set(double value) const = 0;

	/// Move to the initial position.
	virtual void reset() = 0;
};

/*! \brief A (generic) collection of (int, double) pairs.
 *
 *  Useful for storing term id, term weight pairs.
 *  Supports multiple underlying containers, including
 *  1) tree map from int to double
 *  2) hash map from int to double
 *  3) list of (int, double) pairs
 *  4) vector of (int, double) pairs
 *  5) vector of doubles (array index for int)
 *
 *  Since this is just a wrapper, construction is cheap.
 *
 *  \sa ConstValueIterator
 *  \sa ValueIterator
 */
class ValueMap {
public:
	virtual ~ValueMap() {}

	static boost::shared_ptr<ValueMap> from(std::map<int, double> &);
	static boost::shared_ptr<ValueMap> from(boost::unordered_map<int, double> &);
	static boost::shared_ptr<ValueMap> from(std::list<std::pair<int, double> > &);
	static boost::shared_ptr<ValueMap> from(std::vector<std::pair<int, double> > &);
	static boost::shared_ptr<ValueMap> from(std::vector<double> &);

	static boost::shared_ptr<const ValueMap> from(const std::map<int, double> &);
	static boost::shared_ptr<const ValueMap> from(const boost::unordered_map<int, double> &);
	static boost::shared_ptr<const ValueMap> from(const std::list<std::pair<int, double> > &);
	static boost::shared_ptr<const ValueMap> from(const std::vector<std::pair<int, double> > &);
	static boost::shared_ptr<const ValueMap> from(const std::vector<double> &);

	void to(std::map<int, double> &) const;
	void to(boost::unordered_map<int, double> &) const;
	void to(std::list<std::pair<int, double> > &) const;
	void to(std::vector<std::pair<int, double> > &) const;
	void to(std::vector<double> &) const;

	/// Returns a copy of the current object.
	virtual boost::shared_ptr<ValueMap> copy() = 0;

	/// Returns a const copy of the current object.
	virtual boost::shared_ptr<const ValueMap> copy() const = 0;

	/// Clears all pairs.
	virtual void clear() = 0;

	/// Number of pairs.
	virtual int size() const = 0;

	/*! \brief Returns the value associated with an int id.
	 *  \param[in] id id
	 *  \param[out] value value
	 *  \return false if get fails
     */
	virtual bool get(int id, double &value) const = 0;

	/*! \brief Assigns the value associated with an int id.
	 *  \param id id
	 *  \param value value
	 *  \param check_duplicate whether to check if id already exists
	 *  \return false if put fails
	 */
	virtual bool set(int id, double value, bool check_duplicate = false) = 0;

	/// Returns an iterator.
	virtual boost::shared_ptr<ValueIterator> iterator() = 0;

	/// Returns a const iterator.
	virtual boost::shared_ptr<ConstValueIterator> const_iterator() const = 0;
};

} // namespace indexing

#endif
