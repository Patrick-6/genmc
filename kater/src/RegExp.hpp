#ifndef _REGEXP_HPP_
#define _REGEXP_HPP_

#include "TransLabel.hpp"
#include "NFA.hpp"
#include <cassert>
#include <memory>
#include <typeinfo>
#include <string>
#include <utility>

/*******************************************************************************
 **                           RegExp Class (Abstract)
 ******************************************************************************/

class AltRE;

class RegExp {

protected:
	RegExp(std::vector<std::unique_ptr<RegExp> > &&kids = {})
		: kids(std::move(kids)) {}
public:
	virtual ~RegExp() = default;

	static std::unique_ptr<RegExp> createFalse();

	static std::unique_ptr<RegExp> createSym(std::unique_ptr<RegExp> re);

	using kid_iterator = std::vector<std::unique_ptr<RegExp>>::iterator;
	using kid_const_iterator = std::vector<std::unique_ptr<RegExp>>::const_iterator;

	kid_iterator kid_begin() { return getKids().begin(); }
	kid_iterator kid_end() { return getKids().end(); }

	kid_const_iterator kid_begin() const { return getKids().begin(); }
	kid_const_iterator kid_end() const { return getKids().end(); }

	/* Fetches the i-th kid */
	const RegExp *getKid(unsigned i) const {
		assert(i < getNumKids() && "Index out of bounds!");
		return kids[i].get();
	}
	std::unique_ptr<RegExp> &getKid(unsigned i) {
		assert(i < getNumKids() && "Index out of bounds!");
		return kids[i];
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

	bool isFalse() const;

	virtual RegExp &flip() {
		for (auto &r : getKids())
			r->flip();
		return *this;
	}

	/* Convert the RE to an NFA */
	virtual NFA toNFA() const = 0;

	/* Returns a clone of the RE */
	virtual std::unique_ptr<RegExp> clone() const = 0;

	bool operator==(const RegExp &other) const {
		return typeid(*this) == typeid(other) && isEqual(other);
	}
	bool operator!=(const RegExp &other) const {
		return !(*this == other);
	}

	/* Dumpts the RE */
	virtual std::ostream &dump(std::ostream &s) const = 0;

protected:
	virtual bool isEqual(const RegExp &other) const = 0;

	using KidsC = std::vector<std::unique_ptr<RegExp>>;

	const KidsC &getKids() const { return kids; }
	KidsC &getKids() { return kids;	}

	void addKid(std::unique_ptr<RegExp> k) {
		kids.push_back(std::move(k));
	}

	KidsC kids;
};

inline std::ostream &operator<<(std::ostream &s, const RegExp& re)
{
	return re.dump(s);
}

/*******************************************************************************
 **                               Singleton
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

	RegExp &flip() override {
		lab.flip();
		return *this;
	}

	NFA toNFA() const override { return NFA(lab); }

	std::unique_ptr<RegExp> clone() const override { return create(getLabel()); }

	std::ostream &dump(std::ostream &s) const override { return s << getLabel(); }
protected:
	bool isEqual(const RegExp &other) const override {
		return getLabel() == static_cast<const CharRE &>(other).getLabel();
	}

	TransLabel lab;
};


/*
 * RE_1 | RE_2
 */
class AltRE : public RegExp {

protected:
	template<typename... Ts>
	AltRE(Ts&&... args)
		: RegExp() { (addKid(std::move(args)), ...); }
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

	template<typename... Ts>
	static std::unique_ptr<RegExp> createOpt(Ts&&... params);

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

protected:
	bool isEqual(const RegExp &other) const override {
		auto &o = static_cast<const AltRE &>(other);
		auto i = 0u;
		return getNumKids() == other.getNumKids() &&
			std::all_of(kid_begin(), kid_end(), [&](auto &k){
				auto res = (*getKid(i) == *o.getKid(i));
				++i;
				return res;
			});
	}
};



/*
 * RE_1 ; RE_2
 */
class SeqRE : public RegExp {

protected:
	template<typename... Ts>
	SeqRE(Ts&&... args)
		: RegExp() { (addKid(std::move(args)), ...); }
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

	/* Tries to avoid creating an SeqRE if (at least) an epsilon
	 * CharRE is passed */
	template<typename... Ts>
	static std::unique_ptr<RegExp> createOpt(Ts&&... args);

	RegExp &flip() override {
		for (auto &r : getKids())
			r->flip();
		std::reverse(kids.begin(), kids.end());
		return *this;
	}

	NFA toNFA() const override {
		NFA nfa;
		nfa.or_empty();
		for (const auto &k : getKids())
			 nfa.seq(std::move(k->toNFA()));
		return nfa;
	}

	std::unique_ptr<RegExp> clone () const override	{
		std::vector<std::unique_ptr<RegExp> > nk;
		for (auto &k : getKids())
			nk.push_back(std::move(k->clone()));
		return create(std::move(nk));
	}

