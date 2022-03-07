#ifndef _KATER_NFA_HPP_
#define _KATER_NFA_HPP_

#include "TransLabel.hpp"
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class NFA {
public:
	using tok_vector = std::vector< std::pair<TransLabel, int> >;
	using trans_t = std::vector<tok_vector>;

private:
	class State;

	/*
	 * A struct representing NFA transitions
	 */
	struct Transition {
		TransLabel label;
		State *dest;

		Transition() = delete;
		Transition(const TransLabel &lab, State *dest)
			: label(lab), dest(dest) {}

		/* Returns a transition with the label flipped,
		 * and the destination changed to DEST */
		Transition flipTo(State *s) const {
			Transition t(*this);
			t.label.flip();
			t.dest = s;
			return t;
		}

		inline bool operator==(const Transition &t) const {
			return dest == t.dest && label == t.label;
		}
		inline bool operator!=(const Transition &t) const {
			return !(*this == t);
		}

		inline bool operator<(const Transition &t) const {
			return (dest < t.dest) || (dest == t.dest && label < t.label);
		}
		inline bool operator>=(const Transition &t) const {
			return !(*this < t);
		}
		inline bool operator>(const Transition &t) const {
			return (*this >= t) && (*this != t);
		}
		inline bool operator<=(const Transition &t) const {
			return !(*this > t);
		}
	};


	/*
	 * A class representing NFA states.
	 * Each state maintains a collections of its transitions.
	 */
	class State {

	private:
		/* FIXME: Do not use set */
		using TransitionsC = std::set<Transition>;

	public:
		State() = delete;
		State(unsigned id) : id(id) {}

		using trans_iterator = TransitionsC::iterator;
		using trans_const_iterator = TransitionsC::const_iterator;

		trans_iterator out_begin() { return getOutgoing().begin(); }
		trans_iterator out_end() { return getOutgoing().end(); }

		trans_const_iterator out_begin() const { return getOutgoing().begin(); }
		trans_const_iterator out_end() const { return getOutgoing().end(); }

		trans_iterator in_begin() { return getIncoming().begin(); }
		trans_iterator in_end() { return getIncoming().end(); }

		trans_const_iterator in_begin() const { return getIncoming().begin(); }
		trans_const_iterator in_end() const { return getIncoming().end(); }

		/* Returns this state's ID */
		unsigned getId() const { return id; }

		/* Whether this state has an outgoing transition T */
		bool hasOutgoing(const Transition &t) const {
			return getOutgoing().count(t);
		}

		/* Whether this state has any outgoing transitions */
		bool hasOutgoing() const { return out_begin() == out_end(); }

		/* Whether this state has an inverse transition T */
		bool hasIncoming(const Transition &t) const {
			return getIncoming().count(t);
		}

		/* Whether this state has any incoming transitions */
		bool hasIncoming() const { return in_begin() == in_end(); }

		/* Adds an outgoing transition T (if it doesn't exist) */
		void addOutgoing(const Transition &t) {
			getOutgoing().insert(t);
		}

		/* Adds an incoming transition T (if it doesn't exist) */
		void addIncoming(const Transition &t) {
			getIncoming().insert(t);
		}

		/* Removes outgoing transition T from this state (if exists) */
		void removeOutgoing(const Transition &t) {
			getOutgoing().erase(t);
		}

		/* Removes incoming transition T from this state (if exists) */
		void removeIncoming(const Transition &t) {
			getIncoming().erase(t);
		}

		/* Removes all outgoing with S as their dest */
		void removeOutgoingTo(State *s) {
			for (auto it = out_begin(), ie = out_end(); it != ie; /* */) {
				if (it->dest == s)
					it = getOutgoing().erase(it);
				else
					++it;
			}
		}

		/* Removes all incoming with S as their dest */
		void removeIncomingTo(State *s) {
			for (auto it = in_begin(), ie = in_end(); it != ie; /* */) {
				if (it->dest == s)
					it = getIncoming().erase(it);
				else
					++it;
			}
		}

		/* Returns true if all outgoing transitions of this
		 * state return to this state */
		bool hasAllOutLoops() const {
			return std::all_of(out_begin(), out_end(), [&](const Transition &t){
				return t.dest == this;
			});
		}

		/* Returns true if all incoming transition of this
		 * state return to this state */
		bool hasAllInLoops() const {
			return std::all_of(in_begin(), in_end(), [&](const Transition &t){
				return t.dest == this;
			});
		}

		/* Whether all outgoing transitions are the same as the ones of S */
		bool outgoingSameAs(State *s) const {
			return getOutgoing() == s->getOutgoing();
		}

		/* Whether all incoming transitions are the same as the ones of S */
		bool incomingSameAs(State *s) const {
			return getIncoming() == s->getIncoming();
		}

		State &flip() {
			std::swap(getOutgoing(), getIncoming());
			return *this;
		}

	private:
		const TransitionsC &getOutgoing() const { return transitions; }
		TransitionsC &getOutgoing() { return transitions; }

		const TransitionsC &getIncoming() const { return inverse; }
		TransitionsC &getIncoming() { return inverse; }

		unsigned id;
		TransitionsC transitions;
		TransitionsC inverse;
	};

public:
	NFA() = default;
	NFA(const TransLabel &label);

	bool contains_edge (int n, const TransLabel &t, int m) const;

	bool is_starting (int n) const;
	bool is_accepting (int n) const;

	NFA &flip();
	NFA &alt(const NFA &other);
	NFA &seq(const NFA &other);
	NFA &or_empty();
	NFA &star();
	NFA &plus();

	NFA &simplify();
	NFA &simplifyReduce();

	std::pair<NFA, std::vector<std::set<int>>> to_DFA () const;

	void print_calculator_header_public (std::ostream &ostr, int w);
	void print_calculator_header_private (std::ostream &ostr, int w);

	void printCalculatorImpl(std::ostream &ostr, const std::string &name, int w) {
		printCalculatorImplHelper(ostr, name, w, false);
	}
	void printCalculatorImplReduce(std::ostream &ostr, const std::string &name, int w) {
		printCalculatorImplHelper(ostr, name, w, true);
	}

	void print_acyclic_header_public (std::ostream &ostr);
	void print_acyclic_header_private (std::ostream &ostr);
	void print_acyclic_impl (std::ostream &ostr, const std::string &name);

	friend std::ostream& operator<< (std::ostream& ostr, const NFA& nfa);

private:
	void printCalculatorImplHelper(std::ostream &ostr, const std::string &name,
				       int w, bool reduce);

	void remove_node (int n);
	void add_edge (int n, const TransLabel &t, int m);
	void remove_edge (int n, const TransLabel &t, int m);
	void add_outgoing_edges (int n, const tok_vector &v);
	void add_incoming_edges (int n, const tok_vector &v);

	void simplify_basic ();
	void compact_edges ();
	void scm_reduce ();
	std::vector<std::vector<char>> get_state_composition_matrix ();

	trans_t trans;
	trans_t trans_inv;
	std::set<int> starting;
	std::set<int> accepting;
};

#endif /* _KATER_NFA_HPP_ */
