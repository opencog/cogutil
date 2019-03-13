/** Counter.h ---
 *
 * Copyright (C) 2011 OpenCog Foundation
 *
 * \author Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef _OPENCOG_COUNTER_H
#define _OPENCOG_COUNTER_H

#include <set>
#include <map>
#include <initializer_list>
#include <boost/operators.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/map.hpp>

namespace opencog {
/** \addtogroup grp_cogutil
 *  @{
 */

using boost::adaptors::map_values;

//! Class that mimics python Counter container
/**
 * This is basically a dictionary of key:count values.
 * Given following pseudocode:
 * @code
 * for word in ['red', 'blue', 'red', 'green', 'blue', 'blue']:
 *   cnt[word] += 1
 * @endcode
 * the counter will hold 'blue': 3, 'red': 2, 'green': 1
 */
template<typename T, typename CT, typename CMP = std::less<T>>
class Counter : public std::map<T, CT, CMP>
	, boost::arithmetic<Counter<T, CT, CMP>>
	, boost::arithmetic2<Counter<T, CT, CMP>,CT>

{
protected:
	/** @todo this will be replaced by C++11 constructor
	 * delegation instead of init
	 */
	template<typename IT>
	void init(IT from, IT to) {
		while(from != to) {
			//we don't use ++ to put the least assumption on on CT
			this->operator[](*from) += 1;
			++from;
		}
	}

public:
	typedef std::map<T, CT, CMP> super;
	typedef typename super::value_type value_type;

	Counter() {}

	template<typename IT>
	Counter(IT from, IT to)
	{
		init(from, to);
	}

	template<typename Container>
	Counter(const Container& c)
	{
		init(c.begin(), c.end());
	}

	Counter(const std::initializer_list<value_type>& il)
	{
		for(const auto& v : il)
			this->operator[](v.first) = v.second;
	}

	/// Return the count of a key, possibly returning a default if none
	/// is present. That method different that operator[] because it
	/// doesn't insert the element if it is not present. This is very
	/// useful for multi-threading programming, by helping avoid race
	/// conditions.
	CT get(const T& key, CT c = CT()) const
	{
		typename super::const_iterator it = this->find(key);
		return it == this->cend()? c : it->second;
	}

	//! Return the total of all counted elements
	CT total_count() const
	{
		return boost::accumulate(*this | map_values, (CT)0);
	}

	//! Return the mode, the element that occurs most frequently
	T mode() const
	{
		T key = super::begin()->first;
		CT cnt = super::begin()->second;
		for (const auto& v : *this) {
			if (cnt < v.second)
				key = v.first;
		}
		return key;
	}

	/* Counter operators:
	 * examples with:
	 * c1 = {'a':1, 'b':2}
	 * c2 = {'b':2, 'c':3}
	 * v = 2
	 */

	//! add 2 counters,
	/**
	 * c1 += c2
	 * =>
	 * c1 = {'a':1, 'b':4, 'c':3}
	 */
	Counter& operator+=(const Counter& other) {
		for (const auto& v : other)
			this->operator[](v.first) += v.second;
		return *this;
	}

	//! subtract 2 counters,
	/**
	 * c1 -= c2
	 * =>
	 * c1 = {'a':1, 'b':0, 'c':-3}
	 */
	Counter& operator-=(const Counter& other) {
		for (const auto& v : other)
			this->operator[](v.first) -= v.second;
		return *this;
	}

	//! multiply (inner product of) 2 counters,
	/**
	 * c1 *= c2
	 * =>
	 * c1 = {'a':0, 'b':4, 'c':0}
	 */
	Counter& operator*=(const Counter& other) {
		auto it = this->begin();
		auto other_it = other.begin();
		while (it != this->end() and other_it != other.end()) {
			if (this->key_comp()(it->first, other_it->first)) {
				it->second = CT();
				++it;
			} else if (this->key_comp()(other_it->first, it->first)) {
				this->emplace_hint(it, other_it->first, CT());
				++other_it;
			} else {
				it->second *= other_it->second;
				++it;
				++other_it;
			}
		}
		for (; it != this->end(); ++it)
			it->second = CT();
		for (; other_it != other.end(); ++other_it)
			this->emplace_hint(it, other_it->first, CT());
		return *this;
	}

	//! divide 2 counters,
	/**
	 * c1 /= c2
	 * =>
	 * c1 = {'a':1, 'b':1, 'c':0}
	 */
	Counter& operator/=(const Counter& other) {
		for (const auto& v : other)
			this->operator[](v.first) /= v.second;
		return *this;
	}

	//! add CT to counter
	/**
	 * c1 += v
	 * =>
	 * c1 = {'a':3, 'b':4}
	 */
	Counter& operator+=(const CT& num) {
		for (auto& v : *this)
			v.second += num;
		return *this;
	}

	/**
	 * c1 -= v
	 * =>
	 * c1 = {'a':-1, 'b':0}
	 */
	Counter& operator-=(const CT& num) {
		for (auto& v : *this)
			v.second -= num;
		return *this;
	}

	/**
	 * c1 *= v
	 * =>
	 * c1 = {'a':2, 'b':4}
	 */
	Counter& operator*=(const CT& num) {
		for (auto& v : *this)
			v.second *= num;
		return *this;
	}

	/**
	 * c1 /= v
	 * =>
	 * c1 = {'a':0.5, 'b':1}
	 */
	Counter& operator/=(const CT& num) {
		for (auto& v : *this)
			v.second /= num;
		return *this;
	}

	/**
	 * Return all keys.
	 *
	 * For instance:
	 * c1.keys() = {'a', 'b'}
	 */
	std::set<T> keys() const {
		std::set<T> ks;
		for (auto& v : *this)
			ks.insert(v.first);
		return ks;
	}
};

template<typename T, typename CT, typename CMP = std::less<T>>
std::ostream& operator<<(std::ostream& out, const Counter<T, CT, CMP>& c)
{
	typedef Counter<T, CT, CMP> counter_t;
	out << "{";
	for (typename counter_t::const_iterator it = c.begin(); it != c.end();) {
		out << it->first << ": " << it->second;
		++it;
		if(it != c.end())
			out << ", ";
	}
	out << "}";
	return out;
}

/** @}*/
} // ~namespace opencog

#endif // _OPENCOG_COUNTER_H
