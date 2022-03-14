#ifndef _KATER_TRANS_LABEL_HPP_
#define _KATER_TRANS_LABEL_HPP_

#include <iostream>
#include <string>
#include <vector>

enum class RelType  { OneOne, ManyOne, UnsuppMany, Conj, Final };

struct PredicateInfo {
	std::string  name;
	std::string  genmcString;
};

struct RelationInfo {
	std::string  name;
	RelType      type;
	std::string  succString;
	std::string  predString;
};	

extern const std::vector<PredicateInfo> builtinPredicates;
extern const std::vector<RelationInfo> builtinRelations;

class TransLabel {
public:
	TransLabel() = default;
	TransLabel(int s) : trans(s), flipped(false) {}

	bool isPredicate () const { return trans < builtinPredicates.size(); }
	bool isRelation () const { return trans >= builtinPredicates.size(); }
	bool isBuiltin () const { return trans < builtinPredicates.size() + builtinRelations.size(); }

	static TransLabel getFreshCalcLabel() {
		return TransLabel(builtinPredicates.size() + builtinRelations.size() + calcNum++);
	}

	TransLabel &flip() { flipped = !flipped; return *this; }

	void output_for_genmc (std::ostream& ostr, const std::string &arg,
			      const std::string &res) const;

	bool operator< (const TransLabel &other) const { return trans < other.trans; }

	bool operator== (const TransLabel &other) const { return trans == other.trans; }

	bool operator!= (const TransLabel &other) const { return trans != other.trans; }

	friend std::ostream &operator<<(std::ostream &s, const TransLabel &t);

private:
	int trans;
	bool flipped;
	static int calcNum;
};

#endif /* _KATER_TRANS_LABEL_HPP_ */
