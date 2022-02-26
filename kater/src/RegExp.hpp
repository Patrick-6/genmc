#ifndef _REGEXP_HPP_
#define _REGEXP_HPP_

#include <memory>
#include <string>
#include <utility>
#include "TransLabel.hpp"
#include "NFA.hpp"

class RegExp {
public:
	virtual ~RegExp() = default;
	virtual std::unique_ptr<RegExp> clone () const = 0;
	virtual NFA toNFA () const = 0;
	virtual std::ostream & print (std::ostream& ostr) const = 0;
};

std::unique_ptr<RegExp> make_CharRE(const std::string &s);
std::unique_ptr<RegExp> make_BracketRE(std::unique_ptr<RegExp> e);
std::unique_ptr<RegExp> make_AltRE(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2);
std::unique_ptr<RegExp> make_SeqRE(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2);
std::unique_ptr<RegExp> make_PlusRE(std::unique_ptr<RegExp> e);
std::unique_ptr<RegExp> make_StarRE(std::unique_ptr<RegExp> e);
std::unique_ptr<RegExp> make_QMarkRE(std::unique_ptr<RegExp> e);

std::pair<TransLabel, bool> has_trans_label (const RegExp &e);

static inline std::ostream & operator<< (std::ostream& ostr, const RegExp& r) {
	return r.print(ostr);
}

#endif /* _REGEXP_HPP_ */
