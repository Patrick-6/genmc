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
		return ostr << "(" << *exp1 << ";" << *exp2 << ")";
	}
};

class PlusRE : public RegExp {
public:
	std::unique_ptr<const RegExp> exp;

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
	std::unique_ptr<const RegExp> exp;

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
	std::unique_ptr<const RegExp> exp;

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


static RegExp * make_BracketRE_recursive(RegExp *exp);

static RegExp * make_BracketRE_recursive(RegExp *exp)
{
	if (auto e = dynamic_cast<CharRE*>(exp)) {
		e->lab.make_bracket();
		return e;
	}
	if (auto e = dynamic_cast<AltRE*>(exp)) {
		make_BracketRE_recursive(e->exp1.get());
		make_BracketRE_recursive(e->exp2.get());
   		return e;
	}
	if (auto e = dynamic_cast<SeqRE*>(exp)) {
		auto e1 = make_BracketRE_recursive(e->exp1.get());
		auto e2 = make_BracketRE_recursive(e->exp2.get());
		return e;
	}
	std::cerr << "Error: Cannot process [" << *exp << "]." << std::endl;
	return exp;
}


std::unique_ptr<RegExp> make_BracketRE(std::unique_ptr<RegExp> e)
{
	make_BracketRE_recursive(e.get());
	return e;
}

