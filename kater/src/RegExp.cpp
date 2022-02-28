#include <algorithm>
#include <fstream>
#include <iostream>
#include "RegExp.hpp"

struct CharRE : public RegExp {
public:
	TransLabel lab;

	CharRE (const TransLabel &l) : lab(l) { }
	virtual ~CharRE() noexcept override = default;

	std::unique_ptr<RegExp> clone () const override
	{
		return std::unique_ptr<RegExp>(new CharRE (lab));
	}

	NFA toNFA () const override
	{
		return NFA::make_singleton (lab);
	}

	std::ostream & print (std::ostream& ostr) const override
	{
		return ostr << lab;
	}
};

class AltRE : public RegExp {
public:
	std::unique_ptr<RegExp> exp1;
	std::unique_ptr<RegExp> exp2;

	AltRE (std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: exp1(std::move(r1)), exp2(std::move(r2)) { }

	~AltRE() noexcept override = default;

	std::unique_ptr<RegExp> clone () const override
	{
		return std::unique_ptr<RegExp>(new AltRE(exp1->clone(), exp2->clone()));
	}

	NFA toNFA() const override
	{
		NFA nfa1 = exp1->toNFA();
		NFA nfa2 = exp2->toNFA();
		nfa1.alt(nfa2);
		return nfa1;
	}

	std::ostream & print (std::ostream& ostr) const override
	{
		return ostr << "(" << *exp1 << "|" << *exp2 << ")";
	}

};

class SeqRE : public RegExp {
public:
	std::unique_ptr<RegExp> exp1;
	std::unique_ptr<RegExp> exp2;

	SeqRE (std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
		: exp1(std::move(r1)), exp2(std::move(r2)) { }

	~SeqRE() noexcept override = default;

	std::unique_ptr<RegExp> clone () const override
	{
		return std::unique_ptr<RegExp>(new SeqRE(exp1->clone(), exp2->clone()));
	}

	NFA toNFA() const override
	{
		NFA nfa1 = exp1->toNFA();
		NFA nfa2 = exp2->toNFA();
		nfa1.seq(nfa2);
		return nfa1;
	}

	std::ostream & print (std::ostream& ostr) const override
	{
		return ostr << "(" << *exp1 << " ; " << *exp2 << ")";
	}
};

class PlusRE : public RegExp {
public:
	std::unique_ptr<RegExp> exp;

	PlusRE (std::unique_ptr<RegExp> r) : exp(std::move(r)) { }

	~PlusRE() noexcept override = default;

	std::unique_ptr<RegExp> clone () const override
	{
		return std::unique_ptr<RegExp>(new PlusRE(exp->clone()));
	}

	NFA toNFA() const override
	{
		NFA nfa = exp->toNFA();
		nfa.plus();
		return nfa;
	}

	std::ostream & print (std::ostream& ostr) const override
	{
		return ostr << "(" << *exp << ")+";
	}
};

class StarRE : public RegExp {
public:
	std::unique_ptr<RegExp> exp;

	StarRE (std::unique_ptr<RegExp> r) : exp(std::move(r)) { }

	~StarRE() noexcept override = default;

	std::unique_ptr<RegExp> clone () const override
	{
		return std::unique_ptr<RegExp>(new StarRE(exp->clone()));
	}

	NFA toNFA() const override
	{
		NFA nfa = exp->toNFA();
		nfa.star();
		return nfa;
	}

	std::ostream & print (std::ostream& ostr) const override
	{
		return ostr << "(" << *exp << ")*";
	}
};

class QMarkRE : public RegExp {
public:
	std::unique_ptr<RegExp> exp;

	QMarkRE (std::unique_ptr<RegExp> r) : exp(std::move(r)) { }

	~QMarkRE() noexcept override = default;

	std::unique_ptr<RegExp> clone () const override
	{
		return std::unique_ptr<RegExp>(new QMarkRE(exp->clone()));
	}

	NFA toNFA() const override
	{
		NFA nfa = exp->toNFA();
		nfa.or_empty();
		return nfa;
	}

