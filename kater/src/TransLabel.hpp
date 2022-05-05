#ifndef __TRANS_LABEL_HPP__
#define __TRANS_LABEL_HPP__

#include <cassert>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

enum class RelType  { OneOne, ManyOne, UnsuppMany, Conj, Final };

struct PredicateInfo {
	std::string  name;
	unsigned     bitmask;
	std::string  genmcString;
};

struct RelationInfo {
	std::string  name;
	RelType      type;
	bool         insidePo;
	std::string  succString;
	std::string  predString;
};

extern const std::vector<PredicateInfo> builtinPredicates;
extern const std::vector<RelationInfo> builtinRelations;

/*******************************************************************************
 *                           TransLabel class
 ******************************************************************************/

/*
 * Represents the label of an NFA transition
 */
class TransLabel {

public:
	using RelID = int;
	using PredID = int;
	using PredSet = std::set<PredID>;

	/* Costructors/destructor */
	TransLabel() = delete;
	TransLabel(std::optional<RelID> id, const PredSet &preG = {}, const PredSet &postG = {})
		: id(id), preChecks(preG), postChecks(postG) {}

	static TransLabel getFreshCalcLabel() {
		return TransLabel(--calcNum);
	}

	using pred_iter = PredSet::iterator;
	using pred_const_iter = PredSet::const_iterator;

	using builtin_pred_iterator = std::vector<PredicateInfo>::iterator;
	using builtin_pred_const_iterator = std::vector<PredicateInfo>::const_iterator;

	using builtin_rel_iterator = std::vector<RelationInfo>::iterator;
	using builtin_rel_const_iterator = std::vector<RelationInfo>::const_iterator;

	pred_iter pre_begin() { return getPreChecks().begin(); }
	pred_iter pre_end() { return getPreChecks().end(); }

	pred_const_iter pre_begin() const { return getPreChecks().begin(); }
	pred_const_iter pre_end() const { return getPreChecks().end(); }

	pred_iter post_begin() { return getPostChecks().begin(); }
	pred_iter post_end() { return getPostChecks().end(); }

	pred_const_iter post_begin() const { return getPostChecks().begin(); }
	pred_const_iter post_end() const { return getPostChecks().end(); }

	static builtin_rel_const_iterator builtin_rel_begin() {
		return builtinRelations.begin();
	}
	static builtin_rel_const_iterator builtin_rel_end() {
		return builtinRelations.end();
	}

	static builtin_pred_const_iterator builtin_pred_begin() {
		return builtinPredicates.begin();
	}
	static builtin_pred_const_iterator builtin_pred_end() {
		return builtinPredicates.end();
	}

	const std::optional<RelID> &getId() const { return id; }
	std::optional<RelID> &getId() { return id; }

	bool isEpsilon() const { return !getId(); }

	bool isBuiltin() const { return getId().value_or(42) >= 0; }

	int getCalcIndex() const { assert(!isBuiltin()); return -(*getId() + 1); }

	bool isFlipped() const { return flipped; }

	void flip() {
		flipped = !flipped;
		/* Do not flip ε transitions; maintain unique representation */
		if (isEpsilon())
			return;
		std::swap(getPreChecks(), getPostChecks());
	}

	/* Attemps to merge OTHER into THIS and returns whether it
	 * succeeded.  Two transitions can be merged if at least one
	 * of them is an epsilon transition */
	bool merge(const TransLabel &other);

	std::string toString() const;

	bool operator==(const TransLabel &other) const {
		return getId() == other.getId() &&
			isFlipped() == other.isFlipped() &&
			getPreChecks() == other.getPreChecks() &&
			getPostChecks() == other.getPostChecks();
	}
	bool operator!=(const TransLabel &other) const {
		return !operator==(other);
	}

	bool operator<(const TransLabel &other) const {
		return getId() < other.getId() ||
			(getId() == other.getId() && isFlipped() < other.isFlipped()) ||
			(getId() == other.getId() && isFlipped() == other.isFlipped() &&
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
	const PredSet &getPreChecks() const { return preChecks; }
	PredSet &getPreChecks() { return preChecks; }

	const PredSet &getPostChecks() const { return postChecks; }
	PredSet &getPostChecks() { return postChecks; }

	std::optional<RelID> id;
	bool flipped = false;
	static int calcNum;

	PredSet preChecks;
	PredSet postChecks;
};

#endif /* __TRANS_LABEL_HPP__ */
