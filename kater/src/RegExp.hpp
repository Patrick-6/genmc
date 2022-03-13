#ifndef _REGEXP_HPP_
#define _REGEXP_HPP_

#include "TransLabel.hpp"
#include "NFA.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <utility>

/*******************************************************************************
 **                           RegExp Class (Abstract)
 ******************************************************************************/

class RegExp {

protected:
	RegExp(std::vector<std::unique_ptr<RegExp> > &&kids = {})
		: kids(std::move(kids)) {}
public:
	virtual ~RegExp() = default;

	/* Fetches the i-th kid */
	const RegExp *getKid(unsigned i) const {
		assert(i < getNumKids() && "Index out of bounds!");
		return kids[i].get();
	}
	RegExp *getKid(unsigned i) {
		return const_cast<RegExp *>(static_cast<const RegExp &>(*this).getKid(i));
	}

	/* Sets the i-th kid to e */
	void setKid(unsigned i, std::unique_ptr<RegExp> e) {
		assert(i < getNumKids() && "Index out of bounds!");
		kids[i] = std::move(e);
	}

	/* Releases ownership of i-th kid */
	std::unique_ptr<RegExp> releaseKid(unsigned i) {
		assert(i < getNumKids() && "Index out of bounds!");
		return std::move(kids[i]);
	}

	/* Returns whether this RE has kids */
	bool hasKids() const { return !kids.empty(); }

	/* The kids of this RE */
	size_t getNumKids() const { return kids.size(); }

	/* Optimizes RE, but in-place */
	static std::unique_ptr<RegExp> optimize(std::unique_ptr<RegExp> re);

	/* Convert the RE to an NFA */
	virtual NFA toNFA() const = 0;

	/* Returns a clone of the RE */
	virtual std::unique_ptr<RegExp> clone() const = 0;

	/* Dumpts the RE */
	virtual std::ostream &dump(std::ostream &s) const = 0;

	bool isFalse () const;
protected:
	using KidsC = std::vector<std::unique_ptr<RegExp>>;

	const KidsC &getKids() const {
		return kids;
	}

	void addKid(std::unique_ptr<RegExp> k) {
		kids.push_back(std::move(k));
	}

private:
	KidsC kids;
};

inline std::ostream &operator<<(std::ostream &s, const RegExp& re)
{
	return re.dump(s);
}

/*******************************************************************************
 **                               Singleton 
 ******************************************************************************/

class PredRE : public RegExp {
protected:
	PredRE(const TransLabel &l) : RegExp(), lab(l) {}

public:
	template<typename... Ts>
	static std::unique_ptr<PredRE> create(Ts&&... params) {
		return std::unique_ptr<PredRE>(
			new PredRE(std::forward<Ts>(params)...));
	}

	/* Returns the transition label */
	const TransLabel &getLabel() const { return lab; }
	TransLabel &getLabel() { return lab; }

	/* Sets the transition label */
	void setLabel(const TransLabel &l) { lab = l; }

	NFA toNFA() const override { return NFA(lab); }

	std::unique_ptr<RegExp> clone() const override { return create(getLabel()); }

	std::ostream &dump(std::ostream &s) const override { return s << getLabel(); }

protected:
	TransLabel lab;
};

class RelRE : public RegExp {

protected:
	RelRE(const TransLabel &l) : RegExp(), lab(l) {}

public:
	template<typename... Ts>
	static std::unique_ptr<RelRE> create(Ts&&... params) {
		return std::unique_ptr<RelRE>(
			new RelRE(std::forward<Ts>(params)...));
	}

	/* Returns the transition label */
	const TransLabel &getLabel() const { return lab; }
	TransLabel &getLabel() { return lab; }

	/* Sets the transition label */
	void setLabel(const TransLabel &l) { lab = l; }

	NFA toNFA() const override { return NFA(lab); }

	std::unique_ptr<RegExp> clone() const override { return create(getLabel()); }

	std::ostream &dump(std::ostream &s) const override { return s << getLabel(); }

protected:
	TransLabel lab;
};


/*
 * RE_1 | RE_2
 */
class AltRE : public RegExp {

protected:
	AltRE(std::vector<std::unique_ptr<RegExp> > &&kids = {})
                : RegExp(std::move(kids)) {}
	AltRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: RegExp() { addKid(std::move(r1)); addKid(std::move(r2)); }

public:
	template<typename... Ts>
	static std::unique_ptr<AltRE> create(Ts&&... params) {
		return std::unique_ptr<AltRE>(
			new AltRE(std::forward<Ts>(params)...));
	}

	static std::unique_ptr<RegExp>
	createOpt (std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2);

	std::unique_ptr<RegExp> clone () const override	{
		std::vector<std::unique_ptr<RegExp> > nk;
		for (auto &k : getKids())
			nk.push_back(std::move(k->clone()));
		return create(std::move(nk));
	}

	NFA toNFA() const override {
		NFA nfa;
		for (const auto &k : getKids())
			 nfa.alt(std::move(k->toNFA()));
		return nfa;
	}

