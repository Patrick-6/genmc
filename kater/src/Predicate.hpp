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
#include <set>
#include <string>
#include <unordered_map>

/*******************************************************************************
 **                           PredicateInfo Class
 ******************************************************************************/

struct PredicateInfo {
	std::string  name;
	unsigned     bitmask;
};


/*******************************************************************************
 **                           Predicate Class
 ******************************************************************************/

class PredicateSet;

class Predicate {

public:
	/* Negative IDs reserved for user-defined predicates.
	 * We have no such predicates for the time being. */
	using ID = int;

	enum Builtin {
		Marked = 0,
		NA,
		ACQ,
		REL,
		SC,
		/* Random stuff */
		IsDynamicLoc,
		NotHpProtected,
		RfInit,
		REC,
		D,
		/* Memory accesses */
		MemAccess,
		W,
		UW,
		R,
		UR,
		//	HelpingCas,
		/* Fences */
		F,
		Fwmb,
		Frmb,
		Fba,
		Faa,
		Fas,
		Faul,
		/* Thread events */
		TC,
		TJ,
		TK,
		TB,
		TE,
		/* Allocation */
		Alloc,
		Free,
		HpRetire,
		HpProtect,
		/* Mutexes */
		LR,
		LW,
		UL,
		/* RCU */
		RCUSync,
		RCULock,
		RCUUnlock,
		/* File ops */
		//	DskWrite,
		//	DskMdWrite,
		//	DskDirWrite,
		//	DskJnlWrite,
		DskOpen,
		DskFsync,
		DskSync,
		DskPbarrier,
		CLF,
	};

	using builtin_iterator = std::unordered_map<Predicate::Builtin, PredicateInfo>::iterator;
	using builtin_const_iterator = std::unordered_map<Predicate::Builtin, PredicateInfo>::const_iterator;

protected:
	Predicate() = delete;
	Predicate(ID id) : id(id) {}

public:
	static Predicate createBuiltin(Builtin b) { return Predicate(getBuiltinID(b)); }

	static ID getBuiltinID(Builtin b) { return b; }

	static builtin_const_iterator builtin_begin() { return builtins.begin(); }
	static builtin_const_iterator builtin_end() { return builtins.end(); }

	constexpr ID getID() const { return id; }

	constexpr bool isBuiltin() const { return id >= 0; }

	constexpr Builtin toBuiltin() const {
		assert(isBuiltin());
		return static_cast<Builtin>(getID());
	}

	/* Whether THIS is a subpredicate of OTHER */
	bool subOf(const Predicate &other) const;

	bool operator==(const Predicate &other) const {
		return getID() == other.getID();
	}
	bool operator!=(const Predicate &other) const {
		return !(*this == other);
	}

	bool operator<(const Predicate &other) const {
		return getID() < other.getID();
	}
	bool operator>=(const Predicate &other) const {
		return !(*this < other);
	}

	bool operator>(const Predicate &other) const {
		return getID() > other.getID();
	}
	bool operator<=(const Predicate &other) const {
		return !(*this > other);
	}

	friend std::ostream &operator<<(std::ostream& ostr, const Predicate &p);

private:
	friend class PredicateSet;

	ID id;

	static inline ID dispenser = 0; /* in case we support user predicates */
	static const std::unordered_map<Predicate::Builtin, PredicateInfo> builtins;
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
	using PredSet = std::set<Predicate>;

	using iterator = PredSet::iterator;
	using const_iterator = PredSet::const_iterator;

public:
	PredicateSet() = default;
	PredicateSet(Predicate p) : preds_({p}) {}

	iterator begin() { return preds_.begin(); }
	iterator end() { return preds_.end(); }

	const_iterator begin() const { return preds_.begin(); }
	const_iterator end() const { return preds_.end(); }

	/* Whether the collection is empty */
	bool empty() const { return preds_.empty(); }

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
		return preds_ == other.preds_;
	}
	bool operator!=(const PredicateSet &other) const {
		return !(*this == other);
	}

	bool operator<(const PredicateSet &other) const {
		return preds_ < other.preds_;
	}
	bool operator>=(const PredicateSet &other) const {
		return !(*this < other);
	}

	bool operator>(const PredicateSet &other) const {
		return preds_ > other.preds_;
	}
	bool operator<=(const PredicateSet &other) const {
		return !(*this > other);
	}

	friend std::ostream &operator<<(std::ostream& ostr, const PredicateSet &s);

private:
	/* Simplifies the predicate set (e.g., if some predicate
	 * implies another). Idempotent. */
	void simplify();

	PredSet preds_;
};

#endif /* KATER_PREDICATE_HPP */
