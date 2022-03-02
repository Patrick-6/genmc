#ifndef _KATER_TRANS_LABEL_HPP_
#define _KATER_TRANS_LABEL_HPP_

#include <iostream>
#include <set>
#include <string>

class TransLabel {
	using my_set = std::set<std::string>;
	std::string trans;
	my_set pre_checks;
	my_set post_checks;

	static my_set merge_sets (const my_set &a, const my_set &b);

public:
	bool is_empty_trans() const { return trans.empty(); }
	bool is_valid () const { return !trans.empty() || !pre_checks.empty(); }

	static TransLabel make_trans_label (const std::string &s);

	static void register_invalid (const TransLabel &t);

	TransLabel seq (const TransLabel &other) const;
	void flip();
	void make_bracket();

	void output_as_preds (std::ostream& ostr, const std::string &arg,
			      const std::string &res) const;

	friend std::ostream& operator<< (std::ostream& ostr, const TransLabel & t);

	bool operator< (const TransLabel &other) const
	{
		return trans < other.trans || (trans == other.trans &&
			(pre_checks < other.pre_checks ||
			(pre_checks == other.pre_checks && post_checks < other.post_checks)));
	}
	bool operator== (const TransLabel &other) const
	{
		return trans == other.trans && pre_checks == other.pre_checks
			&& post_checks == other.post_checks;
	}
};


#endif /* _KATER_TRANS_LABEL_HPP_ */