	std::ostream &dump(std::ostream &s) const override {
		if (getNumKids() == 0)
			return s << "<empty>";
		s << "(" << *getKid(0);
		for (int i = 1; i < getNumKids(); i++)
			s << " ; " << *getKid(i);
		return s << ")";
	}

protected:
	bool isEqual(const RegExp &other) const override {
		auto &o = static_cast<const SeqRE &>(other);
		auto i = 0u;
		return getNumKids() == other.getNumKids() &&
			std::all_of(kid_begin(), kid_end(), [&](auto &k){
				auto res = (*getKid(i) == *o.getKid(i));
				++i;
				return res;
			});
	}
};

/*******************************************************************************
 **                           Binary REs
 ******************************************************************************/

class BinaryRE : public RegExp {

protected:
	BinaryRE(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: RegExp() { addKid(std::move(r1)); addKid(std::move(r2)); }

	bool isEqual(const RegExp &other) const override {
		auto &o = static_cast<const BinaryRE &>(other);
		auto i = 0u;
		return getNumKids() == other.getNumKids() &&
			std::all_of(kid_begin(), kid_end(), [&](auto &k){
				auto res = (*getKid(i) == *o.getKid(i));
				++i;
				return res;
			});
	}
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

	NFA toNFA() const override {
		std::cerr << "[Error] NFA conversion of minus(\\) expressions is not supported." << std::endl;
		NFA nfa1 = getKid(0)->toNFA();
		return nfa1;
	}

	std::unique_ptr<RegExp> clone () const override	{
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "(" << *getKid(0) << " \\ " << *getKid(1) << ")";
	}

protected:
	bool isEqual(const RegExp &other) const override {
		auto &o = static_cast<const MinusRE &>(other);
		auto i = 0u;
		return getNumKids() == other.getNumKids() &&
			std::all_of(kid_begin(), kid_end(), [&](auto &k){
				auto res = (*getKid(i) == *o.getKid(i));
				++i;
				return res;
			});
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
	static std::unique_ptr<RegExp> createOpt (std::unique_ptr<RegExp> r); 	\
										\
	NFA toNFA() const override {						\
		NFA nfa = getKid(0)->toNFA();					\
		nfa._op();							\
		return nfa;							\
	}									\
										\
	std::unique_ptr<RegExp> clone () const override	{			\
		return create(getKid(0)->clone());				\
	}									\
										\
	std::ostream &dump(std::ostream &s) const override {			\
		return s << *getKid(0) <<  _str;				\
	}									\
										\
protected:									\
	bool isEqual(const RegExp &other) const override {			\
		auto &o = static_cast<const _class##RE &>(other);		\
		auto i = 0u;							\
		return getNumKids() == other.getNumKids() &&			\
			std::all_of(kid_begin(), kid_end(), [&](auto &k){ 	\
				auto res = (*getKid(i) == *o.getKid(i));	\
				++i;						\
				return res;					\
			});							\
	}									\
};

UNARY_RE(Plus, plus, "+");
UNARY_RE(Star, star, "*");
UNARY_RE(QMark, or_empty, "?");


/*******************************************************************************
 **                         Helper functions
 ******************************************************************************/

template<typename OptT>
void addChildToVector(std::unique_ptr<RegExp> arg, std::vector<std::unique_ptr<RegExp>> &res)
{
	if (auto *re = dynamic_cast<const OptT *>(&*arg)) {
		for (auto i = 0u; i < arg->getNumKids(); i++)
			res.emplace_back(arg->releaseKid(i));
	} else {
		res.emplace_back(std::move(arg));
	}
}

template<typename OptT, typename... Ts>
std::vector<std::unique_ptr<RegExp>>
createOptChildVector(Ts... args)
{
	std::vector<std::unique_ptr<RegExp>> res;
	(addChildToVector<OptT>(std::move(args), res), ...);
	return res;
}

template<typename... Ts>
std::unique_ptr<RegExp>
AltRE::createOpt(Ts&&... args)
{
	auto r = createOptChildVector<AltRE>(std::forward<Ts>(args)...);
	std::sort(r.begin(), r.end());
	r.erase(std::unique(r.begin(), r.end()), r.end());
	return r.size() == 1 ? std::move(*r.begin()) : AltRE::create(std::move(r));
}

template<typename... Ts>
std::unique_ptr<RegExp>
SeqRE::createOpt(Ts&&... args)
{
	auto r = createOptChildVector<SeqRE>(std::forward<Ts>(args)...);

	auto it = std::find_if(r.begin(), r.end(), [&](auto &re){ return re->isFalse(); });
	if (it != r.end())
		return std::move(*it);

	for (auto it = r.begin(); it != r.end() && it+1 != r.end(); /* */) {
		auto *p = dynamic_cast<CharRE *>(it->get());
		auto *q = dynamic_cast<CharRE *>((it+1)->get());
		if (p && q && (p->getLabel().isPredicate() || q->getLabel().isPredicate())) {
			if (!p->getLabel().merge(q->getLabel()))
				return RegExp::createFalse();
			it = r.erase(it + 1);
			continue;
		}
		++it;
	}
	return r.size() == 1 ? std::move(*r.begin()) : SeqRE::create(std::move(r));
}

#endif /* _REGEXP_HPP_ */
