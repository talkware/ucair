#include "value_map.h"
#include <algorithm>
#include <cassert>
#include <boost/tuple/tuple.hpp>

using namespace std;
using namespace boost;

namespace indexing {

template <class T>
void convert(const ValueMap &a, T &b) {
	b.clear();
	shared_ptr<ConstValueIterator> itr = a.const_iterator();
	while (itr->ok()) {
		b.insert(b.end(), make_pair(itr->id(), itr->get()));
		itr->next();
	}
}

void ValueMap::to(map<int, double> &v) const {
	convert(*this, v);
}

void ValueMap::to(unordered_map<int, double> &v) const {
	convert(*this, v);
}

void ValueMap::to(list<pair<int, double> > &v) const {
	convert(*this, v);
}

void ValueMap::to(vector<pair<int, double> > &v) const {
	v.reserve(size());
	convert(*this, v);
}

void ValueMap::to(vector<double> &v) const {
	fill(v.begin(), v.end(), 0.0);
	shared_ptr<ConstValueIterator> itr = const_iterator();
	while (itr->ok()) {
		const int id = itr->id();
		assert(id > 0 && id < (int) v.size());
		v[id] = itr->get();
		itr->next();
	}
}

template <class T>
class ConstValueIteratorImplA: public ConstValueIterator {
public:
	ConstValueIteratorImplA(T &x_): x(x_), itr(x.begin()) {}
	bool ok() const { return itr != x.end(); }
	void next() { ++ itr; }
	int id() const { return itr->first; }
	double get() const { return itr->second; }
	void reset() { itr = x.begin(); }
protected:
	T &x;
	typename T::const_iterator itr;
};

template <class T>
class ValueIteratorImplA: public ValueIterator {
public:
	ValueIteratorImplA(T &x_): x(x_), itr(x.begin()) {}
	bool ok() const { return itr != x.end(); }
	void next() { ++ itr; }
	int id() const { return itr->first; }
	double get() const { return itr->second; }
	void set(double value) const { itr->second = value; }
	void reset() { itr = x.begin(); }
protected:
	T &x;
	typename T::iterator itr;
};

/// ValueMap implementation for tree/hash maps.
template <class T>
class ValueMapImplA: public ValueMap {
public:
	ValueMapImplA(T &x_): x(x_) {}

	shared_ptr<ValueMap> copy() { return shared_ptr<ValueMap>(new ValueMapImplA(*this)); }
	shared_ptr<const ValueMap> copy() const { return shared_ptr<const ValueMap>(new ValueMapImplA(*this)); }
	void clear() { x.clear(); }
	int size() const { return x.size(); }
	bool get(int id, double &value) const;
	bool set(int id, double value, bool check_duplicate);

	shared_ptr<ValueIterator> iterator();
	shared_ptr<ConstValueIterator> const_iterator() const;

private:
	T &x;
};

template <class T>
bool ValueMapImplA<T>::get(int id, double &value) const {
	typename T::const_iterator itr = x.find(id);
	if (itr == x.end()){
		return false;
	}
	value = itr->second;
	return true;
}

template <class T>
bool ValueMapImplA<T>::set(int id, double value, bool /*check_duplicate*/){
	typename T::iterator itr;
	bool inserted;
	tie(itr, inserted) = x.insert(make_pair(id, value));
	if (! inserted){
		itr->second = value;
	}
	return true;
}

template <class T>
shared_ptr<ValueIterator> ValueMapImplA<T>::iterator() {
	return shared_ptr<ValueIterator>(new ValueIteratorImplA<T>(x));
}

template <class T>
shared_ptr<ConstValueIterator> ValueMapImplA<T>::const_iterator() const {
	return shared_ptr<ConstValueIterator>(new ConstValueIteratorImplA<T>(x));
}

shared_ptr<ValueMap> ValueMap::from(map<int, double> &x) {
	return shared_ptr<ValueMap>(new ValueMapImplA<map<int, double> >(x));
}

shared_ptr<const ValueMap> ValueMap::from(const map<int, double> &x) {
	map<int, double> &y = const_cast<map<int, double>&>(x);
	return shared_ptr<const ValueMap>(new ValueMapImplA<map<int, double> >(y));
}

shared_ptr<ValueMap> ValueMap::from(unordered_map<int, double> &x) {
	return shared_ptr<ValueMap>(new ValueMapImplA<unordered_map<int, double> >(x));
}

shared_ptr<const ValueMap> ValueMap::from(const unordered_map<int, double> &x) {
	unordered_map<int, double> &y = const_cast<unordered_map<int, double>&>(x);
	return shared_ptr<const ValueMap>(new ValueMapImplA<unordered_map<int, double> >(y));
}


////////////////////

/// ValueMap implementation for list/vector
template <class T>
class ValueMapImplB: public ValueMap {
public:
	ValueMapImplB(T &x_): x(x_) {}

	shared_ptr<ValueMap> copy() { return shared_ptr<ValueMap>(new ValueMapImplB(*this)); }
	shared_ptr<const ValueMap> copy() const { return shared_ptr<const ValueMap>(new ValueMapImplB(*this)); }

