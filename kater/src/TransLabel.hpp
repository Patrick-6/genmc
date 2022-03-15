#ifndef _KATER_TRANS_LABEL_HPP_
#define _KATER_TRANS_LABEL_HPP_

#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <string>
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

/* ------------------------------------------------------------------ */
/*                     Abstract class for labels                      */
/* ------------------------------------------------------------------ */

class TransLabel {
public:
	bool operator<= (const TransLabel &other) const { return other < *this || *this == other; }
	bool operator!= (const TransLabel &other) const { return !(*this == other); }
	bool operator>  (const TransLabel &other) const { return other < *this; }
	bool operator>= (const TransLabel &other) const { return other <= *this; }

	virtual ~TransLabel() = default;

	virtual void flip() { }

	virtual std::unique_ptr<TransLabel> clone() const = 0;

	virtual std::string toString() const = 0;
	virtual void output_for_genmc (std::ostream& ostr, const std::string &arg,
			      const std::string &res) const = 0;
	virtual bool operator< (const TransLabel &other) const = 0;
	virtual bool operator== (const TransLabel &other) const = 0;
};

static inline std::ostream &operator<<(std::ostream &s, const TransLabel &t) {
	return s << t.toString();
}

/* ------------------------------------------------------------------ */
/*                     Predicate labels                               */
/* ------------------------------------------------------------------ */

class PredLabel : public TransLabel {
public:
	PredLabel() = default;
	PredLabel(int s) : preds({s}) {}

	std::unique_ptr<TransLabel> clone() const override {
		return std::unique_ptr<PredLabel>(new PredLabel(*this));
	}

	std::string toString() const override;

	bool merge (const PredLabel &other);

	void output_for_genmc (std::ostream& ostr, const std::string &arg,
			      const std::string &res) const override;

	bool operator< (const TransLabel &other) const override {
		if (auto *o = dynamic_cast<const PredLabel *>(&other))
			return preds < o->preds;
		return false;
	}

	bool operator== (const TransLabel &other) const override {
		if (auto *o = dynamic_cast<const PredLabel *>(&other))
			return preds == o->preds;
		return false;
	}

private:
	std::set<int> preds;
};

/* ------------------------------------------------------------------ */
/*                     Relation labels                                */
/* ------------------------------------------------------------------ */

class RelLabel : public TransLabel {
public:
	RelLabel(int s) : trans(s), flipped(false) {}

	bool isBuiltin () const { return trans >= 0; }
	int getCalcIndex () const { assert(!isBuiltin()); return -trans-1; }

	static RelLabel getFreshCalcLabel() {
		return RelLabel(--calcNum);
	}

	std::unique_ptr<TransLabel> clone() const override {
		return std::unique_ptr<RelLabel>(new RelLabel(*this));
	}

	void flip() override { flipped = !flipped; }

	std::string toString() const override;

	void output_for_genmc (std::ostream& ostr, const std::string &arg,
			      const std::string &res) const override;

	bool operator< (const TransLabel &other) const override {
		if (auto *o = dynamic_cast<const RelLabel *>(&other))
			return trans < o->trans
				|| (trans == o->trans && !flipped && o->flipped);
		return true;
	}

	bool operator== (const TransLabel &other) const override {
		if (auto *o = dynamic_cast<const RelLabel *>(&other))
			return trans == o->trans && flipped == o->flipped;
		return false;
	}

private:
	int trans;
	bool flipped;
	static int calcNum;
};


#endif /* _KATER_TRANS_LABEL_HPP_ */
