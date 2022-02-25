#ifndef _REGEXP_HPP_
#define _REGEXP_HPP_

#include <string>
#include <utility>
#include "NFA.hpp"

class RegExp {
public:
	virtual ~RegExp() noexcept = default;
	virtual RegExp * clone () const = 0;
	virtual NFA toNFA () const = 0;
	virtual std::ostream & print (std::ostream& ostr) const = 0;
};

class CharRE : public RegExp {
public:
	std::string s;

	CharRE (const std::string &s) : s(s) { };
	virtual ~CharRE() noexcept override = default;

	RegExp * clone () const override;
	NFA toNFA () const override;	
	std::ostream & print (std::ostream& ostr) const override;

};

class AltRE : public RegExp {
public:
	std::unique_ptr<RegExp> exp1;
	std::unique_ptr<RegExp> exp2;

	AltRE (RegExp *r1, RegExp *r2) : exp1(r1), exp2(r2) { }
	~AltRE() noexcept override = default;

	RegExp * clone () const override;
	NFA toNFA () const override;
	std::ostream & print (std::ostream& ostr) const override;
};

class SeqRE : public RegExp {
public:
	std::unique_ptr<const RegExp> exp1;
	std::unique_ptr<const RegExp> exp2;

	SeqRE (const RegExp *r1, const RegExp *r2) : exp1(r1), exp2(r2) { }
	~SeqRE() noexcept override = default;

	RegExp * clone () const override;
	NFA toNFA () const override;
	std::ostream & print (std::ostream& ostr) const override;
};

class PlusRE : public RegExp {
public:
	std::unique_ptr<const RegExp> exp;

	PlusRE (const RegExp *r, int foo) : exp(r) { }
	~PlusRE() noexcept override = default;

	RegExp * clone () const override;
	NFA toNFA () const override;
	std::ostream & print (std::ostream& ostr) const override;
};

class StarRE : public RegExp {
public:
	std::unique_ptr<const RegExp> exp;

	StarRE (const RegExp *r, int foo) : exp(r) { }
	~StarRE() noexcept override = default;

	RegExp * clone () const override;
	NFA toNFA () const override;
	std::ostream & print (std::ostream& ostr) const override;
};

class QMarkRE : public RegExp {
public:
	std::unique_ptr<const RegExp> exp;

	QMarkRE (const RegExp *r, int foo) : exp(r) { }
	~QMarkRE() noexcept override = default;

	RegExp * clone () const override;
	NFA toNFA () const override;
	std::ostream & print (std::ostream& ostr) const override;
};

RegExp *make_BracketRE(RegExp *exp);

static inline std::ostream & operator<< (std::ostream& ostr, const RegExp& r) {
	return r.print(ostr);
}

#endif /* _REGEXP_HPP_ */