	void clear() { x.clear(); }
	int size() const { return x.size(); }
	bool get(int id, double &value) const;
	bool set(int id, double value, bool check_duplicate);

	shared_ptr<ValueIterator> iterator();
	shared_ptr<ConstValueIterator> const_iterator() const;

private:
	T &x;
};

template <class T>
bool ValueMapImplB<T>::get(int id, double &value) const {
	for (typename T::const_iterator itr = x.begin(); itr != x.end(); ++ itr){
		if (itr->first == id){
			value = itr->second;
			return true;
		}
	}
	return false;
}

template <class T>
bool ValueMapImplB<T>::set(int id, double value, bool check_duplicate){
	if (check_duplicate){
		for (typename T::iterator itr = x.begin(); itr != x.end(); ++ itr){
			if (itr->first == id){
				itr->second = value;
				return true;
			}
		}
	}
	x.push_back(make_pair(id, value));
	return true;
}

template <class T>
shared_ptr<ValueIterator> ValueMapImplB<T>::iterator() {
	return shared_ptr<ValueIterator>(new ValueIteratorImplA<T>(x));
}

template <class T>
shared_ptr<ConstValueIterator> ValueMapImplB<T>::const_iterator() const {
	return shared_ptr<ConstValueIterator>(new ConstValueIteratorImplA<T>(x));
}

shared_ptr<ValueMap> ValueMap::from(list<pair<int, double> > &x) {
	return shared_ptr<ValueMap>(new ValueMapImplB<list<pair<int, double> > >(x));
}

shared_ptr<const ValueMap> ValueMap::from(const list<pair<int, double> > &x) {
	list<pair <int, double> > &y = const_cast<list<pair<int, double> >&>(x);
	return shared_ptr<const ValueMap>(new ValueMapImplB<list<pair<int, double> > >(y));
}

shared_ptr<ValueMap> ValueMap::from(vector<pair<int, double> > &x) {
	return shared_ptr<ValueMap>(new ValueMapImplB<vector<pair<int, double> > >(x));
}

shared_ptr<const ValueMap> ValueMap::from(const vector<pair<int, double> > &x) {
	vector<pair <int, double> > &y = const_cast<vector<pair<int, double> >&>(x);
	return shared_ptr<const ValueMap>(new ValueMapImplB< vector<pair<int, double> > >(y));
}

////////////////////

template <class T>
class ValueIteratorImplC: public ValueIterator {
public:
	ValueIteratorImplC(T &x_): x(x_), pos(1) {}
	bool ok() const { return pos < x.size(); }
	void next() { ++ pos; }
	int id() const { return pos; }
	double get() const { return x[pos]; }
	void set(double value) const { x[pos] = value; }
	void reset() { pos = 1; }
private:
	T &x;
	size_t pos;
};

template <class T>
class ConstValueIteratorImplC: public ConstValueIterator {
public:
	ConstValueIteratorImplC(const T &x_): x(x_), pos(1) {}
	bool ok() const { return pos < x.size(); }
	void next() { ++ pos; }
	int id() const { return pos; }
	double get() const { return x[pos]; }
	void reset() { pos = 1; }
private:
	const T &x;
	size_t pos;
};

/// ValueMap implementation for array
template <class T>
class ValueMapImplC: public ValueMap {
public:
	ValueMapImplC(T &x_): x(x_) { assert(x.size() > 1); }

	shared_ptr<ValueMap> copy() { return shared_ptr<ValueMap>(new ValueMapImplC(*this)); }
	shared_ptr<const ValueMap> copy() const { return shared_ptr<const ValueMap>(new ValueMapImplC(*this)); }

	void clear() { fill(x.begin(), x.end(), 0.0); }
	int size() const { return x.size(); }
	bool get(int id, double &value) const;
	bool set(int id, double value, bool check_duplicate);

	shared_ptr<ValueIterator> iterator();
	shared_ptr<ConstValueIterator> const_iterator() const;
private:
	T &x;
};

template <class T>
bool ValueMapImplC<T>::get(int id, double &value) const {
	if (id > 0 && id < (int) x.size()){
		value = x[id];
		return true;
	}
	return false;
}

template <class T>
bool ValueMapImplC<T>::set(int id, double value, bool /*check_duplicate*/){
	if (id > 0 && id < (int) x.size()){
		x[id] = value;
		return true;
	}
	return false;
}

template <class T>
shared_ptr<ValueIterator> ValueMapImplC<T>::iterator() {
	return shared_ptr<ValueIterator>(new ValueIteratorImplC<T>(x));
}

template <class T>
shared_ptr<ConstValueIterator> ValueMapImplC<T>::const_iterator() const {
	return shared_ptr<ConstValueIterator>(new ConstValueIteratorImplC<T>(x));
}

shared_ptr<ValueMap> ValueMap::from(vector<double> &x) {
	return shared_ptr<ValueMap>(new ValueMapImplC<vector<double> >(x));
}

shared_ptr<const ValueMap> ValueMap::from(const vector<double> &x) {
	vector<double> &y = const_cast<vector<double>&>(x);
	return shared_ptr<const ValueMap>(new ValueMapImplC<vector<double> >(y));
}

} // namespace indexing
