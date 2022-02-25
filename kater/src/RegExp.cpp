#include <algorithm>
#include <fstream>
#include <iostream>
#include "RegExp.hpp"

RegExp *CharRE::clone () const
{
	return new CharRE(s);
}

NFA CharRE::toNFA () const
{
	return NFA::make_singleton (s);
}

std::ostream & CharRE::print (std::ostream& ostr) const
{
	ostr << s;
	return ostr;
}

RegExp *AltRE::clone () const
{
	return new AltRE(exp1->clone(), exp2->clone());
}

NFA AltRE::toNFA() const
{
	NFA nfa1 = exp1->toNFA();
	NFA nfa2 = exp2->toNFA();
	nfa1.alt(nfa2);
	return nfa1;
}

std::ostream & AltRE::print (std::ostream& ostr) const
{
	ostr << "(" << *exp1 << "|" << *exp2 << ")";
	return ostr;
}

RegExp *SeqRE::clone () const
{
	return new SeqRE(exp1->clone(), exp2->clone());
}

NFA SeqRE::toNFA() const
{
	NFA nfa1 = exp1->toNFA();
	NFA nfa2 = exp2->toNFA();
	nfa1.seq(nfa2);
	return nfa1;
}

std::ostream & SeqRE::print (std::ostream& ostr) const
{
	ostr << "(" << *exp1 << ";" << *exp2 << ")";
	return ostr;
}

RegExp *PlusRE::clone () const
{
	return new PlusRE(exp->clone(), 0);
}

NFA PlusRE::toNFA() const
{
	NFA nfa = exp->toNFA();
	nfa.plus();
	return nfa;
}

std::ostream & PlusRE::print (std::ostream& ostr) const
{
	ostr << *exp << "+";
	return ostr;
}

RegExp *StarRE::clone () const
{
	return new StarRE(exp->clone(), 0);
}

NFA StarRE::toNFA() const
{
	NFA nfa = exp->toNFA();
	nfa.star();
	return nfa;
}

std::ostream & StarRE::print (std::ostream& ostr) const
{
	ostr << *exp << "*";
	return ostr;
}

RegExp *QMarkRE::clone () const
{
	return new QMarkRE(exp->clone(), 0);
}

NFA QMarkRE::toNFA() const
{
	NFA nfa = exp->toNFA();
	nfa.or_empty();
	return nfa;
}

std::ostream & QMarkRE::print (std::ostream& ostr) const
{
	ostr << *exp << "*";
	return ostr;
}


RegExp *make_BracketRE(RegExp *exp)
{
	if (CharRE* e = dynamic_cast<CharRE*>(exp)) {
		e->s = "[" + e->s + "]";
		return e;
	}
	if (AltRE* e = dynamic_cast<AltRE*>(exp)) {
		make_BracketRE(e->exp1.get());
		make_BracketRE(e->exp2.get());
   		return e;
	}
	std::cerr << "Error: Cannot process [" << *exp << "]." << std::endl;
	return exp;
}
