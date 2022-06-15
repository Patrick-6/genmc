#ifndef __TRANS_LABEL_HPP__
#define __TRANS_LABEL_HPP__

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

	using pred_iter = PredicateSet::iterator;
	using pred_const_iter = PredicateSet::const_iterator;

	pred_iter pre_begin() { return getPreChecks().begin(); }
	pred_iter pre_end() { return getPreChecks().end(); }

	pred_const_iter pre_begin() const { return getPreChecks().begin(); }
	pred_const_iter pre_end() const { return getPreChecks().end(); }

	pred_iter post_begin() { return getPostChecks().begin(); }
	pred_iter post_end() { return getPostChecks().end(); }

	pred_const_iter post_begin() const { return getPostChecks().begin(); }
	pred_const_iter post_end() const { return getPostChecks().end(); }

	const std::optional<Relation> &getRelation() const { return id; }
	std::optional<Relation> &getRelation() { return id; }

	const PredicateSet &getPreChecks() const { return preChecks; }
	PredicateSet &getPreChecks() { return preChecks; }

	const PredicateSet &getPostChecks() const { return postChecks; }
	PredicateSet &getPostChecks() { return postChecks; }

	bool isPredicate() const { return !getRelation(); }

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

#endif /* __TRANS_LABEL_HPP__ */
