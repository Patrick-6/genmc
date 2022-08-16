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

#ifndef KATER_TRANS_LABEL_HPP
#define KATER_TRANS_LABEL_HPP

#include "Predicate.hpp"
#include "Relation.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

/*******************************************************************************
 *                           TransLabel class
 ******************************************************************************/

/*
 * Represents the label of an NFA transition
 */
class TransLabel {

public:
	/* Costructors/destructor */
	TransLabel() = delete;
	TransLabel(std::optional<Relation> id, const PredicateSet &preG = {},
		   const PredicateSet &postG = {})
		: id(id), preChecks(preG), postChecks(postG) {}

	const std::optional<Relation> &getRelation() const { return id; }
	std::optional<Relation> &getRelation() { return id; }

	const PredicateSet &getPreChecks() const { return preChecks; }
	PredicateSet &getPreChecks() { return preChecks; }

	const PredicateSet &getPostChecks() const { return postChecks; }
	PredicateSet &getPostChecks() { return postChecks; }

	bool isPredicate() const { return !getRelation(); }

	bool isRelation() const { return !isPredicate(); }

	bool hasPreChecks() const { return !getPreChecks().empty(); }

	bool hasPostChecks() const { return !getPostChecks().empty(); }

	bool isBuiltin() const { return !getRelation().has_value() || getRelation()->isBuiltin(); }

	int getCalcIndex() const { assert(!isBuiltin()); return -(getRelation()->getID() + 1); }

	void flip() {
		/* Do not flip ε transitions; maintain unique representation */
		if (isPredicate()) {
			assert(!hasPostChecks());
			return;
		}
		getRelation()->invert();
		std::swap(getPreChecks(), getPostChecks());
	}

	/* Attemps to merge OTHER into THIS and returns whether it
	 * succeeded.  Two transitions can be merged if at least one
	 * of them is an epsilon transition */
	bool merge(const TransLabel &other,
		   std::function<bool(const TransLabel &)> isValid = [](auto &lab){ return true; });

	bool composesWith(const TransLabel &other) const;

	bool matches(const TransLabel &other) const {
		return getPreChecks().includes(other.getPreChecks()) &&
			(!isRelation() || (other.isRelation() && getRelation()->includes(*other.getRelation()))) &&
			getPostChecks().includes(other.getPostChecks());
	}

	std::string toString() const;

	bool operator==(const TransLabel &other) const {
		return getRelation() == other.getRelation() &&
			getPreChecks() == other.getPreChecks() &&
			getPostChecks() == other.getPostChecks();
	}
	bool operator!=(const TransLabel &other) const {
		return !operator==(other);
	}

	bool operator<(const TransLabel &other) const {
		return getRelation() < other.getRelation() ||
			(getRelation() == other.getRelation() &&
				(getPreChecks() < other.getPreChecks() ||
				 (getPreChecks() == other.getPreChecks() &&
				  getPostChecks() < other.getPostChecks())));
	}
	bool operator<=(const TransLabel &other) const {
		return operator==(other) || operator<(other);
	}
	bool operator>=(const TransLabel &other) const {
		return !operator<(other);
	}
	bool operator>(const TransLabel &other) const {
		return !operator<(other) && !operator==(other);
	}

	friend std::ostream &operator<<(std::ostream &s, const TransLabel &t) {
		return s << t.toString();
	}

private:
	std::optional<Relation> id;
	static int calcNum;

	PredicateSet preChecks;
	PredicateSet postChecks;
};

#endif /* KATER_TRANS_LABEL_HPP */