	std::ostream &dump(std::ostream &s) const override {
		if (getNumKids() == 0)
			return s << "0";
		s << "(" << *getKid(0);
		for (int i = 1; i < getNumKids(); i++)
			s << " | " << *getKid(i);
		return s << ")";
	}
};



/*
 * RE_1 ; RE_2
 */
class SeqRE : public RegExp {

protected:
	SeqRE(std::vector<std::unique_ptr<RegExp> > &&kids = {})
                : RegExp(std::move(kids)) {}
	SeqRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: RegExp() { addKid(std::move(r1)); addKid(std::move(r2)); }

public:
	template<typename... Ts>
	static std::unique_ptr<SeqRE> create(Ts&&... params) {
		return std::unique_ptr<SeqRE>(
			new SeqRE(std::forward<Ts>(params)...));
	}

	static std::unique_ptr<RegExp>
	createOpt (std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2);

	/*
	static std::unique_ptr<RegExp>
	createOrChar(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2) {
		if (auto *charRE1 = dynamic_cast<CharRE *>(&*e1)) {
			if (auto *charRE2 = dynamic_cast<CharRE *>(&*e2)) {
				if (charRE1->getLabel().is_empty_trans() &&
				    charRE2->getLabel().is_empty_trans()) {
					charRE1->setLabel(charRE1->getLabel().seq(charRE2->getLabel()));
					return std::move(e1);
				}
			}
		}
		return create(std::move(e1), std::move(e2));
	} */

	/* Tries to avoid creating an SeqRE if two CharREs are passed,
	 * one of which has an empty transition label. */
	/* static std::unique_ptr<RegExp>
	createOpt(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2) {
		auto *charRE1 = dynamic_cast<const CharRE *>(&*e1);
		auto *charRE2 = dynamic_cast<const CharRE *>(&*e2);
		if (charRE1 && charRE2 && (charRE1->getLabel().is_empty_trans() ||
					   charRE2->getLabel().is_empty_trans()))
			return CharRE::create(charRE1->getLabel().seq(charRE2->getLabel()));
		return create(std::move(e1), std::move(e2));
	} */

	std::unique_ptr<RegExp> clone () const override	{
		std::vector<std::unique_ptr<RegExp> > nk;
		for (auto &k : getKids())
			nk.push_back(std::move(k->clone()));
		return create(std::move(nk));
	}

	NFA toNFA() const override {
		NFA nfa;
		nfa.or_empty();
		for (const auto &k : getKids())
			 nfa.seq(std::move(k->toNFA()));
		return nfa;
	}

	std::ostream &dump(std::ostream &s) const override {
		if (getNumKids() == 0)
			return s << "<empty>";
		s << "(" << *getKid(0);
		for (int i = 1; i < getNumKids(); i++)
			s << " ; " << *getKid(i);
		return s << ")";
	}
};

/*******************************************************************************
 **                           Binary REs
 ******************************************************************************/

class BinaryRE : public RegExp {

protected:
	BinaryRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: RegExp() { addKid(std::move(r1)); addKid(std::move(r2)); }

};

/*
 * RE_1 \ RE_2
 */
class MinusRE : public BinaryRE {

protected:
	MinusRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: BinaryRE(std::move(r1), std::move(r2)) {}

public:
	template<typename... Ts>
	static std::unique_ptr<MinusRE> create(Ts&&... params) {
		return std::unique_ptr<MinusRE>(
			new MinusRE(std::forward<Ts>(params)...));
	}

	std::unique_ptr<RegExp> clone () const override	{
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	NFA toNFA() const override {
		std::cerr << "[Error] NFA conversion of minus(\\) expressions is not supported." << std::endl;
		NFA nfa1 = getKid(0)->toNFA();
		return nfa1;
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "(" << *getKid(0) << " \\ " << *getKid(1) << ")";
	}
};



/*******************************************************************************
 **                         Unary operations on REs
 ******************************************************************************/

#define UNARY_RE(_class, _op, _str)						\
class _class##RE : public RegExp {						\
										\
protected:									\
	_class##RE(std::unique_ptr<RegExp> r)					\
		: RegExp() { addKid(std::move(r)); }				\
										\
public:										\
	template<typename... Ts>						\
	static std::unique_ptr<_class##RE> create(Ts&&... params) {		\
		return std::unique_ptr<_class##RE>(				\
			new _class##RE(std::forward<Ts>(params)...));		\
	}									\
										\
	std::unique_ptr<RegExp> clone () const override	{			\
		return create(getKid(0)->clone());				\
	}									\
										\
	NFA toNFA() const override {						\
		NFA nfa = getKid(0)->toNFA();					\
		nfa._op();							\
		return nfa;							\
	}									\
										\
	std::ostream &dump(std::ostream &s) const override {			\
		return s << *getKid(0) <<  _str;				\
	}									\
};

UNARY_RE(Plus, plus, "+");
UNARY_RE(Star, star, "*");
UNARY_RE(QMark, or_empty, "?");
UNARY_RE(Inv, flip, "^-1");

std::unique_ptr<RegExp> SymRE_create(std::unique_ptr<RegExp> r);

#endif /* _REGEXP_HPP_ */
