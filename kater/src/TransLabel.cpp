#include "TransLabel.hpp"
#include <cassert>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <vector>

template<typename T>
static std::ostream & operator<< (std::ostream& ostr, const std::set<T> &s)
{
	bool not_first = false;
	for (auto &i : s) {
		if (not_first) ostr << "&";
		else not_first = true;
		ostr << i;
	}
	return ostr;
}

std::ostream & operator<< (std::ostream& ostr, const TransLabel & t)
{
	if (!t.is_valid()) return ostr << "INVALID";
	if (!t.pre_checks.empty()) {
		ostr << "[" << t.pre_checks << "]";
		if (!t.is_empty_trans()) ostr << ";";
	}
	if (!t.is_empty_trans()) ostr << t.trans;
	if (!t.post_checks.empty())
		ostr << ";[" << t.post_checks << "]";
	return ostr;
}

/*
static std::unordered_map<std::string, int> string_to_id_map {};
static int string_to_id_map_count = 0;

static int string_to_id (const std::string & s)
{
	auto &res = string_to_id_map.find(s);
	if (res != string_to_id_map.end())
		return res->second;
	int c = ++string_to_id_map_count;
	string_to_id_map.insert({s, c});
	return c;
}
*/

static std::vector<std::set<std::string> > invalids = {};
static std::vector<TransLabel> invalids2 = {};

void TransLabel::register_invalid (const TransLabel &l)
{
	// Do not register already-invalid equations
	if (!l.is_valid()) return;

	if (l.is_empty_trans()) {
		if (std::find (invalids.begin(), invalids.end(), l.pre_checks) == invalids.end())
			invalids.push_back(l.pre_checks);
	} else {
		if (std::find (invalids2.begin(), invalids2.end(), l) == invalids2.end())
			invalids2.push_back(l);
	}
}

TransLabel TransLabel::make_trans_label (const std::string & s)
{
	TransLabel t;
	t.trans = s;
	return t;
}

void TransLabel::make_bracket ()
{
	assert (pre_checks.empty() && post_checks.empty());
	pre_checks.insert (trans);
	trans.clear();
}

TransLabel::my_set TransLabel::merge_sets (const TransLabel::my_set &a,
					   const TransLabel::my_set &b)
{
	/* `r = a \cup b` */
	auto r = a;
	for (const auto &e : b) r.insert(e);

	/* `if (\exists s \in invalids. s \subseteq r) r = \emptyset` */
	auto is_in_r = [&](const std::string &s) {
		return r.find(s) != r.end();
	};
	auto subset_of_r = [&](const TransLabel::my_set &s) {
		return std::all_of (s.begin(), s.end(), is_in_r);
	};
	if (std::any_of (invalids.begin(), invalids.end(), subset_of_r))
		r.clear();
	return r;
}

void TransLabel::flip ()
{
	if (is_empty_trans()) return;
	std::swap (pre_checks, post_checks);
}

TransLabel TransLabel::seq (const TransLabel &other) const
{
	TransLabel r;
	if (!is_valid() || !other.is_valid())
		return r;
	if (is_empty_trans()) {
		r.pre_checks = TransLabel::merge_sets (pre_checks, other.pre_checks);
		if (!r.pre_checks.empty()) { /* is valid? */
			r.trans = other.trans;
			r.post_checks = other.post_checks;
		}
	}
	else {
		assert (other.is_empty_trans());
		r.post_checks = TransLabel::merge_sets (post_checks, other.pre_checks);
		if (!r.post_checks.empty()) { /* is valid? */
			r.trans = trans;
			r.pre_checks = pre_checks;
		}
	}
	if (!r.is_empty_trans()) {
		for (const auto &i : invalids2) {
			if (i.trans != r.trans) continue;
			if (std::any_of(i.pre_checks.begin(), i.pre_checks.end(),
					[&](const std::string &s) {
					return r.pre_checks.find(s) == r.pre_checks.end();
					}))
				continue;
			if (std::any_of(i.post_checks.begin(), i.post_checks.end(),
					[&](const std::string &s) {
					return r.post_checks.find(s) == r.post_checks.end();
					}))
				continue;
			r.pre_checks.clear();
			r.trans.clear();
			r.post_checks.clear();
			return r;
		}
	}
	return r;
}
