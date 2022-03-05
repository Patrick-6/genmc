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
		assert(i < kids.size() && "Index out of bounds!");
		return kids[i].get();
	}
	RegExp *getKid(unsigned i) {
		return const_cast<RegExp *>(static_cast<const RegExp &>(*this).getKid(i));
	}

	/* Sets the i-th kid to e */
	void setKid(unsigned i, std::unique_ptr<RegExp> e) {
		assert(i < kids.size() && "Index out of bounds!");
		kids[i] = std::move(e);
	}

	/* Returns whether this RE has kids */
	bool hasKids() const { return !kids.empty(); }

	/* The kids of this RE */
	size_t getNumKids() const { return kids.size(); }

	/* Optimizes RE, but in-place */
	static std::unique_ptr<RegExp> optimize(std::unique_ptr<RegExp> re);

	/* RE -> [RE] */
	virtual void bracket() = 0;

	/* Convert the RE to an NFA */
	virtual NFA toNFA() const = 0;

	/* Returns a clone of the RE */
	virtual std::unique_ptr<RegExp> clone() const = 0;

	/* Dumpts the RE */
	virtual std::ostream &dump(std::ostream &s) const = 0;

	friend std::ostream &operator<<(std::ostream &s, const RegExp& r);

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


/*******************************************************************************
 **                               CharRE Class
 ******************************************************************************/

class CharRE : public RegExp {

protected:
	CharRE(const TransLabel &l) : RegExp(), lab(l) {}

public:
	template<typename... Ts>
	static std::unique_ptr<CharRE> create(Ts&&... params) {
		return std::unique_ptr<CharRE>(
			new CharRE(std::forward<Ts>(params)...));
	}

	/* Returns the transition label */
	const TransLabel &getLabel() const { return lab; }
	TransLabel &getLabel() { return lab; }

	/* Sets the transition label */
	void setLabel(const TransLabel &l) { lab = l; }

	void bracket() override { getLabel().make_bracket(); }

	NFA toNFA() const override { return NFA::make_singleton(lab); }

	std::unique_ptr<RegExp> clone() const override { return create(getLabel()); }

	std::ostream &dump(std::ostream &s) const override { return s << getLabel(); }

protected:
	TransLabel lab;
};


/*******************************************************************************
 **                           Binary REs
 ******************************************************************************/

class BinaryRE : public RegExp {

protected:
	BinaryRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: RegExp() { addKid(std::move(r1)); addKid(std::move(r2)); }

public:
	virtual void bracket() override {
		getKid(0)->bracket();
		getKid(1)->bracket();
	}
};


/*
 * RE_1 | RE_2
 */
class AltRE : public BinaryRE {

protected:
	AltRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: BinaryRE(std::move(r1), std::move(r2)) {}

public:
	template<typename... Ts>
	static std::unique_ptr<AltRE> create(Ts&&... params) {
		return std::unique_ptr<AltRE>(
			new AltRE(std::forward<Ts>(params)...));
	}

	std::unique_ptr<RegExp> clone () const override	{
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	NFA toNFA() const override {
		NFA nfa1 = getKid(0)->toNFA();
		NFA nfa2 = getKid(1)->toNFA();
		nfa1.alt(nfa2);
		return nfa1;
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "(" << *getKid(0) << "|" << *getKid(1) << ")";
	}
};


/*
 * RE_1 ; RE_2
 */
class SeqRE : public BinaryRE {

protected:
	SeqRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: BinaryRE(std::move(r1), std::move(r2)) {}

public:
	template<typename... Ts>
	static std::unique_ptr<SeqRE> create(Ts&&... params) {
		return std::unique_ptr<SeqRE>(
			new SeqRE(std::forward<Ts>(params)...));
	}

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
	}

	/* Tries to avoid creating an SeqRE if two CharREs are passed,
	 * one of which has an empty transition label. */
	static std::unique_ptr<RegExp>
	createOpt(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2) {
		auto *charRE1 = dynamic_cast<const CharRE *>(&*e1);
		auto *charRE2 = dynamic_cast<const CharRE *>(&*e2);
		if (charRE1 && charRE2 && (charRE1->getLabel().is_empty_trans() ||
					   charRE2->getLabel().is_empty_trans()))
			return CharRE::create(charRE1->getLabel().seq(charRE2->getLabel()));
		return create(std::move(e1), std::move(e2));
	}

	std::unique_ptr<RegExp> clone() const override	{
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	NFA toNFA() const override {
		NFA nfa1 = getKid(0)->toNFA();
		NFA nfa2 = getKid(1)->toNFA();
		nfa1.seq(nfa2);
		return nfa1;
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "(" << *getKid(0) << ";" << *getKid(1) << ")";
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
	void bracket() override	{						\
		std::cerr << "Error: Cannot process [" << *getKid(0)		\
			<< "]." << std::endl;					\
		return;								\
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
		return s << "(" << *getKid(0) << ") " << _str;			\
	}									\
};

UNARY_RE(Plus, plus, "+");
UNARY_RE(Star, star, "*");
UNARY_RE(QMark, or_empty, "?");

inline std::ostream &operator<<(std::ostream &s, const RegExp& re)
{
	return re.dump(s);
}

#endif /* _REGEXP_HPP_ */