	std::ostream & print (std::ostream& ostr) const override
	{
		return ostr << "(" << *exp << ")?";
	}
};

std::unique_ptr<RegExp> make_CharRE(const std::string &s)
{
	return std::unique_ptr<RegExp>(new CharRE(TransLabel::make_trans_label(s)));
}

std::unique_ptr<RegExp> make_AltRE(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2)
{
	return std::unique_ptr<RegExp>(new AltRE(std::move(e1), std::move(e2)));
}

std::unique_ptr<RegExp> make_SeqRE(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2)
{
	if (auto ee1 = dynamic_cast<CharRE*>(e1.get()))
		if (auto ee2 = dynamic_cast<CharRE*>(e2.get()))
			if (ee1->lab.is_empty_trans() &&
			    ee2->lab.is_empty_trans()) {
				ee1->lab = ee1->lab.seq(ee2->lab);
				return std::move(e1);
			}
	return std::unique_ptr<RegExp>(new SeqRE(std::move(e1), std::move(e2)));
}

std::unique_ptr<RegExp> make_SeqRE_opt(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2)
{
	if (auto ee1 = dynamic_cast<CharRE*>(e1.get()))
		if (auto ee2 = dynamic_cast<CharRE*>(e2.get()))
			if (ee1->lab.is_empty_trans() ||
			    ee2->lab.is_empty_trans()) {
				ee1->lab = ee1->lab.seq(ee2->lab);
				return std::move(e1);
			}
	return std::unique_ptr<RegExp>(new SeqRE(std::move(e1), std::move(e2)));
}

std::unique_ptr<RegExp> make_PlusRE(std::unique_ptr<RegExp> e)
{
	return std::unique_ptr<RegExp>(new PlusRE(std::move(e)));
}

std::unique_ptr<RegExp> make_StarRE(std::unique_ptr<RegExp> e)
{
	return std::unique_ptr<RegExp>(new StarRE(std::move(e)));
}

std::unique_ptr<RegExp> make_QMarkRE(std::unique_ptr<RegExp> e)
{
	return std::unique_ptr<RegExp>(new QMarkRE(std::move(e)));
}

std::unique_ptr<RegExp> make_BracketRE(std::unique_ptr<RegExp> exp)
{
	if (auto e = dynamic_cast<CharRE*>(exp.get())) {
		e->lab.make_bracket();
		return std::move(exp);
	}
	if (auto e = dynamic_cast<AltRE*>(exp.get())) {
		e->exp1 = std::move(make_BracketRE(std::move(e->exp1)));
		e->exp2 = std::move(make_BracketRE(std::move(e->exp2)));
		return std::move(exp);
	}
	if (auto e = dynamic_cast<SeqRE*>(exp.get())) {
		return make_SeqRE(std::move(make_BracketRE(std::move(e->exp1))),
				  std::move(make_BracketRE(std::move(e->exp2))));
	}
	std::cerr << "Error: Cannot process [" << *exp << "]." << std::endl;
	return std::move(exp);
}

std::unique_ptr<RegExp> optimizeRE(std::unique_ptr<RegExp> exp)
{
	if (auto e = dynamic_cast<SeqRE*>(exp.get()))
		return std::move(make_SeqRE_opt(std::move(e->exp1), std::move(e->exp2)));
	if (auto e = dynamic_cast<AltRE*>(exp.get())) {
		e->exp1 = std::move(optimizeRE(std::move(e->exp1)));
		e->exp2 = std::move(optimizeRE(std::move(e->exp2)));
		return std::move(exp);
	}
	if (auto e = dynamic_cast<PlusRE*>(exp.get())) {
		e->exp = std::move(optimizeRE(std::move(e->exp)));
		return std::move(exp);
	}
	if (auto e = dynamic_cast<StarRE*>(exp.get())) {
		e->exp = std::move(optimizeRE(std::move(e->exp)));
		return std::move(exp);
	}
	if (auto e = dynamic_cast<QMarkRE*>(exp.get())) {
		e->exp = std::move(optimizeRE(std::move(e->exp)));
		return std::move(exp);
	}
	return std::move(exp);
}

std::pair<TransLabel, bool> has_trans_label (const RegExp &exp)
{
	if (auto e = dynamic_cast<const CharRE*>(&exp))
		return {e->lab, true};
	else {
		return {{}, false};
	}
}

