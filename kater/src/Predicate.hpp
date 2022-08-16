/*
 * KATER -- Automating Weak Memory Model Metatheory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 */

#ifndef KATER_PREDICATE_HPP
#define KATER_PREDICATE_HPP

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <utility>

/*******************************************************************************
 **                           PredicateMask
 ******************************************************************************/

enum PredicateMask : unsigned long long;

/*******************************************************************************
 **                           PredicateInfo
 ******************************************************************************/

struct PredicateInfo {
	std::string name;
	std::string genmcString;
};

/*******************************************************************************
 **                           PredicateSet Class
 ******************************************************************************/

/*
 * A collection of predicates that compose, i.e., their intersection
 * is non-empty.
 */
class PredicateSet {

public:

	using builtin_iterator = std::vector<std::pair<PredicateMask, PredicateInfo>>::const_iterator;

	PredicateSet() = default;
	PredicateSet(PredicateMask p) : mask(p) {}

	/* Whether the collection is empty */
	bool empty() const;

	/* Returns true if the combination of two sets is valid */
	bool composes(const PredicateSet &other) const;

	/* Returns true if OTHER is included in THIS */
	bool includes(const PredicateSet &other) const;

	/* Tries to merge OTHER into THIS (if the combo is valid).
	 * Returns whether the merge was successful */
	bool merge(const PredicateSet &other);

	/* Removes all predicates appearing in OTHER from THIS */
	void minus(const PredicateSet &other);

	bool operator==(const PredicateSet &other) const {
		return mask == other.mask;
	}
	bool operator!=(const PredicateSet &other) const {
		return !(*this == other);
	}

	bool operator<(const PredicateSet &other) const {
		return mask < other.mask;
	}
	bool operator>=(const PredicateSet &other) const {
		return !(*this < other);
	}

	bool operator>(const PredicateSet &other) const {
		return mask > other.mask;
	}
	bool operator<=(const PredicateSet &other) const {
		return !(*this > other);
	}

	static const std::vector<std::pair<PredicateMask, PredicateInfo>> builtins;
	static builtin_iterator builtin_begin() { return builtins.begin(); }
	static builtin_iterator builtin_end() { return builtins.end(); }

	std::string toGenmcString() const;
	
	friend std::ostream &operator<<(std::ostream& ostr, const PredicateSet &s);

private:
	PredicateMask mask;
};

#endif /* KATER_PREDICATE_HPP */
