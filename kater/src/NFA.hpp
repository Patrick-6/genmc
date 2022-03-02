#ifndef _KATER_NFA_HPP_
#define _KATER_NFA_HPP_

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "TransLabel.hpp"

class NFA {
public:
	using tok_vector = std::vector< std::pair<TransLabel, int> >;
	using trans_t = std::vector<tok_vector>;

private:
	trans_t trans;
	trans_t trans_inv;
	std::set<int> starting;
	std::set<int> accepting;

	void remove_node (int n);
	void add_edge (int n, const TransLabel &t, int m);
	void remove_edge (int n, const TransLabel &t, int m);
	void add_outgoing_edges (int n, const tok_vector &v);
	void add_incoming_edges (int n, const tok_vector &v);

	void simplify_basic ();
	void compact_edges ();
	void scm_reduce ();
	std::vector<std::vector<char>> get_state_composition_matrix ();

public:
	bool contains_edge (int n, const TransLabel &t, int m) const;

	bool is_starting (int n) const;
	bool is_accepting (int n) const;

	static NFA make_empty();
	static NFA make_singleton(const TransLabel &t);

	void flip ();
	void alt (const NFA &other);
	void seq (const NFA &other);
	void or_empty ();
	void star ();
	void plus ();

	std::pair<NFA, std::vector<std::set<int>>> to_DFA () const;

	void simplify ();

	void print_calculator_header_public (std::ostream &ostr, int w);
	void print_calculator_header_private (std::ostream &ostr, int w);
	void print_calculator_impl (std::ostream &ostr, const std::string &name,
				    int w, bool reduce);
	void print_acyclic_header_public (std::ostream &ostr);
	void print_acyclic_header_private (std::ostream &ostr);
	void print_acyclic_impl (std::ostream &ostr, const std::string &name);

	friend std::ostream& operator<< (std::ostream& ostr, const NFA& nfa);
};

#endif /* _KATER_NFA_HPP_ */
